#include "wsgi.h"
#include "server.h"
#include "base/definition.hpp"
#include "base/memory.hpp"

#include <uv.h>
#include "http_parser.h"

#include <algorithm>
#include <cassert>
#include <memory.h>
using namespace std;

#define _WSGI(p) ((hydrus::WSGIApplication*)(p->data))
#define _CLIENT(p) ((p->client()))
#define _CONNECTION(p) ((uv_tcp_t*)(p->connection()))


// ----------------------------------
//  Raise Output
// ----------------------------------
static const char kWSGIStatus_400[] = { "HTTP/1.1 400 Bad Request\r\nServer: hydrus\r\n\r\n[hydrus]Bad Request" };
static const char kWSGIStatus_404[] = { "HTTP/1.1 404 Not Found\r\nServer: hydrus\r\n\r\n[hydrus]Not Found" };
static const char kWSGIStatus_500[] = { "HTTP/1.1 500 Internal Server Error\r\nServer: hydrus\r\n\r\n[hydrus]Internal Server Error" };

#ifndef HY_SMALL_BUFFER

static hydrus::FixedMemoryPool<1024*1024> sWriteBuffer;
static const size_t kSendFileBuffer = 1024 * 1024;

#else

static hydrus::FixedMemoryPool<64*1024>   sWriteBuffer;
static const size_t kSendFileBuffer = 128 * 1024;

#endif


// -----------------------------------
//  helper functions
// -----------------------------------
static hydrus::WSGICallback sWSGIHandler = nullptr;
static http_parser_settings sParserSetting;

struct hydrus::WSGIClient
{
    uv_tcp_t*       tcp;
    http_parser     parser;
    bool            openning;
    std::string     tName;
    std::string     tValue;

    WSGIClient() :openning(false), tcp(new uv_tcp_t)
    {
        uv_tcp_init(uv_default_loop(), tcp);
    }
};


// -----------------------------------
//  UV callbacks
// -----------------------------------
static void
http_on_write(uv_write_t* req, int status)
{
    free(req);
}


static void
http_on_close(uv_handle_t * hnd)
{
    free(hnd);
}


static void
fs_on_sendfile(uv_fs_t * fs)
{
    auto wsgi = _WSGI(fs);
    uv_fs_req_cleanup(fs);
    free(fs);
}



// -----------------------------------
//  request parser
// -----------------------------------
static int
parser_on_begin(http_parser * pa)
{
    return 0;
}


static int
parser_on_url(http_parser* pa, const char *at, size_t length)
{
    auto wsgi = _WSGI(pa);
    wsgi->URL = string(at, length);
    return 0;
}


static int
parser_on_body(http_parser* pa, const char *at, size_t length)
{
    auto wsgi = _WSGI(pa);
    wsgi->BODY = string(at, length);
    return 0;
}


static int
parser_on_header_field(http_parser* pa, const char *at, size_t length)
{
    auto wsgi = _WSGI(pa);
    auto client = _CLIENT(wsgi);

    client->openning = true;
    client->tName = string(at, length);

    return 0;
}


static int
parser_on_header_value(http_parser* pa, const char *at, size_t length)
{
    auto wsgi = _WSGI(pa);
    auto client = _CLIENT(wsgi);

    if (!client->openning)
        return 0;

    client->tValue = string(at, length);
    client->openning = false;

    wsgi->HEADERS.push_back({ client->tName, client->tValue });
    return 0;
}


static int
parser_on_header_complete(http_parser* pa)
{
    auto wsgi = _WSGI(pa);
    auto client = _CLIENT(wsgi);

    if (client->openning)
    {
        return -1;
    }
    return 0;
}


static int
parser_on_complete(http_parser* pa)
{
    static char s_addr_buf[64];
    hydrus::WSGIApplication * wsgi = _WSGI(pa);
    uv_tcp_t * connection = _CONNECTION(wsgi);

    sockaddr_in ip;
    int len;
    uv_tcp_getpeername(connection, (sockaddr*)&ip, &len);
    uv_ip4_name(&ip, s_addr_buf, len);

    wsgi->REMOTE_ADDR = s_addr_buf;
    wsgi->REQUEST_METHOD = http_method_str((http_method)pa->method);

    return 0;
}



// -----------------------------------
//  WSGIApplication impletation
// -----------------------------------
NS(hydrus)

void
WSGIReady(WSGICallback callback)
{
    if (sWSGIHandler)
        return;

    // setup callback
    sWSGIHandler = callback;

    // setup parser callback
    sParserSetting.on_message_begin = parser_on_begin;
    sParserSetting.on_url = parser_on_url;
    sParserSetting.on_body = parser_on_body;
    sParserSetting.on_header_field = parser_on_header_field;
    sParserSetting.on_header_value = parser_on_header_value;
    sParserSetting.on_headers_complete = parser_on_header_complete;
    sParserSetting.on_message_complete = parser_on_complete;
}

WSGIApplication::WSGIApplication() :
    SERVER_NAME(Server::host()),
    SERVER_PORT(Server::port()),
    contentLength_(0),
    SERVER_CLOSED(false)
{
    // for headers
    HEADERS.reserve(16);

    // for libuv
    client_ = new WSGIClient();
    client_->tcp->data = this;

    // for http_parser
    client_->parser.data = (void*)this;
    http_parser_init(&(client_->parser), HTTP_REQUEST);
}


WSGIApplication::~WSGIApplication()
{
    uv_close((uv_handle_t*)client_->tcp, http_on_close);
    delete client_;
}

void
WSGIApplication::append(const char * buffer, size_t nread)
{
    rbuffer_.insert(rbuffer_.end(), buffer, buffer + nread);
}


bool
WSGIApplication::parse()
{
    const char * data = rbuffer_.data();
    size_t len = rbuffer_.size();
    if (len > 0)
    {
        size_t parsed = http_parser_execute(&(client_->parser), &sParserSetting, data, len);
        return parsed == len && client_->parser.http_errno == HPE_OK;
    }
    return false;
}


void
WSGIApplication::execute()
{
    sWSGIHandler(*this);
    rbuffer_.clear();
}


void
WSGIApplication::raiseUp(int statusCode)
{
    switch (statusCode)
    {
    case 400:
        // with out `\0` in HTTPResponse buffer
        this->send(kWSGIStatus_400, sizeof(kWSGIStatus_400) - 1);
        break;
    case 404:
        this->send(kWSGIStatus_404, sizeof(kWSGIStatus_404) - 1);
        break;
    case 500:
        this->send(kWSGIStatus_500, sizeof(kWSGIStatus_500) - 1);
        break;
    default:
        this->send("\r\n\r\n", 4);
        break;
    }
}

void *
WSGIApplication::connection()
{
    return client_->tcp;
}


WSGIClient*
WSGIApplication::client() const
{
    return client_;
}


bool
WSGIApplication::keepalive() const
{
    auto client = _CLIENT(this);
    bool shouldKeepalive = http_should_keep_alive(&client->parser) != 0;
    bool hasKeepalive = (client->parser.flags & F_CONNECTION_KEEP_ALIVE) != 0;

    if (shouldKeepalive && hasKeepalive)
    {
        // we know content-length, we can keepalive
        if (this->contentLength() > 0)
        {
            return true;
        }
        else
        {
            // If `Content-Length` is not specified, HTTP 1.1 is default to keep-alive
            // HTTP 1.0 always close
            return (client->parser.http_major > 0 && client->parser.http_minor > 0);
        }
    }
    else
    {
        // Connection: close on HTTP 1.1 or Missing Connection: keep-alive on HTTP 1.0
        return false;
    }
}


void
WSGIApplication::send(const char * data, size_t sz)
{
    size_t remain = sz;
    size_t send_size;

    do
    {
        send_size = remain > sWriteBuffer.size() ? sWriteBuffer.size() : remain;
        remain -= send_size;

        uv_write_t * w = new uv_write_t;
        w->data = this;

        sWriteBuffer.copy(data, send_size);
        uv_buf_t buf = uv_buf_init(sWriteBuffer.data(), send_size);
        uv_write(w, (uv_stream_t*)this->connection(), &buf, 1, http_on_write);
    } while (remain > 0);
}


void
WSGIApplication::sendFile(int file_fd, size_t sz)
{
    auto conn = _CONNECTION(this);

#if !defined(WIN32) && !defined(_WIN32)
    int sockfd = conn->io_watcher.fd;
#else
    int sockfd = conn->socket;
#endif

    size_t remain = sz;
    size_t send_size;
    int offset = 0;

    do
    {
        send_size = remain > kSendFileBuffer ? kSendFileBuffer : remain;
        remain -= send_size;

        uv_fs_t * fs = new uv_fs_t;
        fs->data = this;

        uv_fs_sendfile(uv_default_loop(), fs, sockfd, file_fd, offset, kSendFileBuffer, fs_on_sendfile);
        offset += send_size;
    } while (remain > 0);
}


NS_END
