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
        bool         parse(const char * data, size_t len);
        void         execute();
        void         raiseUp(int statusCode);
        WSGIClient * client() const;
        void *       connection();
        bool         keepalive() const;

        inline bool  finished() const { return finished_; }
        inline void  setFinished(bool on) { finished_ = on; }

        // Properties
        // ---------------------------------

        /* For 1.1 */
        bool                    SERVER_CLOSED;

        /* HTTP */
        uint64_t                CONTENT_LENGTH;
        Str                     CONTENT_TYPE;
        Str                     SERVER_NAME;
        int                     SERVER_PORT;
        Str                     REQUEST_METHOD;
        Str                     REMOTE_ADDR;
        Str                     URL;
        std::vector<char>       BODY;
        std::vector<WSGIHeader> HEADERS;

    private:
        WSGIClient*         client_;
        bool                finished_;
    };

    // Declare the WSGI Callback
    typedef void(*WSGICallback)(WSGIApplication&);
    void WSGIReady(WSGICallback callback);
}

#endif
