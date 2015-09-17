#include "server.h"
#include "request.h"
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
    free(hnd);
}


static void
_hy_onwrite(uv_write_t * writer, int status)   {
    free(writer);
}


static void
_hy_onread(uv_stream_t * stream, ssize_t nread, const uv_buf_t * buf)    {
    auto client = (hydrus::Client*)stream->data;
    auto server = (hydrus::Server*)hy_server.data;

    if (nread < 0)  {
        delete client;
        return;
    }

    hydrus::Request req(buf->base, nread);
    server->callback(*client, req);

    // release old buffer
    free(buf->base);
    delete client;
}


static void
_hy_onconnection(uv_stream_t * server, int status) {
    if (status != 0)    {
        // TODO: on error for connection
        return;
    }
    uv_tcp_t * socket = new uv_tcp_t;
    uv_tcp_init(hy_loop, socket);

    hydrus::Client * client = new hydrus::Client((void*)socket);
    if (!uv_accept(server, (uv_stream_t*)socket))
    {
        socket->data = (void*)client;
        uv_read_start((uv_stream_t*)socket, _hy_onallocate, _hy_onread);
    }
    else
        delete client;
}


// ------------------------------------------------
//  HTTP by libuv
// ------------------------------------------------
static void
_http_listen(hydrus::Server * server, const char * addr, int port, bool ipv6=false)   {
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

    hy_server.data = (void*)server;
    uv_listen((uv_stream_t*)&hy_server, HTTP_LISTEN_BACKLOG, _hy_onconnection);
}


static void
_http_start()   {
    uv_run(hy_loop, UV_RUN_DEFAULT);
}


// ------------------------------------------------
//  HTTP Client implementation
// ------------------------------------------------
class hydrus::Client::ClientImpl
{
public:
    uv_tcp_t * client = nullptr;

    ClientImpl(uv_tcp_t * tcp) : client(tcp) {}
    ~ClientImpl() { uv_close((uv_handle_t*)client, _hy_onclientclose); }
};


hydrus::Client::Client(void * hnd) : impl_(new ClientImpl((uv_tcp_t*)hnd))
{
    uv_tcp_t * tcp = (uv_tcp_t*)hnd;

    sockaddr_in addr;
    int len;
    uv_tcp_getsockname(tcp, (sockaddr*)&addr, &len);
    uv_ip4_name(&addr, remote_address_, len);
}


hydrus::Client::~Client()
{
    delete impl_;
}


void
hydrus::Client::send(const char * buf, size_t sz) const
{
    uv_write_t * w = new uv_write_t;
    char * buffer = new char[sz];
    memcpy(buffer, buf, sz);
    uv_buf_t data = uv_buf_init(buffer, sz);
    w->data = buffer;
    uv_write(w, (uv_stream_t*)impl_->client, &data, 1, _hy_onwrite);
}


// ------------------------------------------------
//  HTTP Server implementation
// ------------------------------------------------
void
hydrus::Server::listen(const char * addr, int port)
{
    _http_listen(this, addr, port);
}


void
hydrus::Server::run()
{
    _http_start();
}
