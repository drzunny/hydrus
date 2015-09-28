#ifndef HYDRUS_CORE_H
#define HYDRUS_CORE_H

#include "wsgi.h"

void hydrus_setup(hydrus::WSGICallback callback);
int  hydrus_listen(const char * address, int port);
void hydrus_run();

#endif
