// SPDX-License-Identifier: BSD-3-Clause-Clear
#include "fastrpc_dsp_service.h"
#include <aidl/vendor/qti/hardware/dsp/IDspService.h>
#include <aidl/vendor/qti/hardware/dsp/IDspManager.h>
#include <android/binder_manager.h>
#include <cerrno>
#include <fcntl.h>
#include <memory>
#include <pthread.h>
#include <unistd.h>

using aidl::vendor::qti::hardware::dsp::DSPDomain;
using aidl::vendor::qti::hardware::dsp::FastrpcDev;
using aidl::vendor::qti::hardware::dsp::IDspManager;
using aidl::vendor::qti::hardware::dsp::IDspService;
namespace {
struct Session {
  std::shared_ptr<IDspManager> manager;
  DSPDomain domain;
  int fd = -1;
  Session *next = nullptr;
  ~Session() {
    if (fd >= 0) close(fd);
    if (manager) {
      int32_t result;
      manager->closeSession(domain, &result);
    }
  }
};
pthread_mutex_t sessions_lock = PTHREAD_MUTEX_INITIALIZER;
Session *sessions;
}

extern "C" int fastrpc_hal_open(int domain) {
  if (domain < 0) { errno = EINVAL; return -1; }
  const std::string instance = std::string(IDspService::descriptor) + "/default";
  auto service = IDspService::fromBinder(ndk::SpAIBinder(
      AServiceManager_checkService(instance.c_str())));
  if (!service) { errno = ENODEV; return -1; }
  auto session = std::make_unique<Session>();
  session->domain.domain = domain;
  auto status = service->getNewDSPSession(0, &session->manager);
  if (!status.isOk() || !session->manager) { errno = EIO; return -1; }
  FastrpcDev result;
  status = session->manager->openSession(session->domain, &result);
  if (!status.isOk() || result.error || result.handle.fds.size() != 1) {
    errno = EACCES;
    return -1;
  }
  int fd = fcntl(result.handle.fds[0].get(), F_DUPFD_CLOEXEC, 0);
  if (fd < 0) return -1;
  session->fd = fd;
  pthread_mutex_lock(&sessions_lock);
  session->next = sessions;
  sessions = session.release();
  pthread_mutex_unlock(&sessions_lock);
  return fd;
}

extern "C" int fastrpc_hal_close(int fd) {
  Session *session = nullptr;
  pthread_mutex_lock(&sessions_lock);
  for (Session **entry = &sessions; *entry; entry = &(*entry)->next) {
    if ((*entry)->fd == fd) {
      session = *entry;
      *entry = session->next;
      break;
    }
  }
  pthread_mutex_unlock(&sessions_lock);
  // Release the Binder session outside the registry lock.
  if (!session) return 0;
  delete session;
  return 1;
}
extern "C" void fastrpc_hal_deinit(void) {
  pthread_mutex_lock(&sessions_lock);
  Session *session = sessions;
  sessions = nullptr;
  pthread_mutex_unlock(&sessions_lock);
  while (session) {
    Session *next = session->next;
    delete session;
    session = next;
  }
}
