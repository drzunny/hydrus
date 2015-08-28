#ifndef HYDRUS_HTTP_REQUEST_H
#define HYDRUS_HTTP_REQUEST_H

#include <cstdint>
#include <memory>
#include "common.hpp"

namespace hydrus
{
    class Request: DISALLOW_COPY
    {
    public:
        class RequestImpl;
    private:
        std::shared_ptr<RequestImpl> impl_;

    public:
        Request();
        Request(const hydrus::Buffer & buf);
        Request(Request && r);

        bool            parse(const char * buffer, size_t sz);
        const char *    get(const char * name) const;

        static void ready();
    };
}

#endif
