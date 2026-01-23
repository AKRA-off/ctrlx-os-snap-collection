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

#ifndef KR2_KORD_CONTENT_READER_H
#define KR2_KORD_CONTENT_READER_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace kr2::kord::protocol {

/**
 * @brief Provides functionality to read data from a content buffer.
 *
 * The `ContentReader` class facilitates reading various types of data from a buffer
 * associated with KORD protocol frames. It supports reading primitive types, arrays,
 * and binary data with specified offsets.
 */
class ContentReader {
public:
    /**
     * @brief Default constructor.
     *
     * Initializes a `ContentReader` instance without associating it with any buffer.
     */
    ContentReader() = default;

    /**
     * @brief Parameterized constructor.
     *
     * Initializes a `ContentReader` with the specified buffer and its length.
     *
     * @param head Pointer to the start of the buffer.
     * @param length The size of the buffer in bytes.
     */
    ContentReader(uint8_t *head, size_t length);

    /**
     * @brief Resets the ContentReader with a new buffer and its length.
     *
     * Associates the `ContentReader` with a new buffer, allowing it to read from
     * a different data source.
     *
     * @param head Pointer to the new buffer.
     * @param length The size of the new buffer in bytes.
     */
    void reset(uint8_t *head, size_t length);

    /**
     * @brief Retrieves data of type T from the current read position.
     *
     * Reads data of the specified type from the buffer starting at the current
     * read position and advances the read pointer accordingly.
     *
     * @tparam T The type of data to retrieve.
     * @return The data of type T read from the buffer.
     *
     * @note Ensure that the buffer contains enough data to read the specified type.
     */
    template <typename T> T getData();

    /**
     * @brief Retrieves data of type T from a specified offset.
     *
     * Reads data of the specified type from the buffer starting at the given offset
     * without altering the current read position.
     *
     * @tparam T The type of data to retrieve.
     * @param offset The byte offset from the start of the buffer to begin reading.
     * @return The data of type T read from the specified offset.
     *
     * @note Ensure that the buffer contains enough data at the specified offset.
     */
    template <typename T> T getData(unsigned int offset);

    /**
     * @brief Retrieves an array of bytes from a specified offset.
     *
     * This function reads a sequence of bytes from the buffer starting at the given
     * offset and stores them in the provided `std::vector<uint8_t>`. The size of
     * the array to be retrieved is determined by reading size information from
     * the offset location.
     *
     * The input data vector will be cleared before populating it with the retrieved data.
     *
     * @param data Output parameter where the retrieved data will be stored.
     * @param offset The byte offset from the start of the buffer to begin reading.
     * @return `true` if the data was successfully retrieved; `false` otherwise.
     *
     * @note The function returns `false` if the offset does not contain valid size information.
     */
    bool getData(std::vector<uint8_t> &data, unsigned int offset);

private:
    /**
     * @brief Reads raw data from the buffer into the provided destination.
     *
     * Copies a specified number of bytes from the current read position into the
     * destination buffer.
     *
     * @param data Pointer to the destination buffer where data will be copied.
     * @param length The number of bytes to read.
     * @return `0` on success; non-zero value on failure.
     */
    int dataRead(uint8_t *data, size_t length);

    /**
     * @brief Reads raw data from the buffer at a specified offset into the provided destination.
     *
     * Copies a specified number of bytes from the given offset in the buffer into the
     * destination buffer.
     *
     * @param data Pointer to the destination buffer where data will be copied.
     * @param length The number of bytes to read.
     * @param offset The byte offset from the start of the buffer to begin reading.
     * @return `0` on success; non-zero value on failure.
     */
    int dataRead(uint8_t *data, size_t length, unsigned int offset);

protected:
    uint8_t *content_{}; /**< Pointer to the current read position within the buffer. */
    uint8_t *head_{};    /**< Pointer to the start of allocated memory. */
    size_t length_{0};   /**< Size of allocated memory in bytes. */
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_CONTENT_READER_H
