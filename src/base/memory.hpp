#ifndef HYDRUS_BASE_MEMORY_H
#define HYDRUS_BASE_MEMORY_H

#include "nocopy.hpp"

#include <assert.h>
#include <memory.h>

namespace hydrus
{
    template<int MAXSIZE>
    class FixedMemoryPool: DISALLOW_COPY
    {
    private:
        char fixedData_[MAXSIZE];
        size_t used_;

    public:
        FixedMemoryPool(): used_(0) {}

        inline size_t used() const
        { return used_; }
        inline size_t size() const
        { return MAXSIZE; }
        char * data()
        { return fixedData_; }
        void clear()
        { used_ = 0; }
        void copy(const char * data, size_t len)
        {
            assert(len <= MAXSIZE);
            memcpy(fixedData_, data, len);
            used_ = len;
        }
        bool append(const char * data, size_t len)
        {
            if (used_ + len > MAXSIZE) return false;
            memcpy(fixedData_ + used_, data, len);
            used_ += len;
        }
    };
}


#endif
