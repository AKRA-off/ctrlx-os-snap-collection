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

#ifndef KR2_KORD_DATA_DESCRIPTION_FORMAT_H
#define KR2_KORD_DATA_DESCRIPTION_FORMAT_H

#include <kord/protocol/KORDDataIDs.h>
#include <kord/protocol/KORDFrames.h>
#include <kord/protocol/KORDItemIDs.h>
#include <kord/protocol/KORDTypes.h>

#include <string>
#include <vector>

namespace kr2::kord::protocol {

/**
 * @brief Represents a single data item in the data format description.
 *
 * The `DataItem` structure holds information about a specific data element,
 * including its identifier, type, and offset within the data frame.
 */
struct DataItem {
    /**
     * @brief Default constructor.
     *
     * Initializes a `DataItem` with default values.
     */
    DataItem();

    /**
     * @brief Parameterized constructor.
     *
     * Initializes a `DataItem` with the specified data ID, type, and offset.
     *
     * @param data_id The identifier of the data item.
     * @param type The type of the data item.
     * @param offset The byte offset of the data item within the frame.
     */
    DataItem(EKORDDataID data_id, EKORDType type, unsigned int offset);

    unsigned int did_;    /**< The identifier for the data item */
    EKORDType type_;      /**< The type of the data item */
    unsigned int offset_; /**< The byte offset of the data item within the frame */
};

/**
 * @brief Manages a list of data item identifiers for format descriptions.
 *
 * The `DataFormatDescriptionItems` class encapsulates a collection of
 * `EKORDDataID` items, providing functionalities to access and manipulate
 * the list of data item identifiers.
 */
class DataFormatDescriptionItems {
public:
    /**
     * @brief Default constructor.
     *
     * Initializes an empty list of data item identifiers.
     */
    DataFormatDescriptionItems() = default;

    /**
     * @brief Parameterized constructor.
     *
     * Initializes the list with a given vector of data item identifiers.
     *
     * @param a_lst A vector of `EKORDDataID` items to initialize the list.
     */
    explicit DataFormatDescriptionItems(std::vector<EKORDDataID> a_lst) : item_list_{a_lst} {}

    /**
     * @brief Virtual destructor.
     *
     * Ensures proper cleanup in derived classes.
     */
    virtual ~DataFormatDescriptionItems() = default;

    /**
     * @brief Retrieves the list of data item identifiers.
     *
     * @return A constant reference to the vector of `EKORDDataID` items.
     */
    [[nodiscard]] const std::vector<EKORDDataID> &getItems() const { return item_list_; }

protected:
    std::vector<EKORDDataID> item_list_{}; /**< List of data item identifiers */
};

/**
 * @brief Forward declaration of the MoveJ_DFD class.
 *
 * The `MoveJ_DFD` class is declared here and defined elsewhere.
 */
// class MoveJ_DFD;

/**
 * @brief Describes the format of data frames in the KORD protocol.
 *
 * The `DataFormatDescription` class encapsulates the structure and metadata
 * of data frames, including the frame ID and the list of data items it contains.
 * It provides functionalities to manipulate and serialize/deserialize the format description.
 */
class DataFormatDescription {
public:
    /**
     * @brief Default constructor.
     *
     * Initializes a `DataFormatDescription` with default values.
     */
    DataFormatDescription();

    /**
     * @brief Parameterized constructor.
     *
     * Initializes a `DataFormatDescription` with the specified frame ID.
     *
     * @param id The identifier for the data format description.
     */
    explicit DataFormatDescription(unsigned int id);

    /**
     * @brief Virtual destructor.
     *
     * Ensures proper cleanup in derived classes.
     */
    virtual ~DataFormatDescription();

    /**
     * @brief Sets the frame ID for the data format description.
     *
     * @param id The new identifier to set.
     * @return Reference to the updated `DataFormatDescription` instance.
     */
    DataFormatDescription &setID(unsigned int id);

    /**
     * @brief Adds a data item identifier to the format description.
     *
     * @param data_id The `EKORDDataID` to add.
     * @return Reference to the updated `DataFormatDescription` instance.
     */
    DataFormatDescription &addItem(EKORDDataID data_id);

    // /**
    //  * @brief Adds a DataItem to the format description.
    //  *
    //  * @param item The `DataItem` to add.
    //  * @return Reference to the updated `DataFormatDescription` instance.
    //  */
    // DataFormatDescription& addItem(DataItem item);

    /**
     * @brief Retrieves the frame ID of the data format description.
     *
     * @return The frame identifier as a `uint32_t`.
     */
    [[nodiscard]] uint32_t ID() const;

    /**
     * @brief Retrieves the list of data items describing the format.
     *
     * @return A constant reference to the vector of `DataItem` structures.
     */
    [[nodiscard]] const std::vector<DataItem> &describeFormat() const;

    /**
     * @brief Converts the data format description to a string representation.
     *
     * @return A `std::string` representing the data format description.
     */
    [[nodiscard]] std::string asString() const;

    /**
     * @brief Retrieves the number of data items in the format description.
     *
     * @return The count of data items as an `unsigned int`.
     */
    [[nodiscard]] unsigned int getItemsCount() const;

    /**
     * @brief Retrieves the list of data items.
     *
     * @return A constant reference to the vector of `DataItem` structures.
     */
    [[nodiscard]] const std::vector<DataItem> &items() const;

    /**
     * @brief Retrieves details about a specific data item by index.
     *
     * @param item_idx The zero-based index of the data item.
     * @return A `DataItem` structure containing details of the specified item.
     */
    [[nodiscard]] DataItem getItem(unsigned int item_idx) const;

    /**
     * @brief Retrieves a specific data item by its data ID.
     *
     * @param data_id The `EKORDDataID` of the data item to retrieve.
     * @param item Output parameter to store the retrieved `DataItem`.
     * @return `true` if the data item was found and retrieved; `false` otherwise.
     */
    [[nodiscard]] bool getItem(EKORDDataID data_id, DataItem &item) const;

    /**
     * @brief Retrieves the byte offset of a specific data item by its data ID.
     *
     * @param data_id The `EKORDDataID` of the data item.
     * @return The byte offset as an `unsigned int`.
     */
    [[nodiscard]] unsigned int getOffset(EKORDDataID data_id) const;

    /**
     * @brief Retrieves the type of a specific data item by its data ID.
     *
     * @param data_id The `EKORDDataID` of the data item.
     * @return The `EKORDType` of the data item.
     */
    [[nodiscard]] EKORDType getType(EKORDDataID data_id) const;

    /**
     * @brief Retrieves the maximum length of data supported by the format description.
     *
     * @return The maximum data length as a `size_t`.
     */
    [[nodiscard]] size_t getMaxDataLength() const;

    /**
     * @brief Serializes the data format description into a byte buffer.
     *
     * @param buffer The `std::vector<uint8_t>` to which the serialized data will be appended.
     */
    void serialize(std::vector<uint8_t> &buffer);

    /**
     * @brief Deserializes the data format description from a byte buffer.
     *
     * @param buffer The `std::vector<uint8_t>` containing the serialized data.
     */
    void deserialize(std::vector<uint8_t> buffer);

    /**
     * @brief Creates a data format description for a status frame.
     *
     * @param version The version of the status frame format. Defaults to the latest version.
     * @return A `DataFormatDescription` instance configured for a status frame.
     */
    static DataFormatDescription makeStatusFrameDescription(unsigned int version = KORD_STATUS_VERSION_LATEST);

    /**
     * @brief Creates a data format description for a specific item and version.
     *
     * @param item_id The `EKORDItemID` of the item.
     * @param version The version of the item format.
     * @return A `DataFormatDescription` instance configured for the specified item and version.
     */
    static DataFormatDescription makeItemDescription(EKORDItemID item_id, int version);

    /**
     * @brief Creates a data format description for a specific item using the latest version.
     *
     * @param item_id The `EKORDItemID` of the item.
     * @return A `DataFormatDescription` instance configured for the specified item with the latest version.
     */
    static DataFormatDescription makeItemDescriptionLatest(EKORDItemID item_id);

private:
    /**
     * @brief Internal implementation class.
     *
     * The `Internals` class hides the implementation details of `DataFormatDescription`
     * using the Pimpl idiom.
     */
    class Internals;

    Internals *his_; /**< Pointer to the internal implementation */
};

} // namespace kr2::kord::protocol

#endif // KR2_KORD_DATA_DESCRIPTION_FORMAT_H
