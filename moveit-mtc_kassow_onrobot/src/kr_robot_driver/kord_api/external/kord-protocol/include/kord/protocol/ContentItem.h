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

#ifndef KR2_KORD_CONTENT_ITEM_H
#define KR2_KORD_CONTENT_ITEM_H

#include <kord/protocol/ContentReader.h>
#include <kord/protocol/ContentWriter.h>
#include <kord/protocol/KORDFrames.h>
#include <kord/protocol/KORDItemIDs.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace kr2::kord::protocol {

/**
 * @brief Represents a content item within a KORD frame.
 *
 * The `ContentItem` class is a versatile container that can represent various types of
 * content items. It provides functionalities to read from and write to the underlying data buffer.
 *
 * @tparam BUFFER_MAX_LEN The maximum length of the internal buffer. Defaults to 1500 bytes.
 */
template <size_t BUFFER_MAX_LEN = 1500> class ContentItem : public ContentReader, public ContentWriter {
public:
    /**
     * @brief Default constructor.
     *
     * Initializes a `ContentItem` with no associated item ID and resets the internal buffer.
     */
    ContentItem() : iid_(EKORDItemID::eNone) { reset(buffer_.size()); }

    /**
     * @brief Parameterized constructor.
     *
     * Initializes a `ContentItem` with the specified item ID and resets the internal buffer.
     *
     * @param iid The `EKORDItemID` to associate with this content item.
     */
    explicit ContentItem(EKORDItemID iid) : iid_(iid) { reset(buffer_.size()); }

    /**
     * @brief Parameterized constructor with buffer initialization.
     *
     * Initializes a `ContentItem` with the specified item ID and copies the provided buffer data
     * into the internal buffer.
     *
     * @param iid The `EKORDItemID` to associate with this content item.
     * @param buffer Pointer to the buffer containing the initial data.
     * @param buffer_len The length of the data in the buffer.
     */
    ContentItem(EKORDItemID iid, const uint8_t *buffer, size_t buffer_len) : iid_(iid)
    {
        setFromFrame(iid, buffer, buffer_len);
    }

    /**
     * @brief Copy constructor.
     *
     * Creates a new `ContentItem` as a copy of an existing one. If the source item's data length
     * exceeds `BUFFER_MAX_LEN`, the new item is set to have no associated item ID.
     *
     * @param other The `ContentItem` to copy from.
     */
    ContentItem(const ContentItem &other) : ContentReader(), ContentWriter(), iid_(other.iid_)
    {
        size_t odl = other.getItemDataLength();
        if (odl > BUFFER_MAX_LEN) {
            iid_ = EKORDItemID::eNone;
        }
        else {
            setFromFrame(other.iid_, other.buffer_.data(), odl);
        }
    }

    /**
     * @brief Retrieves the item ID of the content item.
     *
     * @return The `EKORDItemID` associated with this content item.
     */
    [[nodiscard]] EKORDItemID getItemID() const { return iid_; }

    /**
     * @brief Retrieves the length of the content data.
     *
     * @return The length of the content data in bytes.
     */
    [[nodiscard]] size_t getItemDataLength() const { return ContentWriter::content_ - ContentWriter::head_; }

    /**
     * @brief Retrieves a pointer to the content data.
     *
     * @return A constant pointer to the content data buffer.
     */
    [[nodiscard]] const uint8_t *getItemData() const { return buffer_.data(); }

    /**
     * @brief Assignment operator.
     *
     * Assigns the contents of one `ContentItem` to another. If the source item's data length
     * exceeds `BUFFER_MAX_LEN`, the destination item is set to have no associated item ID.
     *
     * @param other The `ContentItem` to assign from.
     * @return Reference to the assigned `ContentItem`.
     */
    ContentItem &operator=(const ContentItem &other)
    {
        size_t odl = other.getItemDataLength();
        if (odl > BUFFER_MAX_LEN) {
            iid_ = EKORDItemID::eNone;
        }
        else {
            setFromFrame(other.iid_, other.buffer_.data(), odl);
        }

        return *this;
    }

    /**
     * @brief Clears the content item.
     *
     * Resets the `ContentItem` by setting the item ID to `eNone`, zeroing out the buffer,
     * and resetting buffer pointers.
     */
    void clear() override
    {
        iid_ = EKORDItemID::eNone;
        reset(buffer_.size());
        buffer_.fill(0x00);
    }

    /**
     * @brief Sets the item ID of the content item.
     *
     * @param iid The `EKORDItemID` to associate with this content item.
     */
    void setItemID(EKORDItemID iid) { iid_ = iid; }

    /**
     * @brief Sets the version of the content item.
     *
     * @param version The version number to set.
     */
    void setVersion(uint32_t version) { version_ = version; }

    /**
     * @brief Initializes the content item from a frame buffer.
     *
     * Copies the provided buffer data into the internal buffer and sets the item ID.
     *
     * @param iid The `EKORDItemID` to associate with this content item.
     * @param buffer Pointer to the buffer containing the data.
     * @param buffer_len The length of the data in the buffer.
     */
    void setFromFrame(EKORDItemID iid, const uint8_t *buffer, size_t buffer_len)
    {
        iid_ = iid;
        std::memcpy(buffer_.data(), buffer, buffer_len);
        reset(buffer_len);
        ContentWriter::content_ += buffer_len;
    }

private:
    /**
     * @brief Resets the ContentReader and ContentWriter with the specified capacity.
     *
     * @param capacity The capacity to set for the internal buffer.
     */
    void reset(size_t capacity)
    {
        ContentReader::reset(buffer_.data(), capacity);
        ContentWriter::reset(buffer_.data(), capacity);
    }

    EKORDItemID iid_;                              /**< The identifier for the content item */
    uint32_t version_{};                           /**< The version number of the content item */
    std::array<uint8_t, BUFFER_MAX_LEN> buffer_{}; /**< Internal buffer holding the content data */
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_CONTENT_ITEM_H
