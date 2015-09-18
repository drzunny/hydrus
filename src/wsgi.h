#ifndef HYDRUS_WSGI_H
#define HYDRUS_WSGI_H

#include "base/nocopy.hpp"

namespace hydrus
{
    typedef void * WSGIClient;

    class WSGIApplication : DISALLOW_COPY
    {
    public:
        WSGIApplication(WSGIClient client) {}
        void append(const char * buf, size_t nread) {}
        bool parse() { return false; }
        void execute() {  }
        void raise(int statusCode) {}

    private:
        WSGIClient      client_;
        const char *    buffer_;
        size_t          nread_;
    };
}

#endif