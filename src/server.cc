#include "server.h"
#include "stream.h"
#include "request.h"
#include "response.h"

#include <memory>
#include <Python.h>
#include <uv.h>

using namespace std;

// ------------------------------------------------
//  This modules internal variants and structs
// ------------------------------------------------
#define HTTP_LISTEN_BACKLOG 1

static uv_loop_t *  hy_loop;
static uv_tcp_t     hy_server;
static uv_tcp_t     hy_client;


// ------------------------------------------------
//  helpers
// ------------------------------------------------




// ------------------------------------------------
//  UV Callbacks
// ------------------------------------------------
static void
_hy_onallocate(uv_handle_t * hnd, size_t suggest, uv_buf_t * buf)    {
    buf->base = (char*)malloc(suggest);
    buf->len = suggest;
}


static void
_hy_onclientclose(uv_handle_t * hnd)    {
}


static void
_hy_onwrite(uv_write_t * writer, int status)   {
    if (status < 0) {
        goto onwrite_end;
    }

    // if this is not a short response
    if (writer->data != nullptr && !hydrus::Response::isFixedBuffer((const char *)writer->data))
    {
        free(writer->data);
    }

onwrite_end:
    uv_close((uv_handle_t*)writer->handle, _hy_onclientclose);
    free(writer);
}


static void
_hy_onread(uv_stream_t * client, ssize_t nread, const uv_buf_t * buf)    {
    if (nread < 0)  {
        uv_close((uv_handle_t*)client, _hy_onclientclose);
        return;
    }

    hydrus::Request req;
    hydrus::Buffer * buffer = new hydrus::Buffer();

    if (req.parse(hydrus::Buffer(buf->base, buf->len)))
    {
        hydrus::Response resp(std::move(req));
        resp.apply();
        *buffer = resp.data();
    }
    else
    {
        *buffer = hydrus::Response::raise(400);
    }

    uv_write_t * w = (uv_write_t*)malloc(sizeof(uv_write_t));
    uv_buf_t data = uv_buf_init(buffer->data(), buffer->len);
    w->data = buffer;

    uv_write(w, client, &data, 1, _hy_onwrite);
}


static void
_hy_onconnection(uv_stream_t * server, int status) {
    if (status != 0)    {
        // TODO: on error for connection
        return;
    }
    if (!uv_accept(server, (uv_stream_t*)&hy_client))
    {
        uv_read_start((uv_stream_t*)&hy_client, _hy_onallocate, _hy_onread);
    }
    else
        uv_close((uv_handle_t*)&hy_client, _hy_onclientclose);
}


// ------------------------------------------------
//  HTTP by libuv
// ------------------------------------------------
static void
_http_listen(const char * addr, int port, bool ipv6=false)   {
    hy_loop = uv_default_loop();
    uv_tcp_init(hy_loop, &hy_server);


    sockaddr * ip = NULL;
    if (ipv6)   {
        ip = (sockaddr*)malloc(sizeof(sockaddr_in));
        uv_ip4_addr(addr, port, (sockaddr_in*)ip);
    }   else    {
        ip = (sockaddr*)malloc(sizeof(sockaddr_in6));
        uv_ip6_addr(addr, port, (sockaddr_in6*)ip);
    }
    uv_tcp_bind(&hy_server, ip, 0);
    uv_listen((uv_stream_t*)&hy_server, HTTP_LISTEN_BACKLOG, _hy_onconnection);
}


static void
_http_start()   {
    uv_run(hy_loop, UV_RUN_DEFAULT);
}

// ------------------------------------------------
//  HTTP Server implementation
// ------------------------------------------------
static hydrus::HttpServer * s_server = nullptr;

hydrus::HttpServer *
hydrus::HttpServer::createServer()
{
    if (s_server == nullptr)
        s_server = new HttpServer();
    return s_server;
}


void
hydrus::HttpServer::release()
{
    if (s_server == nullptr)
        return;
    delete s_server;
    s_server = nullptr;
}


void
hydrus::HttpServer::setup(hydrus::WSGICallback cb)
{
}


void hydrus::HttpServer::listen(const string & addr, int port)
{
    _http_listen(addr.c_str(), port);
}


void hydrus::HttpServer::run()
{
    _http_start();
}
