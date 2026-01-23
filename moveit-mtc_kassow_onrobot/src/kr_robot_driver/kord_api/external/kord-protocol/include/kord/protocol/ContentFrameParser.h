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

#ifndef KR2_KORD_CONTENT_FRAME_PARSER_H
#define KR2_KORD_CONTENT_FRAME_PARSER_H

#include <kord/protocol/ContentFrameTypes.h>
#include <kord/protocol/ContentItem.h>
#include <kord/protocol/KORDItemIDs.h>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace kr2::kord::protocol {

/**
 * @brief Parser class for interpreting content frames.
 *
 * The ContentFrameParser template class provides functionality to parse
 * content frames of a specified type by extracting content items from a payload.
 *
 * @tparam ContentFrameType The type of content frame to parse.
 */
template <typename ContentFrameType>
class ContentFrameParser {
public:
    /**
     * @brief The size of the content frame.
     */
    static constexpr size_t ContentFrameSize = GetContentFrameSize<ContentFrameType>();

    /**
     * @brief Default constructor.
     *
     * Initializes a new instance of the ContentFrameParser.
     */
    ContentFrameParser();

    /**
     * @brief Copy constructor.
     *
     * Creates a new ContentFrameParser as a copy of an existing one.
     *
     * @param other The ContentFrameParser to copy from.
     */
    ContentFrameParser(const ContentFrameParser &other);

    /**
     * @brief Destructor.
     *
     * Cleans up resources used by the ContentFrameParser.
     */
    ~ContentFrameParser();

    /**
     * @brief Sets the parser's state from a given payload.
     *
     * This function initializes the parser with the provided payload data,
     * allowing subsequent retrieval of content items.
     *
     * @param payload Pointer to the payload data to parse.
     * @param dlen The length of the payload data in bytes.
     * @return true if the payload was successfully parsed.
     * @return false if the payload is invalid or parsing failed.
     */
    bool setFromPayload(const uint8_t *payload, size_t dlen);

    /**
     * @brief Retrieves the number of content items contained in the frame.
     *
     * @return The number of content items.
     */
    [[nodiscard]] int getItemsCount() const;

    /**
     * @brief Retrieves the identifier of a specific content item.
     *
     * @param idx The index of the content item.
     * @return The EKORDItemID of the specified content item.
     */
    [[nodiscard]] EKORDItemID getItemID(int idx) const;

    /**
     * @brief Retrieves the content of a specific content item.
     *
     * @param idx The index of the content item.
     * @return The ContentItem corresponding to the specified index.
     */
    [[nodiscard]] ContentItem<ContentFrameSize> getItemContent(int idx) const;

    /**
     * @brief Retrieves the content of a specific content item.
     *
     * This function fills the provided ContentItem pointer with the content
     * of the specified item, if it exists.
     *
     * @param idx The index of the content item.
     * @param item Pointer to a ContentItem where the content will be stored.
     * @return true if the content item was successfully retrieved.
     * @return false if the index is out of range or retrieval failed.
     */
    bool getContentItem(int idx, ContentItem<ContentFrameSize> *item) const;

    /**
     * @brief Assignment operator.
     *
     * Assigns the contents of one ContentFrameParser to another.
     *
     * @param other The ContentFrameParser to assign from.
     * @return Reference to the assigned ContentFrameParser.
     */
    ContentFrameParser &operator=(const ContentFrameParser &other);

    /**
     * @brief Clears the parser's state.
     *
     * Resets the ContentFrameParser, removing any parsed data and allowing
     * it to parse a new frame.
     *
     * @return true if the parser was successfully cleared.
     * @return false if the parser could not be cleared.
     */
    bool clear();

private:
    /**
     * @brief Implementation class for ContentFrameParser.
     *
     * The actual implementation details are hidden using the Pimpl idiom.
     */
    class ContentFrameParserImpl;

    /**
     * @brief Unique pointer to the implementation.
     */
    std::unique_ptr<ContentFrameParserImpl> impl_;
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_CONTENT_FRAME_PARSER_H
