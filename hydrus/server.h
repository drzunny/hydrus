#ifndef HYDRUS_HTTP_SERVER_H
#define HYDRUS_HTTP_SERVER_H

#include "base/nocopy.hpp"
#include "request.h"

#include <cstdint>

namespace hydrus
{
    // Connection
    class Client : DISALLOW_COPY
    {
    public:
        Client(void * hnd);
        ~Client();
        void send(const char * buf, size_t n) const;
        inline const char * remoteAddress() const { return &(remote_address_[0]); }

    private:
        class ClientImpl;
        char remote_address_[48];
        ClientImpl* impl_;
    };


    // The WSGI Callback
    typedef void (*WSGICallback)(const Client& client, const Request& request);


    // WSGI Server
    class Server: DISALLOW_COPY
    {
    public:
        WSGICallback callback;

        Server(WSGICallback cb) : callback(cb) {}
        void listen(const char * host, int port);
        void run();
    };
}

#endif
