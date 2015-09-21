#ifndef HYDRUS_WSGI_H
#define HYDRUS_WSGI_H

#include <vector>
#include <cstdint>
#include "base/nocopy.hpp"
#include "base/types.hpp"

namespace hydrus
{
    // Declare the core WSGI Application
    struct WSGIClient;
    struct WSGIHeader
    {
        RefBuf name;
        RefBuf value;
    };

    class WSGIApplication : DISALLOW_COPY
    {
    public:
        WSGIApplication();
        ~WSGIApplication();

        void    send(const char * data, size_t sz);
        void    append(const char * buf, size_t nread);
        bool    parse();
        void    execute();
        void    raiseUp(int statusCode);
        void *  client();

        // Properties
        // ---------------------------------

        /* HTTP */
        const char *            SERVER_SOFTWARE;
        const char *            SERVER_NAME;
        int                     SERVER_PORT;
        const char *            REQUEST_METHOD;
        uint64_t                CONTENT_LENGTH;
        const char *            REMOTE_ADDR;
        RefBuf                  URL;
        RefBuf                  BODY;
        std::vector<WSGIHeader> HEADERS;

    private:
        WSGIClient*         client_;
        Buffer<4096>        rbuffer_;
        Buffer<4096>        wbuffer_;
    };

    // Declare the WSGI Callback
    typedef void(*WSGICallback)(WSGIApplication&);
    void WSGIReady(WSGICallback callback);
}

#endif
