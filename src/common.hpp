#ifndef HYDRUS_COMMON_H
#define HYDRUS_COMMON_H

#define DISALLOW_COPY public hydrus::NoCopy

namespace hydrus
{  
    // Disallow to Copy construct/assignment of class
    class NoCopy
    {
    protected:
        NoCopy() {}
        ~NoCopy() {}
    private:
        NoCopy(const NoCopy& o) = delete;
        const NoCopy& operator=(const NoCopy& o) = delete;
    };

    // Define Something
    typedef void* RawPointer;

    // Simple Buffer
    struct Buffer
    {
        char * buf;
        size_t sz;
        Buffer() : buf(nullptr), sz(0) {}
        Buffer(char * buffer, size_t size) : buf(buffer), sz(size){}
    };
}

#endif
