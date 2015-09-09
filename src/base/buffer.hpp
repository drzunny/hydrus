#ifndef HYDRUS_BASE_BUFFER_H
#define HYDRUS_BASE_BUFFER_H

#include "alloc.hpp"
#include "nocopy.hpp"

namespace hydrus
{
    typedef BlockMemory<512 * 1024, 4096> BlockAllocator;
    static BlockAllocator BufferAlloc_;

    //  This is a Buffer wrapper for BlockMemory's data
    // -----------------------------------------------
    class Buffer: DISALLOW_COPY
    {
    private:
        char * base_;
        size_t len_;

    public:
        Buffer() : len_(0) {}
        Buffer(Buffer && rv) : len_(rv.len_), base_(rv.base_) {}
        Buffer(const char * buf, size_t n) : len_(n)
        {
            base_ = BufferAlloc_.malloc(n);
        }
        ~Buffer()
        {
            BufferAlloc_.free(base_);
        }

        Buffer & operator=(Buffer && b)
        {
            base_ = b.base_;
            len_ = b.len_;
            return *this;
        }

        inline char * data() 
        {
            return base_;
        }

        inline size_t length() const
        {
            return len_;
        }
    };
}

#endif
