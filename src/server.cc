#include "server.h"
#include "request.h"
#include "response.h"

#include "http-parser.h"
#include <Python.h>
#include <uv.h>

// --------------------------------------------
//  This modules internal variants
// --------------------------------------------
static uv_loop_t s_hy_loop;
static uv_tcp_t s_hy_connection;


// --------------------------------------------
//  helpers
// --------------------------------------------



// --------------------------------------------
//  UV Callbacks
// --------------------------------------------



// --------------------------------------------
//  HTTP Server implementation
// --------------------------------------------
hydrus::HttpServer(const char * addr, int port): port_(port)
{
    struct sockaddr_in s_addr;
    memcpy(addr_, addr, strlen(addr));
    uv_ip4_addr(&addr_[0], port_, &s_addr)
    uv_loop_init(&s_hy_loop);
    uv_tcp_init(&s_hy_loop, &s_hy_connection);
    uv_tcp_bind(&s_hy_connection, s_addr, 0)
}

void HttpServer::run(WSGICallback app)
{
    uv_run(&s_hy_loop, UV_RUN_DEFAULT);
    uv_loop_close(&s_hy_loop)
}
