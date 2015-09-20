#ifndef HYDRUS_WSGI_H
#define HYDRUS_WSGI_H

#include "base/nocopy.hpp"

namespace hydrus
{
    // Data structure
    struct RefBuf
    {
        const char * buf;
        size_t length;
    };

    // Declare the core WSGI Application
    struct WSGIClient;
    class WSGIApplication : DISALLOW_COPY
    {
    public:
        WSGIApplication();
        ~WSGIApplication() {}

        void    append(const char * buf, size_t nread);
        bool    parse();
        void    execute();
        void    raise(int statusCode);
        void *  client();

        // Properties
        // ---------------------------------

        /* WSGI */
        const char * WSGI_URL_SCHEME;
        const char * WSGI_VERSION;
        bool         WSGI_MULTIPROCESS;
        bool         WSGI_MULTITHREAD;

        /* HTTP */
        const char * SERVER_SOFTWARE;
        const char * SERVER_NAME;
        const char * SERVER_PORT;
        const char * SERVER_PROTOCOL;
        const char * REQUEST_METHOD;
        const char * PATH_INFO;
        const char * QUERY_STRING;
        const char * CONTENT_LENGTH;
        const char * REMOTE_ADDR;

    private:
        WSGIClient*      client_;
    };

    // Declare the WSGI Callback
    typedef void(*WSGICallback)(WSGIApplication&);
    void WSGIReady(WSGICallback callback);
}

#endif
