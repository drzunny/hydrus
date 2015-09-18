#ifndef HYDRUS_BASE_NOCOPY_H
#define HYDRUS_BASE_NOCOPY_H

namespace hydrus
{
    class NoCopy
    {
    protected:
        NoCopy() {}
        ~NoCopy() {}
    private:
        NoCopy(const NoCopy& o) = delete;
        const NoCopy& operator=(const NoCopy& o) = delete;
    };
}

#define DISALLOW_COPY public NoCopy

#endif
