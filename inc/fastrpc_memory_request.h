/* SPDX-License-Identifier: BSD-3-Clause-Clear */
#ifndef FASTRPC_MEMORY_REQUEST_H
#define FASTRPC_MEMORY_REQUEST_H
#include <stddef.h>
#include <stdint.h>
#include "remote.h"
/* Vendor request ABI: map (0) and unmap (1), with explicit SMMU attributes. */
struct fastrpc_memory_request {
  uint32_t operation;
  union {
    struct { int32_t domain, fd; uint64_t length; uint32_t flags, attributes; } map;
    struct { int32_t domain, fd; uint32_t attributes; } unmap;
  } memory;
};
_Static_assert(offsetof(struct fastrpc_memory_request, memory) == 8, "memory ABI");
_Static_assert(sizeof(struct fastrpc_memory_request) == 32, "memory ABI size");
int fastrpc_session_init(int domain, int *dev);
int fastrpc_mem_request(struct fastrpc_memory_request *request);
int fastrpc_mmap_with_attrs(int domain, int fd, void *vaddr, int offset, size_t length,
                           enum fastrpc_map_flags flags, uint32_t attributes, int initialize);
int fastrpc_munmap_with_attrs(int domain, int fd, void *vaddr, size_t length,
                             uint32_t attributes);
#endif
