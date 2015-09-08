#include "request.h"
#include "http_parser.h"

#include <vector>
#include <string>
#include <unordered_map>

using namespace std;


static http_parser_settings rq_setting;


// ------------------------------------------------
//  helpers
// ------------------------------------------------
struct header_s {
    string field;
    string value;
};

struct request_s {
    string url;
    string method;
    string nh_field;
    string nh_value;
    size_t statusCode;
    bool nh_openning;
    hydrus::Buffer body;

    vector<header_s> headers;

    request_s() : headers(vector<header_s>(16)), nh_openning(false) { headers.reserve(16); }
    ~request_s() { headers.clear(); headers.shrink_to_fit(); }
};



// ------------------------------------------------
//  Request Implementation class
// ------------------------------------------------
class hydrus::Request::RequestImpl
{
private:
    bool already_parsed_;
    http_parser parser_;
    size_t headerIter_;
    request_s req_;

public:
    RequestImpl() :already_parsed_(false), headerIter_(0) {}
    RequestImpl(hydrus::Buffer && buf) :already_parsed_(false), headerIter_(0)
    {
        if (this->parse(move(buf)))
            already_parsed_ = true;
    }


    bool parse(hydrus::Buffer && buf)
    {
        http_parser_init(&parser_, HTTP_REQUEST);        
        parser_.data = this;

        size_t nread = http_parser_execute(&parser_, &rq_setting, buf.data(), buf.length());
        return !(nread < buf.length());
    }


    request_s & raw() {
        return req_;
    }


    const char * get(const char * name) const
    {
        if (!already_parsed_) return nullptr;
    }


    bool isParsed() const
    {
        return already_parsed_;
    }
};



// ------------------------------------------------
//  http-parser functions
// ------------------------------------------------
#define REQIMPL hydrus::Request::RequestImpl

static int
_parser_on_msgbegin(http_parser * pa)   {
    return 0;
}


static int
_parser_on_url(http_parser * pa, const char * at, size_t n) {
    REQIMPL * r = (REQIMPL*)pa->data;
    r->raw().url = at;
    return 0;
}


static int
_parser_on_headfield(http_parser * pa, const char * at, size_t n) {
    REQIMPL * r = (REQIMPL*)pa->data;
    r->raw().nh_field = string(at, n);
    r->raw().nh_openning = true;

    return 0;
}


static int
_parser_on_headvalue(http_parser * pa, const char * at, size_t n) {
    REQIMPL * r = (REQIMPL*)pa->data;
    r->raw().nh_value = string(at, n);
    r->raw().nh_openning = false;

    header_s header = { move(r->raw().nh_field), move(r->raw().nh_value) };
    r->raw().headers.push_back(move(header));

    return 0;
}


static int
_parser_on_headerdone(http_parser * pa) {
    REQIMPL * r = (REQIMPL*)pa->data;
    if (r->raw().nh_openning)
        return -1;

    return 0;
}


static int
_parser_on_body(http_parser * pa, const char * at, size_t n) {
    REQIMPL * r = (REQIMPL*)pa->data;

    if (n == 0) return -1;
    auto & rs = r->raw();
    rs.body = hydrus::Buffer(at, n);

    return 0;
}


static int
_parser_on_msgdone(http_parser * pa)   {
    REQIMPL * r = (REQIMPL*)pa->data;

    r->raw().method = http_method_str((http_method)pa->method);
    r->raw().statusCode = (size_t)pa->status_code;

    return 0;
}


static void
_parser_ready() {
    rq_setting.on_message_begin = _parser_on_msgbegin;
    rq_setting.on_url = _parser_on_url;
    rq_setting.on_header_field = _parser_on_headfield;
    rq_setting.on_header_value = _parser_on_headvalue;
    rq_setting.on_headers_complete = _parser_on_headerdone;
    rq_setting.on_body = _parser_on_body;
    rq_setting.on_message_complete = _parser_on_msgdone;
}



// ------------------------------------------------
//  Request parser handler
// ------------------------------------------------
hydrus::Request::Request() : impl_(make_shared<hydrus::Request::RequestImpl>())
{
}


hydrus::Request::Request(hydrus::Buffer && buf) : impl_(make_shared<hydrus::Request::RequestImpl>(move(buf)))
{
}


hydrus::Request::Request(hydrus::Request && r) : impl_(std::move(r.impl_))
{
}


bool 
hydrus::Request::parse(hydrus::Buffer && buf)
{
    return impl_->parse(move(buf));
}


void
hydrus::Request::ready()
{
    _parser_ready();
}