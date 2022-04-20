/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2020 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
#include "libc/bits/bits.h"
#include "libc/bits/pushpop.h"
#include "libc/bits/weaken.h"
#include "libc/calls/internal.h"
#include "libc/calls/strace.internal.h"
#include "libc/dce.h"
#include "libc/elf/pf2prot.internal.h"
#include "libc/errno.h"
#include "libc/fmt/fmt.h"
#include "libc/intrin/describeflags.internal.h"
#include "libc/intrin/kprintf.h"
#include "libc/intrin/nomultics.internal.h"
#include "libc/log/libfatal.internal.h"
#include "libc/macros.internal.h"
#include "libc/nexgen32e/bsr.h"
#include "libc/nexgen32e/nt2sysv.h"
#include "libc/nexgen32e/rdtsc.h"
#include "libc/nt/console.h"
#include "libc/nt/enum/consolemodeflags.h"
#include "libc/nt/enum/filemapflags.h"
#include "libc/nt/enum/filetype.h"
#include "libc/nt/enum/loadlibrarysearch.h"
#include "libc/nt/enum/pageflags.h"
#include "libc/nt/enum/version.h"
#include "libc/nt/files.h"
#include "libc/nt/memory.h"
#include "libc/nt/pedef.internal.h"
#include "libc/nt/process.h"
#include "libc/nt/runtime.h"
#include "libc/nt/signals.h"
#include "libc/nt/struct/ntexceptionpointers.h"
#include "libc/nt/struct/teb.h"
#include "libc/nt/synchronization.h"
#include "libc/nt/thunk/msabi.h"
#include "libc/runtime/directmap.internal.h"
#include "libc/runtime/internal.h"
#include "libc/runtime/memtrack.internal.h"
#include "libc/runtime/runtime.h"
#include "libc/sock/internal.h"
#include "libc/str/tpenc.h"
#include "libc/str/utf16.h"

#if IsTiny()
__msabi extern typeof(CreateFileMapping) *const __imp_CreateFileMappingW;
__msabi extern typeof(MapViewOfFileEx) *const __imp_MapViewOfFileEx;
__msabi extern typeof(VirtualProtect) *const __imp_VirtualProtect;
#define CreateFileMapping __imp_CreateFileMappingW
#define MapViewOfFileEx   __imp_MapViewOfFileEx
#define VirtualProtect    __imp_VirtualProtect
#endif

#define AT_EXECFN     31L
#define MAP_ANONYMOUS 32
#define MAP_PRIVATE   2
#define PROT_EXEC     4
#define PROT_READ     1
#define PROT_WRITE    2

/*
 * TODO: Why can't we allocate addresses above 4GB on Windows 7 x64?
 * TODO: How can we ensure we never overlap with KERNEL32.DLL?
 */

struct WinArgs {
  char *argv[4096];
  char *envp[4092];
  intptr_t auxv[2][2];
  char argblock[ARG_MAX];
  char envblock[ARG_MAX];
};

extern uint32_t __winmainpid;
extern int64_t __wincrashearly;
extern const char kConsoleHandles[3];

static const short kConsoleModes[3] = {
    kNtEnableProcessedInput | kNtEnableLineInput | kNtEnableEchoInput |
        kNtEnableMouseInput | kNtEnableQuickEditMode | kNtEnableExtendedFlags |
        kNtEnableAutoPosition | kNtEnableInsertMode |
        kNtEnableVirtualTerminalInput,
    kNtEnableProcessedOutput | kNtEnableWrapAtEolOutput |
        kNtEnableVirtualTerminalProcessing,
    kNtEnableProcessedOutput | kNtEnableWrapAtEolOutput |
        kNtEnableVirtualTerminalProcessing,
};

forceinline void MakeLongDoubleLongAgain(void) {
  /* 8087 FPU Control Word
      IM: Invalid Operation ───────────────┐
      DM: Denormal Operand ───────────────┐│
      ZM: Zero Divide ───────────────────┐││
      OM: Overflow ─────────────────────┐│││
      UM: Underflow ───────────────────┐││││
      PM: Precision ──────────────────┐│││││
     PC: Precision Control ────────┐  ││││││
      {float,∅,double,long double} │  ││││││
     RC: Rounding Control ───────┐ │  ││││││
      {even, →-∞, →+∞, →0}       │┌┤  ││││││
                                ┌┤││  ││││││
                               d││││rr││││││*/
  int x87cw = 0b0000000000000000001101111111;
  asm volatile("fldcw\t%0" : /* no outputs */ : "m"(x87cw));
}

__msabi static textwindows int OnEarlyWinCrash(struct NtExceptionPointers *ep) {
  uint32_t wrote;
  char buf[64], *p = buf;
  *p++ = 'c';
  *p++ = 'r';
  *p++ = 'a';
  *p++ = 's';
  *p++ = 'h';
  *p++ = ' ';
  *p++ = '0';
  *p++ = 'x';
  p = __fixcpy(p, ep->ExceptionRecord->ExceptionCode, 32);
  *p++ = ' ';
  *p++ = 'r';
  *p++ = 'i';
  *p++ = 'p';
  *p++ = ' ';
  p = __fixcpy(p, ep->ContextRecord ? ep->ContextRecord->Rip : -1, 32);
  *p++ = '\r';
  *p++ = '\n';
  WriteFile(GetStdHandle(kNtStdErrorHandle), buf, p - buf, &wrote, 0);
  ExitProcess(200);
}

__msabi static textwindows wontreturn void WinMainNew(const char16_t *cmdline) {
  bool32 rc;
  int64_t h, hand;
  uint32_t oldprot;
  struct WinArgs *wa;
  const char16_t *env16;
  int i, prot, count, version;
  intptr_t stackaddr, allocaddr;
  size_t allocsize, argsize, stacksize;
  version = NtGetPeb()->OSMajorVersion;
  __oldstack = (intptr_t)__builtin_frame_address(0);
  if ((intptr_t)v_ntsubsystem == kNtImageSubsystemWindowsCui && version >= 10) {
    __winmainpid = __pid;
    rc = SetConsoleCP(kNtCpUtf8);
    NTTRACE("SetConsoleCP(kNtCpUtf8) → %hhhd", rc);
    rc = SetConsoleOutputCP(kNtCpUtf8);
    NTTRACE("SetConsoleOutputCP(kNtCpUtf8) → %hhhd", rc);
    for (i = 0; i < 3; ++i) {
      hand = GetStdHandle(kConsoleHandles[i]);
      rc = GetConsoleMode(hand, __ntconsolemode + i);
      NTTRACE("GetConsoleMode(%p, [%s]) → %hhhd", hand,
              i ? DescribeNtConsoleModeOutputFlags(__ntconsolemode[i])
                : DescribeNtConsoleModeInputFlags(__ntconsolemode[i]),
              rc);
      rc = SetConsoleMode(hand, kConsoleModes[i]);
      NTTRACE("SetConsoleMode(%p, %s) → %hhhd", hand,
              i ? DescribeNtConsoleModeOutputFlags(kConsoleModes[i])
                : DescribeNtConsoleModeInputFlags(kConsoleModes[i]),
              rc);
    }
  }
  _mmi.p = _mmi.s;
  _mmi.n = ARRAYLEN(_mmi.s);
  argsize = ROUNDUP(sizeof(struct WinArgs), FRAMESIZE);
  stackaddr = GetStaticStackAddr(0);
  stacksize = GetStackSize();
  allocsize = argsize + stacksize;
  allocaddr = stackaddr - argsize;
  NTTRACE("WinMainNew() mapping %'zu byte arg block + stack at %p", allocsize,
          allocaddr);
  MapViewOfFileEx(
      (_mmi.p[0].h =
           CreateFileMapping(-1, &kNtIsInheritable, kNtPageExecuteReadwrite,
                             allocsize >> 32, allocsize, NULL)),
      kNtFileMapWrite | kNtFileMapExecute, 0, 0, allocsize, (void *)allocaddr);
  prot = (intptr_t)ape_stack_prot;
  if (~prot & PROT_EXEC) {
    VirtualProtect((void *)allocaddr, allocsize, kNtPageReadwrite, &oldprot);
  }
  _mmi.p[0].x = allocaddr >> 16;
  _mmi.p[0].y = (allocaddr >> 16) + ((allocsize >> 16) - 1);
  _mmi.p[0].prot = prot;
  _mmi.p[0].flags = MAP_PRIVATE | MAP_ANONYMOUS;
  _mmi.p[0].size = allocsize;
  _mmi.i = 1;
  wa = (struct WinArgs *)allocaddr;
  NTTRACE("WinMainNew() loading arg block");
  count = GetDosArgv(cmdline, wa->argblock, ARRAYLEN(wa->argblock), wa->argv,
                     ARRAYLEN(wa->argv));
  for (i = 0; wa->argv[0][i]; ++i) {
    if (wa->argv[0][i] == '\\') {
      wa->argv[0][i] = '/';
    }
  }
  env16 = GetEnvironmentStrings();
  NTTRACE("WinMainNew() loading environment");
  GetDosEnviron(env16, wa->envblock, ARRAYLEN(wa->envblock) - 8, wa->envp,
                ARRAYLEN(wa->envp) - 1);
  FreeEnvironmentStrings(env16);
  NTTRACE("WinMainNew() switching stacks");
  _jmpstack((char *)(stackaddr + stacksize - (intptr_t)ape_stack_align), cosmo,
            count, wa->argv, wa->envp, wa->auxv);
}

/**
 * Main function on Windows NT.
 *
 * The Cosmopolitan Runtime provides the following services, which aim
 * to bring Windows NT behavior closer in harmony with System Five:
 *
 * 1. We configure CMD.EXE for UTF-8 and enable ANSI colors on Win10.
 *
 * 2. Command line arguments are passed as a blob of UTF-16 text. We
 *    chop them up into an char *argv[] UTF-8 data structure, in
 *    accordance with the DOS conventions for argument quoting.
 *
 * 3. Environment variables are passed to us as a sorted UTF-16 double
 *    NUL terminated list. We translate this to char ** using UTF-8.
 *
 * 4. Allocates new stack at a high address. NT likes to choose a
 *    stack address that's beneath the program image. We want to be
 *    able to assume that stack addresses are located at higher
 *    addresses than heap and program memory.
 *
 * 5. Reconfigure x87 FPU so long double is actually long (80 bits).
 *
 * 6. Finally, we need fork. Since disagreeing with fork is axiomatic to
 *    Microsoft's engineering culture, we need to go to great lengths to
 *    have it anyway without breaking Microsoft's rules: using the WIN32
 *    API (i.e. not NTDLL) to copy MAP_PRIVATE pages via a pipe. It'd go
 *    faster if the COW pages CreateFileMappingNuma claims to have turns
 *    out to be true. Until then we have a "PC Scale" and entirely legal
 *    workaround that they hopefully won't block using Windows Defender.
 *
 * @param hInstance call GetModuleHandle(NULL) from main if you need it
 */
__msabi textwindows int64_t WinMain(int64_t hInstance, int64_t hPrevInstance,
                                    const char *lpCmdLine, int nCmdShow) {
  const char16_t *cmdline;
  extern char os asm("__hostos");
  extern uint64_t ts asm("kStartTsc");
  os = WINDOWS; /* madness https://news.ycombinator.com/item?id=21019722 */
  ts = rdtsc();
  __nomultics = true;
  __pid = GetCurrentProcessId();
  __wincrashearly = AddVectoredExceptionHandler(1, (void *)OnEarlyWinCrash);
  cmdline = GetCommandLine();
#ifdef SYSDEBUG
  /* sloppy flag-only check for early initialization */
  if (__strstr16(cmdline, u"--strace")) ++__strace;
#endif
  NTTRACE("WinMain()");
  MakeLongDoubleLongAgain();
  if (weaken(WinSockInit)) weaken(WinSockInit)();
  if (weaken(WinMainForked)) weaken(WinMainForked)();
  WinMainNew(cmdline);
}
