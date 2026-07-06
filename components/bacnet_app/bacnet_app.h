#ifndef BACNET_APP_COMPONENT_H
#define BACNET_APP_COMPONENT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void bacnet_app_init(void);
void bacnet_app_start(void);

void bacnet_app_datalink_lock(char *name);
void bacnet_app_datalink_unlock(void);
char *bacnet_app_datalink_bip(void);
char *bacnet_app_datalink_mstp(void);
char *bacnet_app_datalink_default(void);
void bacnet_app_log_whois_iam(const uint8_t *apdu, int apdu_len, const char *link);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_APP_COMPONENT_H */
