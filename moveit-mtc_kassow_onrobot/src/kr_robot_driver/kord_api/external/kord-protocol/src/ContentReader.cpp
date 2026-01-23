/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2022, Kassow Robots
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

#include <kord/protocol/ContentReader.h>

#include <array>

using namespace kr2::kord::protocol;

ContentReader::ContentReader(uint8_t *a_head, size_t a_length) : content_(a_head), head_(a_head), length_(a_length) {}

void ContentReader::reset(uint8_t *a_head, size_t a_length)
{
    head_ = a_head;
    content_ = a_head;
    length_ = a_length;
}

template <typename T> T ContentReader::getData()
{
    T out{};
    dataRead((uint8_t *)(&out), sizeof(T)); // TODO handle failure
    return out;
}

template double ContentReader::getData<double>();
template uint8_t ContentReader::getData<uint8_t>();
template uint16_t ContentReader::getData<uint16_t>();
template uint32_t ContentReader::getData<uint32_t>();
template uint64_t ContentReader::getData<uint64_t>();
template int8_t ContentReader::getData<int8_t>();
template int16_t ContentReader::getData<int16_t>();
template int32_t ContentReader::getData<int32_t>();
template int64_t ContentReader::getData<int64_t>();
template std::array<double, 6> ContentReader::getData<std::array<double, 6>>();
template std::array<double, 3> ContentReader::getData<std::array<double, 3>>();
template std::array<double, 7> ContentReader::getData<std::array<double, 7>>();
template std::array<uint32_t, 6> ContentReader::getData<std::array<uint32_t, 6>>();
template std::array<uint32_t, 7> ContentReader::getData<std::array<uint32_t, 7>>();
template std::array<uint16_t, 7> ContentReader::getData<std::array<uint16_t, 7>>();
template std::array<float, 7> ContentReader::getData<std::array<float, 7>>();

template <typename T> T ContentReader::getData(unsigned int a_offset)
{
    T out{};
    dataRead((uint8_t *)(&out), sizeof(T), a_offset); // TODO handle failure and initialize to INF
    return out;
}

template bool ContentReader::getData<bool>(unsigned int offset);
template double ContentReader::getData<double>(unsigned int offset);
template uint8_t ContentReader::getData<uint8_t>(unsigned int offset);
template uint16_t ContentReader::getData<uint16_t>(unsigned int offset);
template uint32_t ContentReader::getData<uint32_t>(unsigned int offset);
template uint64_t ContentReader::getData<uint64_t>(unsigned int offset);
template int8_t ContentReader::getData<int8_t>(unsigned int offset);
template int16_t ContentReader::getData<int16_t>(unsigned int offset);
template int32_t ContentReader::getData<int32_t>(unsigned int offset);
template int64_t ContentReader::getData<int64_t>(unsigned int offset);
template std::array<double, 6> ContentReader::getData<std::array<double, 6>>(unsigned int offset);
template std::array<double, 7> ContentReader::getData<std::array<double, 7>>(unsigned int offset);
template std::array<double, 3> ContentReader::getData<std::array<double, 3>>(unsigned int offset);
template std::array<uint32_t, 6> ContentReader::getData<std::array<uint32_t, 6>>(unsigned int offset);
template std::array<uint32_t, 7> ContentReader::getData<std::array<uint32_t, 7>>(unsigned int offset);
template std::array<uint16_t, 7> ContentReader::getData<std::array<uint16_t, 7>>(unsigned int offset);
template std::array<float, 7> ContentReader::getData<std::array<float, 7>>(unsigned int offset);

bool ContentReader::getData(std::vector<uint8_t> &a_data, unsigned int a_offset)
{
    // Check the read size is sensible
    if (a_offset >= length_)
        return false;

    uint16_t array_length = 0;
    dataRead((uint8_t *)(&array_length), 2, a_offset);

    // It is assumed the caller expects some non-zero data.
    if (array_length == 0) {
        return false;
    }

    // Array seems to be too long to make sense.
    if ((a_offset + array_length + 2) > length_) {
        return false;
    }

    a_data.clear();
    unsigned int array_offset = a_offset + 2; // 2B array size
    for (uint32_t i = 0; i < array_length; i++) {
        uint8_t data = 0;
        dataRead(&data, 1, array_offset + i);
        a_data.push_back(data);
    }

    return true;
}

int ContentReader::dataRead(uint8_t *a_data, size_t a_length)
{
    if (!content_ || !head_)
        return -2;
    if (a_length > static_cast<size_t>(head_ + length_ - content_))
        return -1;

    std::copy(content_, content_ + a_length, a_data);
    content_ += a_length;
    return static_cast<int>(a_length);
}

int ContentReader::dataRead(uint8_t *a_data, size_t a_length, unsigned int a_offset)
{
    if (a_offset >= length_)
        return -1;
    if ((a_offset + a_length) > length_)
        return -3;
    if (!head_)
        return -2; // content_ is not used here

    std::copy(head_ + a_offset, head_ + a_offset + a_length, a_data);
    return static_cast<int>(a_length);
}