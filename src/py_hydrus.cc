#include "py_hydrus.h"

#include "request.h"
#include "server.h"

// -------------------------------------
//  other things
// -------------------------------------
static hydrus::HttpServer * hy_server;


// -------------------------------------
//  py_hydrus implementation here
// -------------------------------------
void
hydrus_init()   {
    hydrus::Request::ready();
}


void
hydrus_listen(const char * address, int port, int ipv6)    {
    hy_server = new hydrus::HttpServer(address, port, ipv6 != 0);
}


void
hydrus_run(WSGICallback app)    {
    hy_server->run();
}
