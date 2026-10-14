/* SPDX-License-Identifier: BSD-3-Clause-Clear */
#ifndef FASTRPC_MULTIMODE_H
#define FASTRPC_MULTIMODE_H

/* Wire layout from Qualcomm dsp-kernel's multimode ioctl ABI. */
struct fastrpc_multimode_request {
  __u32 req;
  __u64 invparam;
  __u64 size;
  __u32 dynamic_domains;
  __u32 reserved[7];
};
#define FASTRPC_IOCTL_MULTIMODE _IOWR('R', 12, struct fastrpc_multimode_request)
enum {
  FASTRPC_MULTIMODE_ENHANCED = 2,
  FASTRPC_MULTIMODE_CONTROL = 3,
  FASTRPC_MULTIMODE_SIGNAL = 4,
  FASTRPC_MULTIMODE_NOTIF = 5,
  FASTRPC_MULTIMODE_MULTISESSION = 6,
  FASTRPC_MULTIMODE_CONFIG = 7,
  FASTRPC_MULTIMODE_SESSION = 8,
  FASTRPC_MULTIMODE_CONTEXT = 9,
};
struct fastrpc_multimode_enhanced {
  struct fastrpc_ioctl_invoke inv;
  __u64 crc;
  __u64 perf_kernel;
  __u64 perf_dsp;
  __u32 priority;
  __u32 reserved[19];
};
_Static_assert(sizeof(struct fastrpc_multimode_enhanced) == 120,
               "Unexpected enhanced invoke layout");
struct fastrpc_multimode_context {
  __u32 req;
  __u64 user_ctx;
  __u64 domain_ids;
  __u64 session_ids;
  __u32 num_domains;
  __u64 ctx;
  __u32 reserved[8];
};
_Static_assert(sizeof(struct fastrpc_multimode_context) == 80, "context ABI");
struct fastrpc_multimode_signal {
  __u32 req;
  __u32 signal_id;
  __u32 value;
};
struct fastrpc_multimode_notif {
  __u32 domain;
  __u32 session;
  __u32 status;
};
_Static_assert(sizeof(struct fastrpc_multimode_request) == 56,
               "Unexpected multimode ioctl layout");
#endif
