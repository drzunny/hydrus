#include "server.h"
#include "wsgi.h"
#include "base/memory.hpp"

#include <uv.h>
#include <memory.h>

// ----------------------------------------------
//  UV Variants
// ----------------------------------------------
#define HYDRUS_SERVER_BACKLOG 128

#define _STREAM(p) ((uv_stream_t*)(p))
#define _TCP(p) ((uv_tcp_t*)(p))
#define _WSGI(p) ((hydrus::WSGIApplication*)(p->data))

static uv_loop_t   * sEventLoop     = nullptr;
static uv_tcp_t      sHttpServer;
static sockaddr_in   sIPAddress;
static const char  * sBindAddress   = nullptr;
static int           sBindPort      = 0;
static uv_signal_t   sKeyboardIntep;

#ifndef HY_SMALL_BUFFER
static hydrus::FixedMemoryPool<65535> sReadBuffer;
#else
static hydrus::FixedMemoryPool<4096>  sReadBuffer;
#endif


// ----------------------------------------------
//  HTTP callbacks
// ----------------------------------------------
static void
http_on_allocate(uv_handle_t * handle, size_t suggest, uv_buf_t * buf)
{
    buf->base = sReadBuffer.data();
    buf->len = sReadBuffer.size();
}


static void
http_on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    // NOTICE: WSGI object will be deleted after it write (execute / raise)
    auto wsgi = _WSGI(stream);

    if (nread < 0)
    {
        delete wsgi;
    }
    else if (nread < (ssize_t)buf->len)
    {
        if (nread > 0)
        {
            wsgi->append(buf->base, nread);
        }
        else if (!wsgi->hasBuffer())
        {
            delete wsgi;
            return;
        }
        if (wsgi->parse())
        {
            wsgi->execute();
        }
        else
        {
            wsgi->raiseUp(400);
            delete wsgi;
        }
    }
    else
    {
        wsgi->append(buf->base, nread);
        uv_read_start(stream, http_on_allocate, http_on_read);
    }
}


static void
http_on_connection(uv_stream_t *server, int status)
{
    static int failCounter = 0;
    if (status != 0)
        return;

    hydrus::WSGIApplication * app = new hydrus::WSGIApplication();
    if (uv_accept((uv_stream_t*)&sHttpServer, _STREAM(app->connection())) != 0)
    {
        delete app;
    }
    else
    {
        uv_read_start((uv_stream_t*)app->connection(), http_on_allocate, http_on_read);
    }
}


static void
signal_on_terminate(uv_signal_t* handle, int signum)
{
    if (signum == SIGTERM)
    {
        fprintf(stdout, "Good bye :-)\n");
        uv_signal_stop(handle);
        uv_stop(sEventLoop);
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
    if (sEventLoop)
        goto SERVER_READY;

    sEventLoop = uv_default_loop();
    uv_tcp_init(sEventLoop, &sHttpServer);
    uv_ip4_addr(address, port, &sIPAddress);

    err = uv_tcp_bind(&sHttpServer, (sockaddr*)&sIPAddress, 0);
    if (err)
    {
        return hydrus::Server::INUSED;
    }
    uv_listen((uv_stream_t*)&sHttpServer, HYDRUS_SERVER_BACKLOG, http_on_connection);

SERVER_READY:
    sBindAddress = address;
    sBindPort = port;
    return hydrus::Server::READY;
}


void
Server::run()
{
    uv_signal_init(sEventLoop, &sKeyboardIntep);
    uv_signal_start(&sKeyboardIntep, signal_on_terminate, SIGTERM);

    uv_run(sEventLoop, UV_RUN_DEFAULT);
}


const char *
Server::host()
{
    return sBindAddress;
}


int
Server::port()
{
    return sBindPort;
}


NS_END    // }
