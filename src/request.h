#ifndef HYDRUS_HTTP_REQUEST_H
#define HYDRUS_HTTP_REQUEST_H

#include <cstdint>
#include <memory>
#include "base/nocopy.hpp"
#include "base/buffer.hpp"

namespace hydrus
{
    class Request: DISALLOW_COPY
    {
    public:
        class RequestImpl;
        static void ready();
        static BlockAllocator allocator;

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
