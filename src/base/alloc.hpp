#ifndef HYDRUS_BASE_ALLOC_H
#define HYDRUS_BASE_ALLOC_H

#include "./bqueue.hpp"
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>


#define FANTASTY_PREFIX (0xbeef)

namespace hydrus
{
    template<size_t TOTAL=1, size_t EACH=1>
    class BlockMemory
    {
    public:
        enum BlockProperties
        {
            BM_TOTAL = TOTAL,
            BM_EACH = EACH,
            BM_BLOCKS = TOTAL / EACH
        };
        typedef BlockQueue<TOTAL/EACH> Queue;

    private:
        char data_[TOTAL];
        Queue blocks_;

        inline int _getIndex(char * ptr)
        {
            return ((ptr - sizeof(short)) - &data_[0]) / EACH;
        }

    public:
        BlockMemory() {}
        inline static bool isBlockMemory(char * ptr)
        {
            return ((short*)(ptr - sizeof(short)))[0] != 0;
        }       
        
        inline char * malloc(size_t n)
        {
            char * allocData = nullptr;
            if (n <= EACH - sizeof(short) && !blocks_.empty())
            {
                allocData = &data_[EACH * blocks_.pop()];
                ((short*)allocData)[0] = FANTASTY_PREFIX;
            }
            else
            {
                allocData = (char*)::malloc(n + sizeof(short));
                ((short*)allocData)[0] = 0;
            }
            return allocData + sizeof(short);
        }

        inline void free(char * ptr)
        {
            if (this->isBlockMemory(ptr))
            {
                int index = this->_getIndex(ptr);
                blocks_.push(index);
            }
            else
            {
                char * origin = ptr - sizeof(short);
                ::free(origin);
            }
        }

        inline char * realloc(char * ptr, size_t n)
        {
            if (this->isBlockMemory(ptr))
            {
                if (n <= EACH - sizeof(short))
                    return ptr;
                int index = this->_getIndex(ptr);
                blocks_.push(index);

                char * newdata = this->malloc(n + sizeof(short));
                ((short*)newdata)[0] = 0;
                memcpy(newdata + sizeof(short), ptr, EACH - sizeof(short));
                return newdata;
            }
            else
            {
                return ::realloc(ptr, n);
            }
        }

        inline void copy(const char * buf, size_t n)
        {
            char * newdata = this->malloc(n);
            memcpy(newdata, buf, n);
        }
    };
}

#endif
