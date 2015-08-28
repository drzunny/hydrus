#include "request.h"
#include "http_parser.h"

#include <string>
#include <unordered_map>

using namespace std;


static http_parser_settings rq_setting;


// ------------------------------------------------
//  helpers
// ------------------------------------------------




// ------------------------------------------------
//  Request Implementation class
// ------------------------------------------------
class hydrus::Request::RequestImpl
{
private:
    bool already_parsed_;
    http_parser parser_;
    size_t headerIter_;
    unordered_map<string, string> fields_;

public:
    RequestImpl() :already_parsed_(false), headerIter_(0) {}
    RequestImpl(const hydrus::Buffer & buf) :already_parsed_(false), headerIter_(0)
    {
        if (this->parse(buf.buf, buf.sz))
            already_parsed_ = true;
    }


    bool parse(const char * buffer, size_t sz)
    {
        http_parser_init(&parser_, HTTP_REQUEST);        
        parser_.data = this;

        size_t nread = http_parser_execute(&parser_, &rq_setting, buffer, sz);
        return !(nread < sz);
    }


    void set(const char * name, const char * value)
    {
        fields_.insert(make_pair(name, value));
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
    r->set("url", string(at, n).c_str());

    return 0;
}


static int
_parser_on_headfield(http_parser * pa, const char * at, size_t n) {
    REQIMPL * r = (REQIMPL*)pa->data;    
}


static int
_parser_on_headvalue(http_parser * pa, const char * at, size_t n) {
    REQIMPL * r = (REQIMPL*)pa->data;
}


static int
_parser_on_headerdone(http_parser * pa) {
    REQIMPL * r = (REQIMPL*)pa->data;
}


static int
_parser_on_body(http_parser * pa, const char * at, size_t n) {
    REQIMPL * r = (REQIMPL*)pa->data;
}


static int
_parser_on_msgdone(http_parser * pa)   {
    REQIMPL * r = (REQIMPL*)pa->data;
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


hydrus::Request::Request(const hydrus::Buffer & buf) : impl_(make_shared<hydrus::Request::RequestImpl>(buf))
{
}


hydrus::Request::Request(hydrus::Request && r) : impl_(std::move(r.impl_))
{
}


bool 
hydrus::Request::parse(const char * buffer, size_t sz)
{
    return impl_->parse(buffer, sz);
}

const char *
hydrus::Request::get(const char * name) const
{
    return impl_->get(name);
}


void
hydrus::Request::ready()
{
    _parser_ready();
}