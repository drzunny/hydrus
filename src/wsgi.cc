#include "wsgi.h"
#include "server.h"
#include "base/definition.hpp"

#include <uv.h>
#include "http_parser.h"

#include <cassert>

#define _WSGI(p) ((hydrus::WSGIApplication*)(p->data))
#define _CLIENT(p) ((hydrus::WSGIClient*)(p->client()))


// ----------------------------------
//  Raise Output
// ----------------------------------

static const char kWSGIStatus_400[] = { "HTTP/1.1 400 Bad Request\r\n\r\n" };
static const char kWSGIStatus_404[] = { "HTTP/1.1 404 Not Found\r\n\r\n" };
static const char kWSGIStatus_500[] = { "HTTP/1.1 500 Internal Server Error\r\n\r\n" };


// -----------------------------------
//  helper functions
// -----------------------------------
static hydrus::WSGICallback s_wsgi_callback = nullptr;
static http_parser_settings s_parser_settings;

static const char * kHydrusServerName       = "hydrus";


struct hydrus::WSGIClient
{
    uv_tcp_t        tcp;
    http_parser     parser;
    bool            openning;
    RefBuf          tName;
    RefBuf          tValue;

    WSGIClient() :openning(false) {}
};


// -----------------------------------
//  UV callbacks
// -----------------------------------

// static void
// http_on_close(uv_handle_t * handle)
// {
//    auto wsgi = _WSGI(handle);
//    delete wsgi;
// }


static void
http_on_write(uv_write_t* req, int status)
{
    free(req);
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
    wsgi->URL = { at, length };
    return 0;
}


static int
parser_on_body(http_parser* pa, const char *at, size_t length)
{
    auto wsgi = _WSGI(pa);
    wsgi->BODY = { at, length };
    return 0;
}


static int
parser_on_header_field(http_parser* pa, const char *at, size_t length)
{
    auto wsgi = _WSGI(pa);
    auto client = _CLIENT(wsgi);

    client->openning = true;
    client->tName = { at, length };

    return 0;
}


static int
parser_on_header_value(http_parser* pa, const char *at, size_t length)
{
    auto wsgi = _WSGI(pa);
    auto client = _CLIENT(wsgi);

    if (!client->openning)
        return 0;

    client->tValue = { at, length };
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
    auto wsgi = _WSGI(pa);

    wsgi->REQUEST_METHOD = http_method_str((http_method)pa->method);
    wsgi->CONTENT_LENGTH = pa->content_length;

    return 0;
}


// -----------------------------------
//  WSGIApplication impletation
// -----------------------------------
NS(hydrus)

void
WSGIReady(WSGICallback callback)
{
    if (s_wsgi_callback)
        return;

    // setup callback
    s_wsgi_callback = callback;

    // setup parser callback
    s_parser_settings.on_message_begin = parser_on_begin;
    s_parser_settings.on_url = parser_on_url;
    s_parser_settings.on_body = parser_on_body;
    s_parser_settings.on_header_field = parser_on_header_field;
    s_parser_settings.on_header_value = parser_on_header_value;
    s_parser_settings.on_headers_complete = parser_on_header_complete;
    s_parser_settings.on_message_complete = parser_on_complete;
}

WSGIApplication::WSGIApplication():    
    client_(new WSGIClient),
    SERVER_NAME(Server::host()),
    SERVER_PORT(Server::port())
{
    // for headers
    HEADERS.reserve(16);

    // for libuv
    client_->tcp.data = this;
    uv_tcp_init(uv_default_loop(), &(client_->tcp));

    // for http_parser
    client_->parser.data = (void*)this;
    http_parser_init(&(client_->parser), HTTP_REQUEST);
}


WSGIApplication::~WSGIApplication()
{
    uv_close((uv_handle_t*)&client_->tcp, nullptr);
    delete client_;
}

void
WSGIApplication::append(const char * buffer, size_t nread)
{
    rbuffer_.append(buffer, nread);
}


bool
WSGIApplication::parse()
{
    return http_parser_execute(&client_->parser, &s_parser_settings, rbuffer_.data(), rbuffer_.length()) == 0;
}


void
WSGIApplication::execute()
{
    s_wsgi_callback(*this);
}


void
WSGIApplication::raiseUp(int statusCode)
{
    switch (statusCode)
    {
    case 400:
        this->send(kWSGIStatus_400, sizeof(kWSGIStatus_400));
        break;
    case 404:
        this->send(kWSGIStatus_404, sizeof(kWSGIStatus_404));
        break;
    case 500:
        this->send(kWSGIStatus_500, sizeof(kWSGIStatus_500));
        break;
    default:
        this->send("\r\n\r\n", 5);
        break;
    }
}

void *
WSGIApplication::client()
{
    return &(client_->tcp);
}


void
WSGIApplication::send(const char * data, size_t sz)
{
    uv_write_t * w = new uv_write_t;
    w->data = this;

    wbuffer_.clear();
    wbuffer_.append(data, sz);

    uv_buf_t buf = uv_buf_init(wbuffer_.data(), sz);
    uv_write(w, (uv_stream_t*)this->client(), &buf, 1, http_on_write);
}


NS_END
