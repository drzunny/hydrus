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
static const char   *s_bind_address = nullptr;
static int           s_bind_port = 0;


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
        //printf("nread:%d, buf->len:%d\n", nread, buf->len);
        if (nread > 0)  
        {
            //printf("content:\n\n%s \n\n", buf->base);
            wsgi->append(buf->base, nread);
        }
        else if (!wsgi->has_buffer())
        {
            printf("No buffer, bye bye:\n");
            delete wsgi;
            return;
        }

        //printf("I parser it \n");
        if (wsgi->parse())
        {
            //printf("parse OK\n");
            wsgi->execute();
        }
        else
        {
            //printf("Parse error\n");
            wsgi->raiseUp(400);
        }
        delete wsgi;
    }
    else if (nread < 0)
    {
        wsgi->raiseUp(400);
        delete wsgi;
    }
    else 
    {
        wsgi->append(buf->base, nread);
        uv_read_start(stream, http_on_allocate, http_on_read);
    }
    free(buf->base);
}


static void
http_on_connection(uv_stream_t *server, int status)
{
    static int failCounter = 0;
    if (status != 0) 
    {
        printf("Status Error, bye bye:%d\n", status);
        return;
    }

    hydrus::WSGIApplication * app = new hydrus::WSGIApplication();
    if (uv_accept((uv_stream_t*)&s_http_server, _STREAM(app->raw_client())) < 0)
    {
        printf("Connect fail, bye bye:%d\n", ++failCounter);
        delete app;
    }
    else
    {
        uv_read_start((uv_stream_t*)app->raw_client(), http_on_allocate, http_on_read);
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
    s_bind_address = address;
    s_bind_port = port;
    return hydrus::Server::READY;
}


void
Server::run()
{
    uv_run(s_loop, UV_RUN_DEFAULT);
}


const char *
Server::host()
{
    return s_bind_address;
}


int
Server::port()
{
    return s_bind_port;
}


NS_END    // }
