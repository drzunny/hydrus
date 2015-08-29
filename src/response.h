#ifndef HYDRUS_HTTP_RESPONSE_H
#define HYDRUS_HTTP_RESPONSE_H

#include "request.h"
#include "base.hpp"

namespace hydrus
{
    class Response: DISALLOW_COPY
    {
    private:
        Request req_;
        bool    applied_or_raised_;

    public:
        Response(Request && req);
        ~Response();

        void            apply();
        Buffer          data();

        static Buffer   raise(int statusCode);
        static bool     isFixedBuffer(const char * p);
    };
}

#endif
