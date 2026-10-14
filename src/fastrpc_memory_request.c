/* SPDX-License-Identifier: BSD-3-Clause-Clear */
#include "fastrpc_memory_request.h"
#include "AEEStdErr.h"
int fastrpc_mem_request(struct fastrpc_memory_request *request) {
#ifndef FASTRPC_USE_MULTIMODE
  (void)request;
  return AEE_EUNSUPPORTED;
#else
  if (!request) return AEE_EBADPARM;
  switch (request->operation) {
  case 0:
    if (request->memory.map.length > SIZE_MAX) return AEE_EBADPARM;
    return fastrpc_mmap_with_attrs(request->memory.map.domain,
        request->memory.map.fd, NULL, 0, request->memory.map.length,
        request->memory.map.flags, request->memory.map.attributes, 1);
  case 1:
    return fastrpc_munmap_with_attrs(request->memory.unmap.domain,
        request->memory.unmap.fd, NULL, 0, request->memory.unmap.attributes);
  default:
    return AEE_EUNSUPPORTED;
  }
#endif
}
