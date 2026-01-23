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

/**
 * @file StringUtils.h
 * @brief Provides utility functions for string manipulation within the KORD protocol.
 */

#ifndef KR2_PROTOCOL_STRING_UTILS_H
#define KR2_PROTOCOL_STRING_UTILS_H

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace kr2::kord::protocol::StringUtils {

/**
 * @brief Trims leading and trailing whitespace from a string.
 *
 * This function removes all leading and trailing whitespace characters from the input string.
 * Whitespace characters considered include space (` `), horizontal tab (`\t`), newline 
 * and carriage return (`\r`).
 *
 * @param str The input string to be trimmed.
 * @return A new string with leading and trailing whitespace removed.
 *
 * @note If the input string consists entirely of whitespace characters, an empty string is returned.
 */
inline std::string trim(const std::string &str)
{
    const std::string whitespace = " \t\n\r";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return ""; // All whitespace

    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

/**
 * @brief Splits a string by a given delimiter and returns a vector of tokens.
 *
 * This function divides the input string into multiple substrings based on the specified
 * delimiter character. Each substring (token) is added to a vector, which is then returned.
 *
 * @param s The input string to be split.
 * @param delimiter The character used to split the string.
 * @return A vector containing the resulting tokens after splitting.
 *
 * @note If the delimiter is not found in the input string, the returned vector will contain
 *       a single element, which is the original string.
 */
inline std::vector<std::string> split(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

} // namespace kr2::kord::protocol::StringUtils

#endif // KR2_PROTOCOL_STRING_UTILS_H
