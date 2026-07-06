#ifndef BACNET_APP_MSTP_H
#define BACNET_APP_MSTP_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool bacnet_mstp_init(void);
void bacnet_mstp_start(void);
void bacnet_mstp_send_i_am(void);
void bacnet_mstp_tick_1s(void);
void bacnet_mstp_run_30s_diagnostics(void);
bool bacnet_mstp_is_link_alive(void);
void *bacnet_mstp_port_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* BACNET_APP_MSTP_H */
