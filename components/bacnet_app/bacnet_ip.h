#ifndef BACNET_APP_IP_H
#define BACNET_APP_IP_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void bacnet_ip_register_with_bbmd(void);
void bacnet_ip_start(void);
void bacnet_ip_send_i_am(void);
bool bacnet_ip_is_connected(void);
void bacnet_ip_get_ip_string(char *out, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_APP_IP_H */
