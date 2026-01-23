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

#ifndef KR2_KORD_TYPES_H_
#define KR2_KORD_TYPES_H_

#include <cstdint>
#include <vector>

namespace kr2::kord::protocol {

// Explicit data types
enum class EKORDType : uint16_t {
    eVoid = 0, /**< Void */
    eInt8, /**< Int8 */
    eInt16, /**< Int16 */
    eInt32, /**< Int32 */
    eInt64, /**< Int64 */
    eUInt8, /**< UInt8 */
    eUInt16, /**< UInt16 */
    eUInt32, /**< UInt32 */
    eUInt64, /**< UInt64 */
    eDouble, /**< Double */
    eVector3d, /**< Vector3d */
    eVector6d, /**< Vector6d */
    eVector7d, /**< Vector7d */
    eVector6i, /**< Vector6i */
    eVector7i, /**< Vector7i */
    eVector7ui16, /**< Vector7ui16 */
    eVector7f, /**< Vector7f */
    eByteArray, /**< ByteArray */
    eTypeCount /**< Type count */
};

} // namespace kr2::kord::protocol

#endif