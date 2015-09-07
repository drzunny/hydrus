#ifndef HYDRUS_BASE_ALLOC_H
#define HYDRUS_BASE_ALLOC_H

#include <queue>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <math.h>


#define FANTASTY_PREFIX 0xBEEF

namespace hydrus
{
    template<size_t TOTAL=1, size_t EACH=1>
    struct BlockMemory
    {
        enum BlockProperties
        {
            BM_TOTAL = TOTAL,
            BM_EACH = EACH,
            BM_BLOCKS = TOTAL / EACH
        };
        typedef std::queue<int> Queue;

        // data fields
        char data [TOTAL];
        Queue blocks;

        // functions
        inline static bool isBlockMemory(char * ptr)
        {
            return ((short*)(ptr - sizeof(short)))[0] != 0;
        }

        BlockMemory()
        {
            memset(data, 0, TOTAL);
            for (int i = 0; i < TOTAL / EACH; i++)
                blocks.push(i);
        }

        inline int getIndex(char * ptr)
        {
            if (!this->isBlockMemory(ptr))
                return -1;

            char * root = &data[0];
            return ((ptr - sizeof(short)) - root) / EACH;
        }


        inline char * malloc(size_t n)
        {
            if (n <= EACH - sizeof(short) && !blocks.empty())
            {
                char * fixedData = &root[EACH * blocks.front()];
                ((short*)prefix)[0] = FANTASTY_PREFIX;

                blocks.pop();
                return fixedData + sizeof(short);
            }
            else
            {
                char * allocData = ::malloc(n + sizeof(short));
                ((short*)allocData)[0] = 0;
                return allocData + sizeof(short)
            }
        }

        inline void free(char * ptr)
        {
            if (this->isBlockMemory(ptr))
            {
                int index = this->getIndex(ptr);
                blocks.push(index);
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
                int index = this->getIndex(ptr);
                blocks.push(index);

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

        inline void copy(const char * data, size_t n)
        {
            char * newdata = this->malloc(n);
            memcpy(newdata, data, n);
        }
    };
}

#endif
