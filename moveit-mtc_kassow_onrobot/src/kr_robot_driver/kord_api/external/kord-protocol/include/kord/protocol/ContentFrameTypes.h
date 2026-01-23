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

#ifndef KR2_KORD_CONTENT_FRAME_TYPES_H
#define KR2_KORD_CONTENT_FRAME_TYPES_H

#include <type_traits>

#include <kord/protocol/KORDFrames.h>

namespace kr2::kord::protocol {

/**
 * @brief Trait to get the corresponding ContentFrame type for a given FrameType.
 *
 * The `GetContentFrameType` template struct is specialized for different
 * `FrameType`s to provide the associated `ContentFrame` type.
 *
 * @tparam FrameType The frame type for which to get the corresponding content frame type.
 */
template <typename FrameType>
struct GetContentFrameType;

/**
 * @brief Specialization of `GetContentFrameType` for `KORDFrame`.
 *
 * Associates `KORDFrame` with `KORDContentFrame`.
 */
template <>
struct GetContentFrameType<KORDFrame> {
    using Type = KORDContentFrame; /**< The corresponding content frame type for KORDFrame */
};

/**
 * @brief Specialization of `GetContentFrameType` for `KORDFrameTCP`.
 *
 * Associates `KORDFrameTCP` with `KORDContentFrameTCP`.
 */
template <>
struct GetContentFrameType<KORDFrameTCP> {
    using Type = KORDContentFrameTCP; /**< The corresponding content frame type for KORDFrameTCP */
};

/**
 * @brief Retrieves the size of the content frame based on its type.
 *
 * The `GetContentFrameSize` function template returns the size of the content frame
 * corresponding to the provided `ContentFrameType`. It uses compile-time
 * type checking to determine the appropriate size.
 *
 * @tparam ContentFrameType The type of the content frame.
 * @return The size of the content frame in bytes.
 *
 * @note
 * - For `KORDContentFrame`, the size is `MAX_UDP_DATA_LEN`.
 * - For `KORDContentFrameTCP`, the size is `MAX_TCP_DATA_LEN`.
 * - If an unsupported `ContentFrameType` is provided, a compile-time error is generated.
 */
template <typename ContentFrameType>
constexpr size_t GetContentFrameSize()
{
    if constexpr (std::is_same_v<ContentFrameType, KORDContentFrame>) {
        return MAX_UDP_DATA_LEN; /**< Maximum size for UDP-based content frames */
    }
    else if constexpr (std::is_same_v<ContentFrameType, KORDContentFrameTCP>) {
        return MAX_TCP_DATA_LEN; /**< Maximum size for TCP-based content frames */
    }
    else {
        static_assert(std::is_same_v<ContentFrameType, void>, "Unsupported ContentFrameType");
    }
    return 0;
}

} // namespace kr2::kord::protocol

#endif // KR2_KORD_CONTENT_FRAME_TYPES_H
