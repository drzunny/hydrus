#ifndef HYDRUS_HTTP_SERVER_H
#define HYDRUS_HTTP_SERVER_H

#include "common.hpp"

namespace hydrus
{
    class HttpServer: DISALLOW_COPY
    {
    private:
        char address_[40];
        int port_;
        // disallow copy a instance
        HttpServer(const HttpServer & other) {}
    private:
        HttpServer(const char * addr, int port);
        void run();
    };
}

#endif
