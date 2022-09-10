/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2022 Justine Alexandra Roberts Tunney                              │
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
#include "libc/calls/struct/sched_param.h"
#include "libc/sysv/consts/sched.h"
#include "libc/thread/freebsd.internal.h"
#include "libc/thread/posixthread.internal.h"

#define RTP_SET_FREEBSD       1
#define PRI_REALTIME_FREEBSD  2
#define RTP_PRIO_MAX_FREEBSD  31
#define PRI_FIFO_BIT_FREEBSD  8
#define PRI_FIFO_FREEBSD      (PRI_REALTIME_FREEBSD | PRI_FIFO_BIT_FREEBSD)
#define PRI_TIMESHARE_FREEBSD 3

int rtprio_thread(int fun, int tid, struct rtprio *inout_rtp);

int _pthread_setschedparam_freebsd(int tid, int policy,
                                   const struct sched_param *param) {
  struct rtprio rtp;
  if (policy == SCHED_RR) {
    rtp.type = PRI_REALTIME_FREEBSD;
    rtp.prio = RTP_PRIO_MAX_FREEBSD - param->sched_priority;
  } else if (policy == SCHED_FIFO) {
    rtp.type = PRI_FIFO_FREEBSD;
    rtp.prio = RTP_PRIO_MAX_FREEBSD - param->sched_priority;
  } else {
    rtp.type = PRI_TIMESHARE_FREEBSD;
    rtp.prio = 0;
  }
  return rtprio_thread(RTP_SET_FREEBSD, tid, &rtp);
}
