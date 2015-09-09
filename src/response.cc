#include "response.h"

#include "base/alloc.hpp"
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
static hydrus::BlockMemory<512 * 1024, 4 * 1024> _memories;



// ----------------------------------------
//  Response class implementation
// ----------------------------------------
hydrus::BlockAllocator hydrus::Response::allocator = hydrus::BlockAllocator();

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