#include "server.h"
#include "wsgi.h"

#include <uv.h>
#include <memory.h>


// ----------------------------------------------
//  UV Variants
// ----------------------------------------------
#define HYDRUS_SERVER_BACKLOG 128

#define _STREAM(p) ((uv_stream_t*)(p))
#define _TCP(p) ((uv_tcp_t*)(p))
#define _WSGI(p) ((hydrus::WSGIApplication*)(p->data))

static uv_loop_t    *s_loop     = nullptr;
static uv_tcp_t      s_http_server;
static sockaddr_in   s_ipaddr;


// ----------------------------------------------
//  HTTP Server here
// ----------------------------------------------
static void
http_on_allocate(uv_handle_t * handle, size_t suggest, uv_buf_t * buf)
{
    buf->base = (char*)malloc(suggest);
    buf->len = suggest;
}


static void
http_on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    // NOTICE: WSGI object will be deleted after it write (execute / raise)
    auto wsgi = _WSGI(stream);

    if (nread == UV_EOF || nread < (ssize_t)buf->len)
    {
        if (wsgi->parse())
            wsgi->execute();
        else
            wsgi->raise(400);
    }
    else if (nread < 0)
    {
        wsgi->raise(400);
        delete wsgi;
    }
    else
        wsgi->append(buf->base, nread);
}


static void
http_on_connection(uv_stream_t *server, int status)
{
    if (status != 0)
        return;

    hydrus::WSGIApplication * app = new hydrus::WSGIApplication();
    if (uv_accept((uv_stream_t*)&s_http_server, _STREAM(app->client())) < 0)
    {
        delete app;
    }
    else
    {
        uv_read_start((uv_stream_t*)app->client(), http_on_allocate, http_on_read);
    }
}



// ----------------------------------------------
//  Server Class Implementation
// ----------------------------------------------

NS(hydrus) // namespace hydrus {

Server::ServerStatus
Server::listen(const char * address, int port)
{
    int err;
    if (s_loop)
        goto SERVER_READY;

    s_loop = uv_default_loop();
    uv_tcp_init(s_loop, &s_http_server);
    uv_ip4_addr(address, port, &s_ipaddr);

    err = uv_tcp_bind(&s_http_server, (sockaddr*)&s_ipaddr, 0);
    if (err)
    {
        return hydrus::Server::INUSED;
    }
    uv_listen((uv_stream_t*)&s_http_server, HYDRUS_SERVER_BACKLOG, http_on_connection);

SERVER_READY:
    return hydrus::Server::READY;
}


void
Server::run()
{
    uv_run(s_loop, UV_RUN_DEFAULT);
}

NS_END    // }
