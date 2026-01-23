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

#ifndef KR2_KORD_PROTOCOL_REQUEST_RESPONSES_TYPES_H
#define KR2_KORD_PROTOCOL_REQUEST_RESPONSES_TYPES_H

#include <cstdint>

namespace kr2::kord::protocol {

/**
 * @brief Maximum length of labels in characters.
 *
 * Defines the maximum number of characters allowed for labels within geometries and safety zones.
 */
constexpr int MAX_LABEL_LENGTH = 128;

/**
 * @brief Maximum number of geometries in a safety zone.
 *
 * Specifies the maximum number of geometry entries that a safety zone can contain.
 */
constexpr int MAX_GEOMETRIES = 16;

/**
 * @struct KORDGeometry
 * @brief Represents the geometry data within a safety zone.
 *
 * This structure holds information about a specific geometry, including unique identifiers,
 * labels, points, and normals.
 *
 * @note The struct is packed to ensure no padding is added by the compiler.
 */
struct KORDGeometry {
    /**
     * @brief Unique identifier for the geometry.
     *
     * This identifier distinguishes each geometry within a safety zone.
     */
    uint32_t geometry_duid_;

    /**
     * @brief Label for the geometry.
     *
     * A human-readable name for the geometry, limited by MAX_LABEL_LENGTH.
     */
    char geometry_label_[MAX_LABEL_LENGTH];

    /**
     * @brief Coordinates of the geometry points.
     *
     * An array of three doubles representing the (x, y, z) coordinates of the geometry.
     */
    double points_[3];

    /**
     * @brief Normals of the geometry points.
     *
     * An array of three doubles representing the normal vectors at each point of the geometry.
     */
    double normals_[3];
} __attribute__((packed));

/**
 * @struct KORDSafetyZone
 * @brief Represents a safety zone with associated geometries and sensitivities.
 *
 * This structure contains information about a safety zone, including unique identifiers,
 * labels, rated speed, buffer width, sensitivities, and associated geometries.
 *
 * @note The struct is packed to ensure no padding is added by the compiler.
 */
struct KORDSafetyZone {
    /**
     * @brief Unique identifier for the safety zone.
     *
     * This identifier distinguishes each safety zone within the system.
     */
    uint32_t zone_duid_;

    /**
     * @brief Unique identifier for the sensitivity associated with the zone.
     *
     * Links the safety zone to its specific sensitivity configuration.
     */
    uint32_t sensitivity_duid_;

    /**
     * @brief Unique identifier for the safety IO associated with the zone.
     *
     * Associates the safety zone with its corresponding safety input/output configuration.
     */
    uint32_t safety_io_duid_;

    /**
     * @brief Rated speed for the safety zone.
     *
     * Specifies the maximum allowable speed within the safety zone, measured in appropriate units (e.g., m/s).
     */
    double rated_speed_;

    /**
     * @brief Buffer width around the safety zone.
     *
     * Defines the additional buffer space surrounding the safety zone to account for safety margins.
     */
    double buffer_width_;

    /**
     * @brief Sensitivity levels for the safety zone.
     *
     * An array of seven doubles representing different sensitivity thresholds or configurations.
     */
    double sensitivities_[7];

    /**
     * @brief Label for the safety zone.
     *
     * A human-readable name for the safety zone, limited by MAX_LABEL_LENGTH.
     */
    char zone_label_[MAX_LABEL_LENGTH];

    /**
     * @brief Label for the sensitivity associated with the zone.
     *
     * A human-readable name for the sensitivity configuration, limited by MAX_LABEL_LENGTH.
     */
    char sensitivity_label_[MAX_LABEL_LENGTH];

    /**
     * @brief Number of geometries in the safety zone.
     *
     * Indicates how many geometry entries are associated with this safety zone, up to MAX_GEOMETRIES.
     */
    uint8_t n_geometries_;

    /**
     * @brief Array of geometries associated with the safety zone.
     *
     * Contains up to MAX_GEOMETRIES entries of type KORDGeometry, defining the spatial boundaries of the safety zone.
     */
    KORDGeometry geometries_[MAX_GEOMETRIES];
} __attribute__((packed));

} // namespace kr2::kord::protocol

#endif // KR2_KORD_PROTOCOL_REQUEST_RESPONSES_TYPES_H
