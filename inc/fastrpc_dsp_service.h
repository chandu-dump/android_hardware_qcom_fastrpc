/* SPDX-License-Identifier: BSD-3-Clause-Clear */
#ifndef FASTRPC_DSP_SERVICE_H
#define FASTRPC_DSP_SERVICE_H
#ifdef __cplusplus
extern "C" {
#endif
int fastrpc_hal_open(int domain);
/* Returns nonzero if this descriptor was owned and closed by the HAL client. */
int fastrpc_hal_close(int fd);
void fastrpc_hal_deinit(void);
#ifdef __cplusplus
}
#endif
#endif
