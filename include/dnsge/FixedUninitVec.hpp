#pragma once

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>

namespace dnsge {

/**
 * @brief A fixed-length, dynamically allocated vector of T. Elements are not 
 * initialized nor destructed automatically -- users must take care to keep track
 * of which elements need to be destructed.
 * 
 * @tparam T 
 */
template <typename T>
class FixedUninitVec {
public:
    FixedUninitVec(size_t size)
        : size_(size) {
        if (size == 0) {
            this->data_ = nullptr;
            return;
        }

        auto* alloc = static_cast<T*>(::operator new(size * sizeof(T)));
        assert(alloc != nullptr);
        this->data_ = alloc;
    }

    ~FixedUninitVec() {
        ::operator delete(this->data_);
    }

    FixedUninitVec(const FixedUninitVec &other)
        : size_(other.size_) {
        if (this->size_ == 0) {
            this->data_ = nullptr;
            return;
        }
        auto* alloc = static_cast<T*>(::operator new(this->size_ * sizeof(T)));
        assert(alloc != nullptr);
        this->data_ = alloc;
        // Copy data from other vector
        std::memcpy(this->data_, other.data_, this->size_ * sizeof(T));
    }

    FixedUninitVec &operator=(const FixedUninitVec &other) {
        FixedUninitVec temp(other);
        *this = std::move(temp);
        return *this;
    }

    FixedUninitVec(FixedUninitVec &&other) noexcept
        : size_(other.size_)
        , data_(other.data_) {
        other.size_ = 0;
        other.data_ = nullptr;
    }

    FixedUninitVec &operator=(FixedUninitVec &&other) noexcept {
        FixedUninitVec temp(std::move(*this));
        this->size_ = other.size_;
        this->data_ = other.data_;
        other.size_ = 0;
        other.data_ = nullptr;
        return *this;
    }

    T &operator[](size_t n) {
        assert(n < this->size_);
        return *(this->data_ + n);
    }

    const T &operator[](size_t n) const {
        assert(n < this->size_);
        return *(this->data_ + n);
    }

    size_t size() const {
        return this->size_;
    }

    T* begin() {
        return this->data_;
    }

    T* end() {
        return this->data_ + this->size_;
    }

    T* data() {
        return this->data_;
    }

    const T* data() const {
        return this->data_;
    }

private:
    size_t size_;
    T* data_;
};

} // namespace dnsge
