#ifndef HYDRUS_HTTP_SERVER_H
#define HYDRUS_HTTP_SERVER_H

namespace hydrus
{
    class Request;
    class Response;
    typedef void(Request*, Response*) WSGICallback;

    class HttpServer
    {
    private:
        char address_[40];
        int port_;
        // disallow copy a instance
        HttpServer(const HttpServer & other) {}
    private:
        HttpServer(const char * addr, int port);
        void run(WSGICallback app);

        const char * address() const { return &address_[0]; }
        int port() const { return port_; }
    };
}

#endif
