#pragma once

#include "array_ptr.h"

#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <utility>

class ReserveProxyObj {
public:
    explicit ReserveProxyObj(size_t capacity_to_reserve)
        : capacity_(capacity_to_reserve)
    {
    }

    size_t GetCapacity() {
        return capacity_;
    }

private:
    size_t capacity_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    explicit SimpleVector(size_t size)
        : SimpleVector(size, std::move(Type()))
    {
    }

    SimpleVector(size_t size, const Type& value)
        : simple_vector_(size)
        , size_(size)
        , capacity_(size)
    {
        std::fill(begin(), end(), value);
    }

    SimpleVector(std::initializer_list<Type> init)
        : simple_vector_(init.size())
        , size_(init.size())
        , capacity_(init.size())
    {
        std::copy(std::make_move_iterator(init.begin()), std::make_move_iterator(init.end()), begin());
    }

    SimpleVector(ReserveProxyObj capacity_to_reserve) {
        Reserve(capacity_to_reserve.GetCapacity());
    }

    SimpleVector(const SimpleVector& other)
        : simple_vector_(other.capacity_)
        , size_(other.size_)
    {
        std::copy(other.begin(), other.end(), simple_vector_.Get());
    }

    SimpleVector(SimpleVector&& other)
        : simple_vector_(other.capacity_)
    {
        swap(other);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            SimpleVector local(rhs);
            swap(local);
        }
        return *this;
    }

    Type& operator[](size_t index) noexcept {
        assert(index < size_);
        return simple_vector_[index];
    }

    const Type& operator[](size_t index) const noexcept {
        assert(index < size_);
        return simple_vector_[index];
    }

    Type& At(size_t index) {
        if (index < size_) {
            return simple_vector_[index];
        }
        else {
            throw std::out_of_range("Out of range");
        }
    }

    const Type& At(size_t index) const {
        if (size_ > index) {
            return simple_vector_[index];
        }
        else {
            throw std::out_of_range("Out of range");
        }
    }

    Iterator begin() noexcept {
        return Iterator(&simple_vector_[0]);
    }

    Iterator end() noexcept {
        return Iterator(&simple_vector_[size_]);
    }

    ConstIterator begin() const noexcept {
        return ConstIterator(&simple_vector_[0]);
    }

    ConstIterator end() const noexcept {
        return ConstIterator(&simple_vector_[size_]);
    }

    ConstIterator cbegin() const noexcept {
        return ConstIterator(&simple_vector_[0]);
    }

    ConstIterator cend() const noexcept {
        return ConstIterator(&simple_vector_[size_]);
    }

    size_t GetSize() const noexcept {
        return size_;
    }

    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    bool IsEmpty() const noexcept {
        return size_ == 0;
    }

    void Clear() noexcept {
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if (capacity_ >= new_size) {
            size_ = new_size;
        }
        if (size_ < new_size) {
            for (auto iterator = begin() + size_; iterator != begin() + new_size; ++iterator) {
                *iterator = std::move(Type());
            }
            size_ = new_size;
        }
        if (capacity_ < new_size) {
            ArrayPtr<Type> local(new_size);
            FillForMove(local.Get(), local.Get() + new_size);
            std::move(simple_vector_.Get(), simple_vector_.Get() + capacity_, local.Get());
            simple_vector_.swap(local);
            size_ = new_size;
            capacity_ = new_size * 2;
        }
    }

    void PushBack(const Type& item) {
        if (size_ + 1 > capacity_) {
            size_t new_capacity = std::max(size_ + 1, capacity_ * 2);
            ArrayPtr<Type> local(new_capacity);
            std::fill(local.Get(), local.Get() + new_capacity, Type());
            std::copy(simple_vector_.Get(), simple_vector_.Get() + size_, local.Get());
            simple_vector_.swap(local);
            capacity_ = new_capacity;
        }
        simple_vector_[size_] = item;
        ++size_;
    }

    void PushBack(Type&& item) {
        if (size_ + 1 > capacity_) {
            size_t new_capacity = std::max(size_ + 1, capacity_ * 2);
            ArrayPtr<Type> local(new_capacity);
            std::move(simple_vector_.Get(), simple_vector_.Get() + size_, local.Get());
            simple_vector_.swap(local);
            capacity_ = new_capacity;
        }
        simple_vector_[size_] = std::move(item);
        ++size_;
    }

    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        auto index_position = std::distance(cbegin(), pos);
        if (size_ < capacity_) {
            std::copy_backward(simple_vector_.Get() + index_position, simple_vector_.Get() + size_, simple_vector_.Get() + size_ + 1);
            simple_vector_[index_position] = value;
        }
        else {
            //Надеюсь, я верно поняла Ваш комментарий про ветку с нулевой вместимостью и ненулевой (для того, чтобы было более логично?)
            if (capacity_ == 0) {
                ArrayPtr<Type> local(1);
                local[index_position] = value;
                simple_vector_.swap(local);
                ++capacity_;
            }
            else {
                size_t new_capacity = std::max(size_ + 1, capacity_ * 2);
                ArrayPtr<Type> local(capacity_);
                std::copy(simple_vector_.Get(), simple_vector_.Get() + size_, local.Get());
                std::copy_backward(simple_vector_.Get() + index_position, simple_vector_.Get() + size_, local.Get() + size_ + 1);
                local[index_position] = value;
                simple_vector_.swap(local);
                capacity_ = new_capacity;
            }
        }
        ++size_;
        return &simple_vector_[index_position];
    }

    Iterator Insert(ConstIterator&& pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        auto index_position = std::distance(cbegin(), pos);
        if (size_ < capacity_) {
            std::move_backward(simple_vector_.Get() + index_position, simple_vector_.Get() + size_, simple_vector_.Get() + size_ + 1);
            simple_vector_[index_position] = std::move(value);
        }
        else {
            if (capacity_ == 0) {
                ArrayPtr<Type> local(1);
                local[index_position] = std::move(value);
                simple_vector_.swap(local);
                ++capacity_;
            }
            else {
                size_t new_capacity = std::max(size_ + 1, capacity_ * 2);
                ArrayPtr<Type> local(new_capacity);
                std::move(simple_vector_.Get(), simple_vector_.Get() + size_,
                    local.Get());
                std::move_backward(simple_vector_.Get() + index_position, simple_vector_.Get() + size_,
                    local.Get() + size_ + 1);
                local[index_position] = std::move(value);
                simple_vector_.swap(local);
                capacity_ = new_capacity;
            }
        }
        ++size_;
        return &simple_vector_[index_position];
    }

    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    Iterator Erase(ConstIterator pos) {
        if (begin() <= pos && end() >= pos) {
            auto index_position = std::distance(cbegin(), pos);
            std::move(&simple_vector_[index_position + 1], end(), const_cast<Iterator>(pos));
            --size_;
            return const_cast<Iterator>(pos);
        }
        else {
            throw std::out_of_range("Out of range");
        }
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> temp(new_capacity);
            std::fill(temp.Get(), temp.Get() + new_capacity, Type());
            std::copy(simple_vector_.Get(), simple_vector_.Get() + size_, temp.Get());
            simple_vector_.swap(temp);
            capacity_ = new_capacity;
        }
    }

    void swap(SimpleVector& other) noexcept {
        simple_vector_.swap(other.simple_vector_);
        std::swap(capacity_, other.capacity_);
        std::swap(size_, other.size_);
    }

private:
    ArrayPtr<Type> simple_vector_;
    size_t size_ = 0;
    size_t capacity_ = 0;

    void FillForMove(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            *first = std::move(Type());
        }
    }
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return rhs < lhs;
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}