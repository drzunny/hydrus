#ifndef HYDRUS_HTTP_RESPONSE_H
#define HYDRUS_HTTP_RESPONSE_H

namespace hydrus
{
    class Response
    {
    private:
        int code_;
        int contentType_;
        int mimeType_;
        int body_;
    public:
        Response();
        void toString() const;
    };
}

#endif
