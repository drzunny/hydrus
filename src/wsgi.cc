#include "wsgi.h"
#include "server.h"
#include "base/definition.hpp"

#include <uv.h>
#include "http_parser.h"

#include <algorithm>
#include <cassert>
#include <iostream>
using namespace std;

#define _WSGI(p) ((hydrus::WSGIApplication*)(p->data))
#define _CLIENT(p) ((p->client()))


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

static const char * kHydrusServerName = "hydrus";


struct hydrus::WSGIClient
{
    uv_tcp_t*       tcp;
    http_parser     parser;
    bool            openning;
    std::string     tName;
    std::string     tValue;

    WSGIClient() :openning(false), tcp(new uv_tcp_t) {}
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



// -----------------------------------
//  request parser
// -----------------------------------
static int
parser_on_begin(http_parser * pa)
{
    //cout << "parser_on_begin" << endl;
    //cout << "PA:CONTENT_LENGTH : " << pa->content_length << endl;
    return 0;
}


static int
parser_on_url(http_parser* pa, const char *at, size_t length)
{
    //cout << "parser_on_url" << endl;
    //cout << "PA:CONTENT_LENGTH : " << pa->content_length << endl;
    auto wsgi = _WSGI(pa);
    wsgi->URL = string(at, length);
    return 0;
}


static int
parser_on_body(http_parser* pa, const char *at, size_t length)
{
    //cout << "parser_on_body" << endl;
    //cout << "PA:CONTENT_LENGTH : " << pa->content_length << endl;
    auto wsgi = _WSGI(pa);
    wsgi->BODY = string(at, length);
    return 0;
}


static int
parser_on_header_field(http_parser* pa, const char *at, size_t length)
{
    //cout << "parser_on_field" << endl;
    //cout << "PA:CONTENT_LENGTH : " << pa->content_length << endl;
    auto wsgi = _WSGI(pa);
    auto client = _CLIENT(wsgi);

    client->openning = true;
    client->tName = string(at, length);

    return 0;
}


static int
parser_on_header_value(http_parser* pa, const char *at, size_t length)
{
    //cout << "parser_on_value" << endl;
    //cout << "PA:CONTENT_LENGTH : " << pa->content_length << endl;
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
    //cout << "parser_on_header end" << endl;
    //cout << "PA:CONTENT_LENGTH : " << pa->content_length << endl;
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
    //cout << "parser_on_complete" << endl;
    //cout << "PA:CONTENT_LENGTH : " << pa->content_length << endl;
    static char s_addr_buf[64];
    auto wsgi = _WSGI(pa);
    auto client = _CLIENT(wsgi);

    sockaddr_in ip;
    int len;
    uv_tcp_getpeername(client->tcp, (sockaddr*)&ip, &len);
    uv_ip4_name(&ip, s_addr_buf, len);

    wsgi->REMOTE_ADDR = s_addr_buf;
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

WSGIApplication::WSGIApplication() :
client_(new WSGIClient),
SERVER_NAME(Server::host()),
SERVER_PORT(Server::port())
{
    // for headers
    HEADERS.reserve(16);

    // for libuv
    client_->tcp->data = this;
    uv_tcp_init(uv_default_loop(), client_->tcp);

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
    //printf("Going to parse, %p(%d)\n%s\n", this, len, string(rbuffer_.data(), rbuffer_.size()));
    if (len > 0)
    {
        size_t parsed = http_parser_execute(&(client_->parser), &s_parser_settings, data, len);
        return parsed == len && client_->parser.http_errno == HPE_OK;
    }
    return false;
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
WSGIApplication::raw_client()
{
    return client_->tcp;
}


WSGIClient*
WSGIApplication::client()
{
    return client_;
}


void
WSGIApplication::send(const char * data, size_t sz)
{
    static const size_t MAX_SEND = 1024 * 1024;
    static char SENDING[MAX_SEND];

    size_t send_size = sz > MAX_SEND ? MAX_SEND : sz;
    size_t remain = sz - send_size;

    uv_write_t * w = new uv_write_t;
    w->data = this;

    memcpy(SENDING, data, send_size);
    uv_buf_t buf = uv_buf_init(SENDING, send_size);
    uv_write(w, (uv_stream_t*)this->raw_client(), &buf, 1, http_on_write);
    if (remain > 0)
    {
        this->send(data + send_size, remain);
    }
}


NS_END
