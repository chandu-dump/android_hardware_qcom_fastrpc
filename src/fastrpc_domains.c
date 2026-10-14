/* SPDX-License-Identifier: BSD-3-Clause-Clear */
#include "fastrpc_domains.h"
#include "AEEStdErr.h"
#include "fastrpc_common.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FASTRPC_SYSFS_ROOT
#define FASTRPC_SYSFS_ROOT "/sys/kernel/fastrpc"
#endif
#define PUBLIC_SESSION_STRIDE 100
#define DOMAIN_LIMIT 32
struct domain_entry {
  struct fastrpc_domain_info info;
  unsigned legacy_id;
  char legacy_name[32];
};
static int read_field(const char *directory, const char *field, char *out, size_t size) {
  char path[PATH_MAX];
  int n = snprintf(path, sizeof(path), "%s/%s/%s", FASTRPC_SYSFS_ROOT, directory, field);
  if (n < 0 || (size_t)n >= sizeof(path)) return AEE_EBADPARM;
  FILE *file = fopen(path, "r");
  if (!file) return AEE_EFAILED;
  if (!fgets(out, size, file)) { fclose(file); return AEE_EFAILED; }
  size_t len = strlen(out);
  if (len && out[len-1] == '\n') out[len-1] = 0;
  else if (!feof(file)) { fclose(file); return AEE_EBADSIZE; }
  fclose(file);
  return AEE_SUCCESS;
}
static int read_number(const char *directory, const char *field, uint32_t *value) {
  char text[32], *end;
  int rc = read_field(directory, field, text, sizeof(text));
  if (rc) return rc;
  errno = 0;
  unsigned long n = strtoul(text, &end, 10);
  if (errno || end == text || *end || text[0] == '-' || n > UINT32_MAX)
    return AEE_EBADPARM;
  *value = n;
  return AEE_SUCCESS;
}
static int read_domains(struct domain_entry *entries, size_t *count) {
  DIR *dir = opendir(FASTRPC_SYSFS_ROOT);
  if (!dir) return AEE_EFAILED;
  struct dirent *entry;
  int rc = AEE_SUCCESS;
  *count = 0;
  while ((entry = readdir(dir))) {
    if (entry->d_name[0] == '.') continue;
    if (*count == DOMAIN_LIMIT) { rc = AEE_EBADSIZE; break; }
    struct domain_entry item = {0};
#define READ_NUMBER(field, dest) do { rc = read_number(entry->d_name, field, dest); if (rc) goto done; } while (0)
    READ_NUMBER("domain_id", &item.info.domain_id);
    READ_NUMBER("instance_id", &item.info.instance_id);
    READ_NUMBER("type", &item.info.type);
    READ_NUMBER("status", &item.info.status);
    READ_NUMBER("legacy_id", &item.legacy_id);
#undef READ_NUMBER
    rc = read_field(entry->d_name, "name", item.info.name, sizeof(item.info.name));
    if (rc) break;
    rc = read_field(entry->d_name, "legacy_name", item.legacy_name, sizeof(item.legacy_name));
    if (rc) break;
    if (!item.info.name[0] || item.legacy_id >= NUM_DOMAINS ||
        item.info.domain_id > (INT_MAX - (NUM_SESSIONS - 1)) / PUBLIC_SESSION_STRIDE) {
      rc = AEE_EBADPARM; break;
    }
    for (size_t i = 0; i < *count; ++i) {
      if (entries[i].info.domain_id == item.info.domain_id ||
          entries[i].legacy_id == item.legacy_id) { rc = AEE_EBADPARM; goto done; }
    }
    entries[(*count)++] = item;
  }
done:
  closedir(dir);
  return rc;
}
/* Identity lookups are frequent during mappings; refresh status on discovery. */
static pthread_mutex_t domains_lock = PTHREAD_MUTEX_INITIALIZER;
static struct domain_entry cached_domains[DOMAIN_LIMIT];
static size_t cached_count;
static int cache_valid;
static int load_domains(struct domain_entry *entries, size_t *count, int refresh) {
  int rc = AEE_SUCCESS;
  pthread_mutex_lock(&domains_lock);
  if (!cache_valid || refresh) {
    rc = read_domains(cached_domains, &cached_count);
    cache_valid = !rc;
  }
  if (!rc) {
    *count = cached_count;
    memcpy(entries, cached_domains, cached_count * sizeof(*entries));
  }
  pthread_mutex_unlock(&domains_lock);
  return rc;
}
int remote_system_request(struct fastrpc_system_request *request) {
#ifndef FASTRPC_USE_MULTIMODE
  (void)request;
  return AEE_EUNSUPPORTED;
#else
  if (!request || request->id || request->sys.max_domains < 0 ||
      (!request->sys.domains && request->sys.max_domains) ||
      (request->sys.domains && !request->sys.max_domains)) return AEE_EBADPARM;
  struct domain_entry entries[DOMAIN_LIMIT];
  size_t count;
  int rc = load_domains(entries, &count, 1);
  if (rc) return rc;
  /* Stock reports total capacity for a size query, before applying filters. */
  if (!request->sys.domains) { request->sys.num_domains = count; return AEE_SUCCESS; }
  if (request->sys.flags > 4) return AEE_EBADPARM;
  size_t selected = 0;
  for (size_t i = 0; i < count; ++i)
    if (!request->sys.flags || request->sys.flags == entries[i].info.type) ++selected;
  if ((size_t)request->sys.max_domains < selected) return AEE_EBADPARM;
  size_t j = 0;
  for (size_t i = 0; i < count; ++i)
    if (!request->sys.flags || request->sys.flags == entries[i].info.type)
      request->sys.domains[j++] = entries[i].info;
  request->sys.num_domains = selected;
  return AEE_SUCCESS;
#endif
}
int fastrpc_domain_to_slot(int domain) {
  if (domain >= -1 && domain < NUM_DOMAINS * NUM_SESSIONS) return domain;
#ifdef FASTRPC_USE_MULTIMODE
  if (domain < 0) return -2;
  struct domain_entry entries[DOMAIN_LIMIT]; size_t count;
  if (load_domains(entries, &count, 0)) return -2;
  for (size_t i = 0; i < count; ++i) {
    if ((unsigned)domain == entries[i].info.domain_id) return entries[i].legacy_id;
    if ((unsigned)domain / PUBLIC_SESSION_STRIDE == entries[i].info.domain_id &&
        domain % PUBLIC_SESSION_STRIDE < NUM_SESSIONS)
      return entries[i].legacy_id + NUM_DOMAINS * (domain % PUBLIC_SESSION_STRIDE);
  }
#endif
  return -2;
}
int fastrpc_name_to_slot(const char *name, size_t length) {
  if (!name || !length || length >= 32) return -2;
  struct domain_entry entries[DOMAIN_LIMIT]; size_t count;
  if (load_domains(entries, &count, 0)) return -2;
  for (size_t i = 0; i < count; ++i)
    if ((strlen(entries[i].info.name) == length && !memcmp(name, entries[i].info.name, length)) ||
        (strlen(entries[i].legacy_name) == length && !memcmp(name, entries[i].legacy_name, length)))
      return entries[i].legacy_id;
  return -2;
}
int fastrpc_slot_to_domain(int slot) {
  if (slot < 0 || slot >= NUM_DOMAINS * NUM_SESSIONS) return -2;
  struct domain_entry entries[DOMAIN_LIMIT]; size_t count;
  if (load_domains(entries, &count, 0)) return -2;
  for (size_t i = 0; i < count; ++i)
    if (entries[i].legacy_id == (unsigned)slot % NUM_DOMAINS)
      return entries[i].info.domain_id * PUBLIC_SESSION_STRIDE + slot / NUM_DOMAINS;
  return -2;
}
int fastrpc_domain_from_uri(const char *uri, int standalone) {
  if (!uri) return -2;
  const char *name = standalone ? uri : strstr(uri, "&_dom=");
  if (!name) return -2;
  if (!standalone) name += strlen("&_dom=");
  int slot = fastrpc_name_to_slot(name, strcspn(name, "&"));
  if (slot < 0 || standalone) return slot;
  const char *session = strstr(uri, "&_session=");
  if (session) {
    char *end; errno = 0;
    session += strlen("&_session=");
    unsigned long id = strtoul(session, &end, 10);
    if (errno || end == session || (*end && *end != '&') || *session == '-' || id >= NUM_SESSIONS) return -2;
    slot += NUM_DOMAINS * id;
  }
  return slot;
}
