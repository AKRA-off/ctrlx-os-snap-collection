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

#ifndef KR2_KORD_CONTENT_FRAME_BUILDER_H
#define KR2_KORD_CONTENT_FRAME_BUILDER_H

#include <kord/protocol/ContentFrameTypes.h>
#include <kord/protocol/ContentItem.h>
#include <kord/protocol/KORDItemIDs.h>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace kr2::kord::protocol {

/**
 * @brief Builder class for constructing content frames.
 *
 * The ContentFrameBuilder template class provides functionality to build
 * content frames of a specified type by adding content items and managing
 * the internal buffer.
 *
 * @tparam ContentFrameType The type of content frame to build.
 */
template <typename ContentFrameType> class ContentFrameBuilder {
public:
    /**
     * @brief The size of the content frame.
     */
    static constexpr size_t ContentFrameSize = GetContentFrameSize<ContentFrameType>();

    /**
     * @brief Default constructor.
     *
     * Initializes a new instance of the ContentFrameBuilder.
     */
    ContentFrameBuilder();

    /**
     * @brief Copy constructor.
     *
     * Creates a new ContentFrameBuilder as a copy of an existing one.
     *
     * @param other The ContentFrameBuilder to copy from.
     */
    ContentFrameBuilder(const ContentFrameBuilder &other);

    /**
     * @brief Destructor.
     *
     * Cleans up resources used by the ContentFrameBuilder.
     */
    ~ContentFrameBuilder();

    /**
     * @brief Copies the internal buffer to an external buffer.
     *
     * This function attempts to copy the built frame's payload into the provided
     * external buffer.
     *
     * @param buffer Pointer to the external buffer where the payload will be copied.
     * @param buffer_reserve The size of the external buffer in bytes.
     * @return true if the payload was successfully copied.
     * @return false if the external buffer is insufficient to hold the payload data.
     */
    bool getPayload(uint8_t *buffer, size_t buffer_reserve);

    /**
     * @brief Adds a content item to the frame.
     *
     * This function adds a content item identified by an item ID and its associated
     * data to the content frame being built.
     *
     * @param iid The identifier of the content item.
     * @param data Pointer to the data associated with the content item.
     * @param content_length The length of the content data in bytes.
     * @return true if the content item was successfully added.
     * @return false if the content item could not be added.
     */
    bool addContentItem(EKORDItemID iid, uint8_t *data, size_t content_length);

    /**
     * @brief Adds a content item to the frame.
     *
     * This overload allows adding a content item by providing a ContentItem object.
     *
     * @param item The ContentItem to add to the frame.
     * @return true if the content item was successfully added.
     * @return false if the content item could not be added.
     */
    bool addContentItem(const ContentItem<ContentFrameSize> &item);

    /**
     * @brief Assignment operator.
     *
     * Assigns the contents of one ContentFrameBuilder to another.
     *
     * @param other The ContentFrameBuilder to assign from.
     * @return Reference to the assigned ContentFrameBuilder.
     */
    ContentFrameBuilder &operator=(const ContentFrameBuilder &other);

    /**
     * @brief Clears the frame builder.
     *
     * Resets the ContentFrameBuilder, allowing a new frame to be constructed.
     *
     * @return true if the frame builder was successfully cleared.
     * @return false if the frame builder could not be cleared.
     */
    bool clear();

private:
    /**
     * @brief Implementation class for ContentFrameBuilder.
     *
     * The actual implementation details are hidden using the Pimpl idiom.
     */
    class ContentFrameBuilderImpl;

    /**
     * @brief Pointer to the implementation.
     */
    ContentFrameBuilderImpl *impl_;
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_CONTENT_FRAME_BUILDER_H
