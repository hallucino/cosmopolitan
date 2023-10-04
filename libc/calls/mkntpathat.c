/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2021 Justine Alexandra Roberts Tunney                              │
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
#include "libc/calls/internal.h"
#include "libc/calls/syscall_support-nt.internal.h"
#include "libc/intrin/strace.internal.h"
#include "libc/macros.internal.h"
#include "libc/nt/enum/fileflagandattributes.h"
#include "libc/nt/files.h"
#include "libc/nt/thunk/msabi.h"
#include "libc/str/str.h"
#include "libc/sysv/consts/at.h"
#include "libc/sysv/errfuns.h"

__msabi extern typeof(GetFileAttributes) *const __imp_GetFileAttributesW;

static int __mkntpathat_impl(int dirfd, const char *path, int flags,
                             char16_t file[hasatleast PATH_MAX]) {
  char16_t dir[PATH_MAX];
  uint32_t dirlen, filelen;
  if (!isutf8(path, -1)) return eilseq();  // thwart overlong nul in conversion
  if ((filelen = __mkntpath2(path, file, flags)) == -1) return -1;
  if (!filelen) return enoent();
  if (file[0] != u'\\' && dirfd != AT_FDCWD) {  // ProTip: \\?\C:\foo
    if (!__isfdkind(dirfd, kFdFile)) return ebadf();
    dirlen = GetFinalPathNameByHandle(g_fds.p[dirfd].handle, dir, ARRAYLEN(dir),
                                      kNtFileNameNormalized | kNtVolumeNameDos);
    if (!dirlen) return __winerr();
    if (dirlen + 1 + filelen + 1 > ARRAYLEN(dir)) {
      STRACE("path too long: %#.*hs\\%#.*hs", dirlen, dir, filelen, file);
      return enametoolong();
    }
    dir[dirlen] = u'\\';
    memcpy(dir + dirlen + 1, file, (filelen + 1) * sizeof(char16_t));
    memcpy(file, dir, (dirlen + 1 + filelen + 1) * sizeof(char16_t));
    return dirlen + 1 + filelen;
  } else {
    return filelen;
  }
}

int __mkntpathat(int dirfd, const char *path, int flags,
                 char16_t file[hasatleast PATH_MAX]) {

  // convert the path.
  int len;
  if ((len = __mkntpathat_impl(dirfd, path, flags, file)) == -1) {
    return -1;
  }

  // if path ends with a slash, then we need to manually do what linux
  // does and check to make sure it's a directory, and return ENOTDIR,
  // since WIN32 will reject the path with EINVAL if we don't do this.
  if (len && file[len - 1] == '\\') {
    uint32_t fattr;
    if (len > 1 && !(len == 3 && file[1] == ':')) {
      file[--len] = 0;
    }
    if ((fattr = __imp_GetFileAttributesW(file)) != -1u &&
        !(fattr & kNtFileAttributeReparsePoint) &&
        !(fattr & kNtFileAttributeDirectory)) {
      return enotdir();
    }
  }

  return len;
}
