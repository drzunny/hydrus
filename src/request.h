#ifndef HYDRUS_HTTP_REQUEST_H
#define HYDRUS_HTTP_REQUEST_H

#include <cstdint>
#include <string>
#include <vector>
#include "base/nocopy.hpp"

namespace hydrus
{
    struct Headers
    {
        std::string name;
        std::string value;
    };


    class Request: DISALLOW_COPY
    {
    public:
        class RequestImpl;
        static bool parserReady;

    private:
        typedef std::string Str;
        typedef std::vector<Headers> HeadersList;

        RequestImpl* impl_;
        bool already_parsed_;

    public:
        Request(const char * buffer, size_t nread);
        ~Request();

        // Check
        bool isParsed() const;

        // Request fields
        Str         url;
        Str         method;
        size_t      status_code; 
        HeadersList headers;
        char *      body;
        bool        keepalive;
        
    };
}

#endif
