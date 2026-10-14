/* SPDX-License-Identifier: BSD-3-Clause-Clear */
#ifndef FASTRPC_DOMAINS_H
#define FASTRPC_DOMAINS_H
#include <stddef.h>
#include <stdint.h>

/* Android vendor FastRPC domain-query ABI, verified against stock. */
struct fastrpc_domain_info {
  uint32_t domain_id;
  uint32_t instance_id;
  char name[32];
  uint32_t type;
  uint32_t status;
  uint32_t reserved[12];
};
struct fastrpc_system_request {
  uint32_t id;
  struct {
    struct fastrpc_domain_info *domains;
    int32_t max_domains;
    int32_t num_domains;
    uint32_t flags;
  } sys;
};
_Static_assert(sizeof(struct fastrpc_domain_info) == 96, "domain ABI");
#if UINTPTR_MAX == UINT64_MAX
_Static_assert(offsetof(struct fastrpc_system_request, sys) == 8, "request ABI");
_Static_assert(sizeof(struct fastrpc_system_request) == 32, "request ABI size");
#endif
int remote_system_request(struct fastrpc_system_request *request);
/* Internal slot IDs remain private to the legacy kernel backend. */
int fastrpc_domain_to_slot(int domain);
int fastrpc_name_to_slot(const char *name, size_t length);
int fastrpc_slot_to_domain(int slot);
int fastrpc_domain_from_uri(const char *uri, int standalone);
#endif
