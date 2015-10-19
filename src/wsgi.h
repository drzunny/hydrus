#ifndef HYDRUS_WSGI_H
#define HYDRUS_WSGI_H

#include <string>
#include <vector>
#include <cstdint>
#include "base/nocopy.hpp"

namespace hydrus
{
    // Declare the core WSGI Application
    struct WSGIClient;
    struct WSGIHeader
    {
        std::string name;
        std::string value;
    };

    class WSGIApplication : DISALLOW_COPY
    {
    public:
        typedef std::string Str;
        WSGIApplication();
        ~WSGIApplication();

        void         send(const char * data, size_t sz);
        void         sendFile(int file_fd, size_t sz);
        void         append(const char * buf, size_t nread);
        bool         parse();
        void         execute();
        void         raiseUp(int statusCode);
        WSGIClient * client() const;
        void *       connection();
        bool         keepalive() const;

        inline bool     hasBuffer() const { return !rbuffer_.empty(); }

        // Properties
        // ---------------------------------

        /* For 1.1 */
        bool                    SERVER_CLOSED;

        /* HTTP */
        uint64_t                CONTENT_LENGTH;
        Str                     CONTENT_TYPE;
        Str                     SERVER_SOFTWARE;
        Str                     SERVER_NAME;
        int                     SERVER_PORT;
        Str                     REQUEST_METHOD;
        Str                     REMOTE_ADDR;
        Str                     URL;
        Str                     BODY;
        std::vector<WSGIHeader> HEADERS;

    private:
        WSGIClient*         client_;
        std::vector<char>   rbuffer_;
    };

    // Declare the WSGI Callback
    typedef void(*WSGICallback)(WSGIApplication&);
    void WSGIReady(WSGICallback callback);
}

#endif
