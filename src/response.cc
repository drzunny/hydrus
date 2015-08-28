#include "response.h"

#include <memory>

using namespace std;

#define HTTP_RESPONSE_200
#define HTTP_RESPONSE_400
#define HTTP_RESPONSE_404
#define HTTP_RESPONSE_500

#define FIXED_SIZE 512*1024

static char hy_fixed_buffer[FIXED_SIZE];


// ----------------------------------------
//  Helpers
// ----------------------------------------



// ----------------------------------------
//  Response pimpl class
// ----------------------------------------



// ----------------------------------------
//  Response class implementation
// ----------------------------------------
hydrus::Response::Response(hydrus::Request && req) : req_(move(req))
{
}


hydrus::Response::~Response()
{
}


hydrus::Buffer
hydrus::Response::raise(int statusCode)
{
    return Buffer();
}


void
hydrus::Response::apply()
{
}


hydrus::Buffer
hydrus::Response::data()
{
    if (!applied_or_raised_) 
        return hydrus::Buffer();
    return hydrus::Buffer();
}


bool
hydrus::Response::isFixedBuffer(const char* p)
{
    return p >= &hy_fixed_buffer[0] && p <= &hy_fixed_buffer[FIXED_SIZE - 1];
}