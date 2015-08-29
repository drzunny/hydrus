#ifndef HYDRUS_COMMON_H
#define HYDRUS_COMMON_H

#include <cstdlib>
#include <memory.h>

#define DISALLOW_COPY public hydrus::NoCopy

namespace hydrus
{
    //   Disallow to Copy construct/assignment of class
    // -----------------------------------------------------
    class NoCopy
    {
    protected:
        NoCopy() {}
        ~NoCopy() {}
    private:
        NoCopy(const NoCopy& o) = delete;
        const NoCopy& operator=(const NoCopy& o) = delete;
    };

    //  Define Something
    // -------------------------------
    union Share32 {
        char   sbuf[32];
        char * lbuf;
    };

    //  Simple Buffer
    // ------------------------------------
    struct Buffer: public NoCopy
    {
        Share32 base;
        size_t len;
        Buffer(): len(0) {}
        Buffer(Buffer && rv): len(rv.len), base(rv.base) {}
        Buffer(const char * buf, size_t n): len(n)
        {
            if (n < 32)    {
                base = *(Share32*)buf;
                base.sbuf[n] = '\0';
            }   else    {
                base.lbuf = new char[n+1];
                memcpy(base.lbuf, buf, n);
                base.lbuf[n] = '\0';
            }
        }
        ~Buffer()
        {
            if (len >= 32)
                free(base.lbuf);
        }

        Buffer & operator=(Buffer && b)
        {
            base = b.base;
            len = b.len;
            return *this;
        }

        inline char * data() { return len < 32 ? &base.sbuf[0] : base.lbuf; }
    };
}

#endif
