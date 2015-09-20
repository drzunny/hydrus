#include "wsgi.h"
#include "base/definition.hpp"

#include <uv.h>
#include "http_parser.h"

#include <cassert>

#define _WSGI(p) ((hydrus::WSGIApplication*)(p->data))

// -----------------------------------
//  helper functions
// -----------------------------------
static hydrus::WSGICallback s_wsgi_callback = nullptr;
static http_parser_settings s_parser_settings;

static const char * kHydrusServerName   = "hydrus";
static const char * kHydrusUrlScheme    = "http";


struct hydrus::WSGIClient
{
    uv_tcp_t        tcp;
    http_parser     parser;
};


// -----------------------------------
//  UV callbacks
// -----------------------------------
static void
http_on_close(uv_handle_t * handle)
{
    auto wsgi = _WSGI(handle);
    delete wsgi;
}


static void
http_on_write(uv_stream_t * stream, int status)
{
    auto wsgi = (hydrus::WSGIApplication*)(stream->data);
    delete wsgi;
}


// -----------------------------------
//  request parser
// -----------------------------------
static int
parser_on_begin(http_parser * pa)
{}


static int
parser_on_url(http_parser* pa, const char *at, size_t length)
{}


static int
parser_on_body(http_parser* pa, const char *at, size_t length)
{}


static int
parser_on_header_field(http_parser* pa, const char *at, size_t length)
{}


static int
parser_on_header_value(http_parser* pa, const char *at, size_t length)
{}


static int
parser_on_header_complete(http_parser* pa, const char *at, size_t length)
{}


static int
parser_on_complete(http_parser* pa)
{    
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
    s_parser_settings.on_body = nullptr;
}

WSGIApplication::WSGIApplication():
    client_(new WSGIClient),
    WSGI_URL_SCHEME(kHydrusUrlScheme)
{
    // for libuv
    client_->tcp.data = this;
    uv_tcp_init(uv_default_loop(), &(client_->tcp));

    // for http_parser
    http_parser_init(&client_->parser, HTTP_REQUEST);
}

void
WSGIApplication::append(const char * buffer, size_t nread)
{
}


bool
WSGIApplication::parse()
{
    return http_parser_execute(&client_->parser, &s_parser_settings, nullptr, 0) > 0;
}


void
WSGIApplication::execute()
{
    s_wsgi_callback(*this);
}


void
WSGIApplication::raise(int statusCode)
{
}

void *
WSGIApplication::client()
{
    return &(client_->tcp);
}

NS_END
