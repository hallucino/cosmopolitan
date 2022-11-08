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
#include "libc/intrin/_getenv.internal.h"
#include "libc/intrin/strace.internal.h"
#include "libc/runtime/runtime.h"

/**
 * Returns value of environment variable, or NULL if not found.
 *
 * Environment variables can store empty string on Unix but not Windows.
 *
 * @return pointer to value of `environ` entry, or null if not found
 */
char *getenv(const char *s) {
  char **p;
  struct Env e;
  if (!s) return 0;
  if (!(p = environ)) return 0;
  e = _getenv(p, s);
#if SYSDEBUG
  // if (!(s[0] == 'T' && s[1] == 'Z' && !s[2])) {
  // TODO(jart): memoize TZ or something
  STRACE("getenv(%#s) → %#s", s, e.s);
  //}
#endif
  return e.s;
}