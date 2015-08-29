#ifndef HYDRUS_HTTP_SERVER_H
#define HYDRUS_HTTP_SERVER_H

#include "base.hpp"
#include <cstdint>
#include <string>

namespace hydrus
{
    typedef void WSGICallback(void*, size_t);

    class HttpServer: DISALLOW_COPY
    {
    private:
        typedef std::string Str;
        HttpServer() {}
    public:
        static HttpServer * createServer();
        static void release();

        void setup(WSGICallback cb);
        void listen(const Str & host, int port);
        void run();
    };
}

#endif
