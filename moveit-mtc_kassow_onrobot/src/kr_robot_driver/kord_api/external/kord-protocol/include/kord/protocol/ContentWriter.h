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

#ifndef KR2_KORD_CONTENT_WRITER_H
#define KR2_KORD_CONTENT_WRITER_H

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace kr2::kord::protocol {

/**
 * @brief Provides functionality to write data to a content buffer.
 *
 * The `ContentWriter` class facilitates writing various types of data into a buffer
 * associated with KORD protocol frames. It supports writing primitive types, arrays,
 * and binary data with specified offsets.
 */
class ContentWriter {
public:
    /**
     * @brief Default constructor.
     *
     * Initializes a `ContentWriter` instance without associating it with any buffer.
     */
    ContentWriter() = default;

    /**
     * @brief Virtual destructor.
     *
     * Ensures proper cleanup in derived classes.
     */
    virtual ~ContentWriter() = default;

    /**
     * @brief Parameterized constructor.
     *
     * Initializes a `ContentWriter` with the specified buffer and reserve size.
     *
     * @param head Pointer to the start of the buffer.
     * @param reserve The reserve size in bytes for writing data.
     */
    ContentWriter(uint8_t *head, size_t reserve)
        : content_{head}, capacity_{reserve}, head_{head}, tail_{head_ + reserve}
    {
    }

    /**
     * @brief Resets the ContentWriter with a new buffer and reserve size.
     *
     * Associates the `ContentWriter` with a new buffer, allowing it to write to
     * a different data destination.
     *
     * @param head Pointer to the new buffer.
     * @param reserve The reserve size in bytes for the new buffer.
     */
    void reset(uint8_t *head, size_t reserve)
    {
        content_ = head;
        capacity_ = reserve;
        head_ = head;
        tail_ = head_ + reserve;
    }

    /**
     * @brief Clears the content buffer.
     *
     * Resets the write pointer to the start of the buffer, effectively clearing
     * any data that was previously written.
     */
    virtual void clear() { content_ = head_; }

    /**
     * @brief Adds a fixed-size array of doubles to the content buffer.
     *
     * @param v A `std::array<double, 7>` containing the data to be added.
     * @return `true` if the array was added successfully; `false` otherwise.
     *
     * @note Returns `false` if the input array is empty or if there is insufficient
     *       space in the buffer to add the array.
     */
    bool addData(const std::array<double, 7> &v)
    {
        if (v.empty())
            return false;
        size_t len = sizeof(v);
        auto written = dataWrite(reinterpret_cast<const uint8_t *>(v.data()), len);
        return written == len;
    }

    /**
     * @brief Adds a fixed-size array of floats to the content buffer.
     *
     * @param v A `std::array<float, 7>` containing the data to be added.
     * @return `true` if the array was added successfully; `false` otherwise.
     */
    bool addData(const std::array<float, 7> &v)
    {
        if (v.empty())
            return false;
        size_t len = sizeof(v);
        auto written = dataWrite(reinterpret_cast<const uint8_t *>(v.data()), len);
        return written == len;
    }

    /**
     * @brief Adds data of an arbitrary integral or floating type to the content buffer.
     *
     * This templated function allows adding data of any type `T` that can be
     * represented as a sequence of bytes.
     *
     * @tparam T The type of data to add.
     * @param data A constant reference to the data to be added.
     * @return `true` if the data was added successfully; `false` otherwise.
     *
     * @note Ensure that the type `T` does not exceed the buffer's remaining capacity.
     */
    template <typename T>
    bool addData(const T &data)
    {
        size_t len = sizeof(data);
        auto written = dataWrite(reinterpret_cast<const uint8_t *>(&data), len);
        return written == len;
    }

    /**
     * @brief Adds data of an arbitrary integral or floating type at a specified offset.
     *
     * This templated function allows adding data of any type `T` at a specific
     * offset within the buffer.
     *
     * @tparam T The type of data to add.
     * @param data A constant reference to the data to be added.
     * @param offset The byte offset within the buffer where the data should be written.
     * @return `true` if the data was added successfully; `false` otherwise.
     *
     * @note Returns `false` if the offset is invalid or if there is insufficient space
     *       in the buffer to add the data.
     */
    template <typename T>
    bool addData(const T &data, unsigned int offset)
    {
        if (offset == UINT32_MAX) {
            return false;
        }
        size_t len = sizeof(data);
        auto written = dataWrite(reinterpret_cast<const uint8_t *>(&data), offset, len);
        return written == len;
    }

    /**
     * @brief Adds a byte array into the content buffer at a specified offset.
     *
     * This function writes a sequence of bytes into the buffer starting at the given
     * offset. It first writes the size of the array as a `uint16_t` and then writes
     * the array data itself.
     *
     * @param a_in_data A `std::vector<uint8_t>` containing the data to be added.
     * @param a_offset The byte offset within the buffer where the data should be written.
     * @return `true` if the array was added successfully; `false` otherwise.
     *
     * @note
     * - Returns `false` if writing beyond the acceptable region.
     * - Returns `false` if the input array is empty.
     * - Returns `false` if there is insufficient space to write the array size and data.
     * - Returns `false` if writing the array size or data fails.
     */
    bool addData(const std::vector<uint8_t> &a_in_data, unsigned int a_offset)
    {
        if (head_ + a_offset >= tail_) {
            return false; // writing beyond acceptable region
        }

        if (a_in_data.empty()) {
            return false; // empty array
        }

        if (a_in_data.size() > capacity_) {
            // Not enough space to write
            return false;
        }

        if (a_in_data.size() > static_cast<size_t>(tail_ - (head_ + a_offset))) {
            return false; // there is not enough space in the remaining memory
        }

        unsigned int array_offset = a_offset;

        array_offset += 2;
        uint16_t in_data_size = a_in_data.size();
        if (dataWrite(reinterpret_cast<const uint8_t *>(&in_data_size), a_offset, 2) != 2) {
            return false; // failed to write length
        }

        if (dataWrite(a_in_data.data(), array_offset, a_in_data.size()) != a_in_data.size()) {
            return false;
        }
        return true;
    }

private:
    /**
     * @brief Writes raw data to the buffer.
     *
     * Copies a specified number of bytes from the source data into the buffer at the current
     * write position. Advances the write pointer and decreases the available capacity accordingly.
     *
     * @param data Pointer to the source data to be written.
     * @param length The number of bytes to write.
     * @return The number of bytes successfully written.
     *
     * @note Returns `0` if the buffer is not associated or if there is insufficient capacity.
     */
    size_t dataWrite(const uint8_t *data, size_t length)
    {
        if (!content_)
            return 0;
        if (length > capacity_)
            return 0;

        std::copy(data, data + length, content_);
        content_ += length;
        capacity_ -= length;
        return length;
    }

    /**
     * @brief Writes raw data to the buffer at a specified offset.
     *
     * Copies a specified number of bytes from the source data into the buffer at the given
     * offset. Adjusts the write pointer based on the offset and length.
     *
     * @param data Pointer to the source data to be written.
     * @param offset The byte offset within the buffer where the data should be written.
     * @param length The number of bytes to write.
     * @return The number of bytes successfully written.
     *
     * @note
     * - Returns `0` if the buffer is not associated, if there is insufficient capacity,
     *   or if writing beyond the buffer's tail.
     * - Maintains the highest written offset to accurately report the buffer's size.
     */
    size_t dataWrite(const uint8_t *data, unsigned int offset, size_t length)
    {
        if (!content_ || !head_ || !tail_)
            return 0;
        if (length > capacity_)
            return 0;
        if ((head_ + offset + length) > tail_)
            return 0;

        std::copy(data, data + length, head_ + offset);

        // Check if the offset about to be written is beyond the current
        // keep the highest offset - it has effect on reported size
        // when writing by offset it is desirable to keep the highest written
        uint8_t *pc = content_;
        content_ = head_ + offset + length;
        if (content_ < pc) {
            // Restore original pointer
            content_ = pc;
        }
        capacity_ -= length;
        return length;
    }

protected:
    uint8_t *content_{}; /**< Pointer to the current write position within the buffer. */
    size_t capacity_{0}; /**< Remaining capacity in the buffer in bytes. */
    uint8_t *head_{};    /**< Pointer to the start of the allocated memory buffer. */
    uint8_t *tail_{};    /**< Pointer to the end of the allocated memory buffer. */
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_CONTENT_WRITER_H
