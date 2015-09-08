#ifndef HYDRUS_BASE_BQUEUE_H
#define HYDRUS_BASE_BQUEUE_H

#include <stdint.h>

namespace hydrus
{
    template<int BLOCKS>
    struct BlockQueue
    {
        typedef size_t ValueType;
        // data
        static const int capacity = BLOCKS + 1;
        ValueType content[BLOCKS+1];
        ValueType cursor;
        
        // methods
        BlockQueue() :cursor(BLOCKS) 
        {
            for (ValueType i = 0; i <= BLOCKS; i++)
                content[i] = i;
        }
        inline bool empty()
        {
            return cursor == 0;
        }

        inline void push(ValueType val)
        {
            // if cursor == capacity - 1, it's a black hole position
            content[cursor++] = val;
            if (cursor >= capacity)
                cursor = capacity - 1;
        }

        inline ValueType pop()
        {            
            return cursor > 0 ? content[--cursor] : 0;
        }
    };
}

#endif
