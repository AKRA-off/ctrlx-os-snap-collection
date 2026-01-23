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

#ifndef KR2_KORD_STATUS_FRAME_BUILDER_H
#define KR2_KORD_STATUS_FRAME_BUILDER_H

#include <kord/protocol/ContentWriter.h>

#include <cstddef>
#include <cstdint>

namespace kr2::kord::protocol {

/**
 * @brief Builder class for constructing status frames.
 *
 * The `StatusFrameBuilder` class inherits from `ContentWriter` and provides functionalities
 * to build status frames by adding various data items such as sequence numbers. It manages
 * the internal structure and ensures that the payload is correctly formatted for transmission.
 */
class StatusFrameBuilder : public ContentWriter {
public:
    /**
     * @brief Default constructor.
     *
     * Initializes a `StatusFrameBuilder` instance with default settings.
     */
    StatusFrameBuilder();

    /**
     * @brief Parameterized constructor.
     *
     * Initializes a `StatusFrameBuilder` with a specified frequency.
     *
     * @param frequency The frequency at which the status frame should be built or updated.
     */
    explicit StatusFrameBuilder(unsigned int frequency);

    /**
     * @brief Copy constructor.
     *
     * Creates a new `StatusFrameBuilder` as a copy of an existing one.
     *
     * @param other The `StatusFrameBuilder` instance to copy from.
     */
    StatusFrameBuilder(const StatusFrameBuilder &other);

    /**
     * @brief Assignment operator.
     *
     * Assigns the contents of one `StatusFrameBuilder` to another.
     *
     * @param other The `StatusFrameBuilder` instance to assign from.
     * @return Reference to the assigned `StatusFrameBuilder`.
     */
    StatusFrameBuilder &operator=(const StatusFrameBuilder &other);

    /**
     * @brief Destructor.
     *
     * Cleans up resources used by the `StatusFrameBuilder`.
     */
    ~StatusFrameBuilder() override;

    /**
     * @brief Copies the built payload to an external buffer.
     *
     * This function attempts to copy the constructed status frame's payload into the provided
     * external buffer.
     *
     * @param buffer Pointer to the external buffer where the payload will be copied.
     * @param buffer_reserve The size of the external buffer in bytes.
     * @return `true` if the payload was successfully copied.
     * @return `false` if the frame has not been built or if the external buffer is insufficient.
     */
    bool getPayload(uint8_t *buffer, size_t buffer_reserve) const;

    /**
     * @brief Adds a sequence number to the status frame.
     *
     * This function appends a sequence number to the status frame, which can be used for tracking
     * and synchronization purposes.
     *
     * @param sequence_number The sequence number to add.
     * @return `true` if the sequence number was successfully added.
     * @return `false` if the sequence number could not be added.
     */
    bool addSequenceNumber(uint16_t sequence_number);

    /**
     * @brief Clears the status frame builder.
     *
     * Resets the `StatusFrameBuilder`, allowing a new status frame to be constructed.
     */
    void clear() override;

private:
    /**
     * @brief Implementation class for StatusFrameBuilder.
     *
     * The `StatusFrameBuilderImpl` class hides the implementation details of `StatusFrameBuilder`
     * using the Pimpl idiom, promoting encapsulation and reducing compilation dependencies.
     */
    class StatusFrameBuilderImpl;

    /**
     * @brief Pointer to the internal implementation.
     *
     * This pointer manages the internal state and operations of the `StatusFrameBuilder`.
     */
    StatusFrameBuilderImpl *impl_;
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_STATUS_FRAME_BUILDER_H
