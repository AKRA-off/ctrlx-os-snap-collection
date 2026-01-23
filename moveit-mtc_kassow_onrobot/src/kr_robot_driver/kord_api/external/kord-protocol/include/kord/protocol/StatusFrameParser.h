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

#ifndef KR2_KORD_STATUS_FRAME_PARSER_H
#define KR2_KORD_STATUS_FRAME_PARSER_H

#include <kord/protocol/ContentReader.h>

#include <cstddef>
#include <cstdint>

namespace kr2::kord::protocol {

/**
 * @brief Parser class for interpreting status frames.
 *
 * The `StatusFrameParser` class inherits from `ContentReader` and provides functionalities
 * to parse status frames by extracting various data items such as frequency, version, and sequence number.
 * It manages the internal structure and ensures that the payload is correctly interpreted.
 */
class StatusFrameParser : public ContentReader {
public:
    /**
     * @brief Default constructor.
     *
     * Initializes a `StatusFrameParser` instance with default settings.
     */
    StatusFrameParser();

    /**
     * @brief Destructor.
     *
     * Cleans up resources used by the `StatusFrameParser`.
     */
    virtual ~StatusFrameParser();

    /**
     * @brief Copy constructor.
     *
     * Creates a new `StatusFrameParser` as a copy of an existing one.
     *
     * @param other The `StatusFrameParser` instance to copy from.
     */
    StatusFrameParser(const StatusFrameParser &other);

    /**
     * @brief Assignment operator.
     *
     * Assigns the contents of one `StatusFrameParser` to another.
     *
     * @param other The `StatusFrameParser` instance to assign from.
     * @return Reference to the assigned `StatusFrameParser`.
     */
    StatusFrameParser &operator=(const StatusFrameParser &other);

    /**
     * @brief Sets the parser's state from the provided payload.
     *
     * This function initializes the parser with the provided payload data,
     * allowing subsequent retrieval of data items such as frequency, version, and sequence number.
     *
     * @param data Pointer to the payload data to parse.
     * @param data_len The length of the payload data in bytes.
     * @return `true` if the payload was successfully parsed; `false` otherwise.
     */
    bool setFromPayload(uint8_t *data, size_t data_len);

    /**
     * @brief Retrieves the frequency from the parsed status frame.
     *
     * @return The frequency as an `unsigned int`.
     */
    [[nodiscard]] unsigned int getFrequency() const;

    /**
     * @brief Retrieves the version from the parsed status frame.
     *
     * @return The version as an `unsigned int`.
     */
    [[nodiscard]] unsigned int getVersion() const;

    /**
     * @brief Retrieves the sequence number from the parsed status frame.
     *
     * @return The sequence number as an `unsigned int`.
     */
    [[nodiscard]] unsigned int getSequenceNumber() const;

    /**
     * @brief Clears the parser's state.
     *
     * Resets the `StatusFrameParser`, removing any parsed data and allowing it to parse a new frame.
     */
    void clear();

private:
    /**
     * @brief Implementation class for StatusFrameParser.
     *
     * The `StatusFrameParserImpl` class hides the implementation details of `StatusFrameParser`
     * using the Pimpl idiom, promoting encapsulation and reducing compilation dependencies.
     */
    class StatusFrameParserImpl;

    /**
     * @brief Pointer to the internal implementation.
     *
     * This pointer manages the internal state and operations of the `StatusFrameParser`.
     */
    StatusFrameParserImpl *impl_; /**< Pointer to the internal implementation */
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_STATUS_FRAME_PARSER_H
