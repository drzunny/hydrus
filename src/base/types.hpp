#ifndef HYDRYS_BASE_TYPES_H
#define HYDRYS_BASE_TYPES_H

#include "nocopy.hpp"
#include <string>

namespace hydrus
{
    // WSGI Callback

    // Just a reference of other buffer
    // No allocate
    struct RefBuf
    {
        const char * buf;
        size_t n;
    };

    // easy buffer
    template<int SIZE>
    class Buffer: DISALLOW_COPY
    {
    private:
        char        sbuf_[SIZE];
        std::string lbuf_;
        size_t total_;

    public:
        Buffer() : total_(0) {}
        Buffer(Buffer && rh) : sbuf_(rh.sbuf_), lbuf_(rh.lbuf_), total_(rh.total_) {}

        Buffer& operator=(Buffer && rh) {sbuf_ = rh.sbuf_; lbuf_ = move(rh.lbuf_); total_ = rh.total_; }

        void clear()
        {
            total_ = 0;
            lbuf_.clear();
            lbuf_.shrink_to_fit();
        }

        void append(const char * buf, size_t n)
        {
            if (total_ + n <= SIZE)  {
                memcpy(sbuf_ + total_, buf, n);
                total_ += n;
                return;
            }
            else if (total_ <= SIZE)
                lbuf_.append(sbuf_, total_);
            lbuf_.append(buf, n);
            total_ += n;
        }

        char * data() { return total_ <= SIZE ? sbuf_ : &lbuf_.at(0); }   
        size_t length() const { return total_; }
    };
}

#endif 