#ifndef HYDRUS_HTTP_REQUEST_H
#define HYDRUS_HTTP_REQUEST_H

#include <cstdint>
#include <memory>
#include "base.hpp"

namespace hydrus
{
    class Request: DISALLOW_COPY
    {
    public:
        class RequestImpl;
        static void ready();
    private:
        std::shared_ptr<RequestImpl> impl_;

    public:
        Request();
        Request(hydrus::Buffer && buf);
        Request(Request && r);
        
        bool parse(hydrus::Buffer && buf);
    };
}

#endif
