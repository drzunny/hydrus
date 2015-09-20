#ifndef HYDRUS_CORE_H
#define HYDRUS_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

void hydrus_setup();
int  hydrus_listen(const char * address, int port);
void hydrus_run();

#ifdef __cplusplus
}
#endif

#endif
