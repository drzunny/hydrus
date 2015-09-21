#ifndef HYDRUS_SERVER_H
#define HYDRUS_SERVER_H

#include "base/nocopy.hpp"
#include "base/definition.hpp"

namespace hydrus
{
    namespace Server
    {
        enum ServerStatus
        {
            INUSED = -1,
            INIT       = 0,
            READY      = 1,
            RUNNING    = 2,
        };
        
        const char * host();
        int          port();
        ServerStatus listen(const char * address, int port);
        void run();
    };
}

#endif