#include "hy_core.h"

#include "wsgi.h"
#include "server.h"

using namespace hydrus;

void
hydrus_setup(WSGICallback callback)
{
    WSGIReady(callback);
}


int
hydrus_listen(const char * address, int port)
{
    return Server::listen(address, port);
}


void
hydrus_run()
{
    Server::run();
}
