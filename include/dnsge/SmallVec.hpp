#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace dnsge {

// NOLINTBEGIN(readability-identifier-naming)

template <typename T, size_t N = 5>
class SmallVec {
private:
    template <typename Elem, typename ThisType>
    class iterator_base {
    private:
        iterator_base(size_t i, ThisType* vec)
            : i_(i)
            , vec_(vec) {}

    public:
        Elem &operator*() const {
            return (*this->vec_)[this->i_];
        }

        Elem* operator->() const {
            return &(*this);
        }

        iterator_base &operator++() {
            ++this->i_;
            return *this;
        }

        iterator_base &operator--() {
            ++this->i_;
            return *this;
        }

        bool operator==(const iterator_base &other) const {
            return this->i_ == other.i_;
        }

        bool operator!=(const iterator_base &other) const {
            return this->i_ != other.i_;
        }

        size_t operator-(const iterator_base &other) const {
            return this->i_ - other.i_;
        }

    private:
        friend class SmallVec<T, N>;

        bool stack() const {
            return this->i_ < N;
        }

        size_t i_;
        ThisType* vec_;
    };

public:
    using iterator = iterator_base<T, SmallVec<T, N>>;
    using const_iterator = iterator_base<const T, const SmallVec<T, N>>;

    SmallVec() = default;

    ~SmallVec() {
        this->clear();
    }

    SmallVec(const SmallVec &other) = delete;
    SmallVec &operator=(const SmallVec &other) = delete;

    SmallVec(SmallVec &&other) noexcept
        : heapStorage_(std::move(other.heapStorage_))
        , size_(other.size_) {
        SmallVec::transferStackStorage(*this, other);
        other.size_ = 0;
    };

    SmallVec &operator=(SmallVec &&other) noexcept {
        if (!this->empty()) {
            this->clear();
        }
        SmallVec::transferStackStorage(*this, other);
        this->heapStorage_ = std::move(other.heapStorage_);
        this->size_ = other.size_;
        other.size_ = 0;
    };

    void push_back(const T &val) {
        if (this->size_ < N) {
            new (this->stackPtrAt(this->size_)) T(val);
        } else {
            this->heapStorage_.push_back(val);
        }
        ++this->size_;
    }

    void push_back(T &&val) {
        if (this->size_ < N) {
            new (this->stackStorage_.data() + this->size_) T(std::move(val));
        } else {
            this->heapStorage_.push_back(std::move(val));
        }
        ++this->size_;
    }

    void clear() {
        if (this->empty()) {
            return;
        }
        for (size_t i = 0; i < std::min(N, this->size_); ++i) {
            this->destroyAtStack(i);
        }
        this->heapStorage_.clear();
        this->size_ = 0;
    }

    T &operator[](size_t i) {
        if (i < N) {
            return this->stackRefAt(i);
        } else {
            return this->heapStorage_[i - N];
        }
    }

    const T &operator[](size_t i) const {
        if (i < N) {
            return this->stackRefAt(i);
        } else {
            return this->heapStorage_[i - N];
        }
    }

    void removeFirst(const T &val) {
        for (size_t i = 0; i < this->size_; ++i) {
            if ((*this)[i] == val) {
                for (; i < this->size_ - 1; ++i) {
                    std::swap((*this)[i], (*this)[i + 1]);
                }
                if (i >= N) {
                    this->heapStorage_.pop_back();
                } else {
                    this->destroyAtStack(i);
                }
                --this->size_;
                return;
            }
        }
    }

    /*
    iterator erase(iterator begin, iterator end) {
        size_t dist = end - begin;
        if (!begin.stack() && !end.stack()) {
            this->heapStorage_.erase(this->heapStorage_.begin() + (begin.i_ - N),
                                     this->heapStorage_.begin() + (end.i_ - N));
            this->size_ -= dist;
            return;
        }

        auto it = begin;

        // Erase items on stack
        size_t itemsErased = 0;
        iterator stackEnd = this->stackEndIt();
        while (it != end && it != stackEnd) {
            this->destroyAtStack(it);
            ++itemsErased;
            ++it;
        }

        // Erase remaining items on heap
        if (it != end) {
            this->heapStorage_.erase(this->heapStorage_.begin() + (it.i_ - N),
                                     this->heapStorage_.begin() + (end.i_ - N));
            itemsErased += end.i_ - it.i_;
        }

        if (it == this->end()) {
            // End of vec, no shifting required
            this->size_ -= dist;
            return;
        }

        if (it == end) {
            // Reached end
            return;
        }
    }
    */

    size_t size() const {
        return this->size_;
    }

    bool empty() const {
        return this->size_ == 0;
    }

    iterator begin() {
        return iterator{0, this};
    }

    iterator end() {
        return iterator{this->size_, this};
    }

    const_iterator begin() const {
        return const_iterator{0, this};
    }

    const_iterator end() const {
        return const_iterator{this->size_, this};
    }

    const_iterator cbegin() const {
        return const_iterator{0, this};
    }

    const_iterator cend() const {
        return const_iterator{this->size_, this};
    }

private:
    void destroyAtStack(size_t i) {
        this->stackRefAt(i).~T();
    }

    void destroyAtStack(iterator it) {
        this->stackRefAt(it.i_).~T();
    }

    iterator stackEndIt() {
        return iterator{N, this};
    }

    T &stackRefAt(size_t i) {
        return reinterpret_cast<T &>(this->stackStorage_[i * sizeof(N)]);
    }

    const T &stackRefAt(size_t i) const {
        return reinterpret_cast<const T &>(this->stackStorage_[i * sizeof(N)]);
    }

    T* stackPtrAt(size_t i) {
        return reinterpret_cast<T*>(&this->stackStorage_[i * sizeof(N)]);
    }

    const T* stackPtrAt(size_t i) const {
        return reinterpret_cast<const T*>(&this->stackStorage_[i * sizeof(N)]);
    }

    static void transferStackStorage(SmallVec &dest, SmallVec &src) noexcept {
        for (size_t i = 0; i < std::min(N, src.size_); ++i) {
            new (dest.stackPtrAt(i)) T(std::move(src.stackRefAt(i)));
            src.destroyAtStack(i);
        }
    }

    alignas(T) std::array<char, sizeof(T) * N> stackStorage_;
    std::vector<T> heapStorage_;

    size_t size_;
};

// NOLINTEND(readability-identifier-naming)

} // namespace dnsge
