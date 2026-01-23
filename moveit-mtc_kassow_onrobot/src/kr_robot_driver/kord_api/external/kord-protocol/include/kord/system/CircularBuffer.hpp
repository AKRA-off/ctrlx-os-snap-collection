/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2025, Kassow Robots
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Kassow Robots nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#ifndef KR2_KORD_CIRCULAR_BUFFER_HPP
#define KR2_KORD_CIRCULAR_BUFFER_HPP

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace kr2::kord {

/**
 * @brief A fixed-size circular buffer implementation.
 *
 * The `CircularBuffer` class provides a circular buffer (ring buffer) data structure
 * that efficiently manages a fixed-size buffer. It supports operations such as adding
 * elements, accessing elements by index, and searching for elements based on a predicate.
 *
 * @tparam T The type of elements stored in the buffer.
 */
template <typename T> class CircularBuffer {
public:
    /**
     * @brief Constructs a CircularBuffer with the specified capacity.
     *
     * Initializes the buffer with the given capacity, setting the size to zero and the
     * head index to zero.
     *
     * @param capacity The maximum number of elements the buffer can hold.
     */
    explicit CircularBuffer(size_t capacity) : capacity_(capacity), size_(0), head_(0) { buffer_.resize(capacity_); }

    /**
     * @brief Adds an element to the end of the buffer.
     *
     * If the buffer is not full, the element is added to the next available position.
     * If the buffer is full, the oldest element (at the head) is overwritten, and the head
     * index is advanced.
     *
     * @param value The element to add to the buffer.
     */
    void push_back(const T &value)
    {
        if (size_ < capacity_) {
            buffer_[(head_ + size_) % capacity_] = value;
            ++size_;
        }
        else {
            buffer_[head_] = value;
            head_ = (head_ + 1) % capacity_;
        }
    }

    /**
     * @brief Checks if the buffer is empty.
     *
     * @return `true` if the buffer contains no elements; otherwise, `false`.
     */
    [[nodiscard]] bool empty() const { return size_ == 0; }

    /**
     * @brief Retrieves the current number of elements in the buffer.
     *
     * @return The number of elements currently stored in the buffer.
     */
    [[nodiscard]] size_t size() const { return size_; }

    /**
     * @brief Retrieves the maximum capacity of the buffer.
     *
     * @return The maximum number of elements the buffer can hold.
     */
    [[nodiscard]] size_t capacity() const { return capacity_; }

    /**
     * @brief Accesses the element at the specified index.
     *
     * @param index The zero-based index of the element to access.
     * @return A reference to the element at the specified index.
     * @throws std::out_of_range If the index is out of bounds.
     */
    T &operator[](size_t index)
    {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return buffer_[(head_ + index) % capacity_];
    }

    /**
     * @brief Accesses the element at the specified index.
     *
     * Provides read-only access to the element at the given index. The index is zero-based,
     * where index 0 corresponds to the oldest element in the buffer.
     *
     * @param index The zero-based index of the element to access.
     * @return A constant reference to the element at the specified index.
     * @throws std::out_of_range If the index is out of bounds.
     */
    const T &operator[](size_t index) const
    {
        if (index >= size_) {
            throw std::out_of_range("Index out of range");
        }
        return buffer_[(head_ + index) % capacity_];
    }

    /**
     * @brief Clears all elements from the buffer.
     *
     * Resets the buffer by setting the size to zero and the head index to zero.
     * The buffer's capacity remains unchanged.
     */
    void clear()
    {
        size_ = 0;
        head_ = 0;
    }

    /**
     * @brief Finds the first element that satisfies the given predicate.
     *
     * Searches the buffer for an element that meets the condition defined by the predicate.
     * If such an element is found, it is assigned to `foundValue`, and the function returns `true`.
     * Otherwise, the function returns `false`.
     *
     * @tparam Predicate A callable type that takes a `const T&` and returns a `bool`.
     * @param pred The predicate to apply to each element.
     * @param foundValue Reference to store the found element if the predicate returns `true`.
     * @return `true` if an element satisfying the predicate is found; otherwise, `false`.
     */
    template <typename Predicate> bool find_if(Predicate pred, T &foundValue) const
    {
        auto it = std::find_if(begin(), end(), pred);
        if (it != end()) {
            foundValue = *it;
            return true;
        }
        return false;
    }

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;

        iterator(CircularBuffer *buffer, size_t index) : buffer_(buffer), index_(index) {}

        reference operator*() { return (*buffer_)[index_]; }
        pointer operator->() { return &(*buffer_)[index_]; }

        iterator &operator++()
        {
            ++index_;
            return *this;
        }

        iterator operator++(int)
        {
            iterator temp = *this;
            ++index_;
            return temp;
        }

        bool operator==(const iterator &other) const { return index_ == other.index_ && buffer_ == other.buffer_; }
        bool operator!=(const iterator &other) const { return !(*this == other); }

    private:
        CircularBuffer *buffer_;
        size_t index_;
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T *;
        using reference = const T &;

        const_iterator(const CircularBuffer *buffer, size_t index) : buffer_(buffer), index_(index) {}

        reference operator*() const { return (*buffer_)[index_]; }
        pointer operator->() const { return &(*buffer_)[index_]; }

        const_iterator &operator++()
        {
            ++index_;
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator temp = *this;
            ++index_;
            return temp;
        }

        bool operator==(const const_iterator &other) const { return index_ == other.index_ && buffer_ == other.buffer_; }

        bool operator!=(const const_iterator &other) const { return !(*this == other); }

    private:
        const CircularBuffer *buffer_;
        size_t index_;
    };

    /**
     * @brief Returns an iterator to the beginning of the buffer.
     *
     * @return An iterator pointing to the first element in the buffer.
     */
    iterator begin() { return iterator(this, 0); }

    iterator end() { return iterator(this, size_); }

    /**
     * @brief Returns a constant iterator to the beginning of the buffer.
     *
     * @return A constant iterator pointing to the first element in the buffer.
     */
    [[nodiscard]] const_iterator begin() const { return const_iterator(this, 0); }

    /**
     * @brief Returns a constant iterator to the end of the buffer.
     *
     * @return A constant iterator pointing one past the last element in the buffer.
     */
    [[nodiscard]] const_iterator end() const { return const_iterator(this, size_); }

private:
    std::vector<T> buffer_; /**< The underlying storage for the circular buffer */
    size_t capacity_;       /**< The maximum number of elements the buffer can hold */
    size_t size_;           /**< The current number of elements in the buffer */
    size_t head_;           /**< The index of the oldest element in the buffer */
};

} // namespace kr2::kord

#endif // KR2_KORD_CIRCULAR_BUFFER_HPP
