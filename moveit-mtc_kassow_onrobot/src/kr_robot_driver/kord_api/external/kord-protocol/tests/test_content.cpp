#include <kord/protocol/ContentFrameBuilder.h>
#include <kord/protocol/ContentFrameParser.h>
#include <kord/protocol/ContentItem.h>
#include <kord/protocol/DataFormatDescription.h>
#include <kord/protocol/KORDFrames.h>
#include <kord/protocol/JointFirmwareCommand.h>
#include <kord/protocol/DataDescriptions/Requests/ControlCommandItems.h>

#define CRCPP_INCLUDE_ESOTERIC_CRC_DEFINITIONS
#include <CRC.h>

#include <numeric>
#include <array>
#include <chrono>

#include <gtest/gtest.h>

using namespace kr2::kord::protocol;

TEST(ContentItemManipulation, contentReadWrite){
    std::vector<uint8_t> data_in{10, 11, 13, 15, 17, 19};
    ContentItem test(EKORDItemID::eNone, data_in.data(), data_in.size());
    ContentItem copy = test;
}

TEST(ContentItemManipulation, testSlefAssign){
    std::vector<uint8_t> data_in{10, 11, 13, 15, 17, 19};
    ContentItem test(EKORDItemID::eNone, data_in.data(), data_in.size());
    test = test;
}

TEST(ContentItemManipulation, testCopyContruction){
    std::vector<uint8_t> data_in{10, 11, 13, 15, 17, 19};
    ContentItem test(EKORDItemID::eNone, data_in.data(), data_in.size());
}

TEST(ContentItemManipulation, creatingItemOneByOne){
    struct {    
        uint16_t sequence_number_;
        uint32_t tx_ts_ns_uh_;
        uint32_t tx_ts_ns_lh_;
        double joint_1_position_;
        double joint_2_position_;
        double joint_3_position_;
        double joint_4_position_;
        double joint_5_position_;
        double joint_6_position_;
        double joint_7_position_;
        uint16_t crc_;
    }__attribute__((packed)) cmd_move_j;

    memset(&cmd_move_j, 0x00, sizeof(cmd_move_j));
    cmd_move_j.sequence_number_ = 55;
    cmd_move_j.joint_4_position_ = 1.568;
    cmd_move_j.crc_ = 55678;

    // Add Move J Command
    ContentItem content_item_1(EKORDItemID::eCommandMoveJ);
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.sequence_number_));
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.tx_ts_ns_uh_));
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.tx_ts_ns_lh_));
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.joint_1_position_));
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.joint_2_position_));
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.joint_3_position_));
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.joint_4_position_));
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.joint_5_position_));
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.joint_6_position_));
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.joint_7_position_));
    EXPECT_TRUE(content_item_1.addData(cmd_move_j.crc_));
    EXPECT_EQ(EKORDItemID::eCommandMoveJ, content_item_1.getItemID());
    EXPECT_EQ(sizeof(cmd_move_j), content_item_1.getItemDataLength());

}

TEST(ContentItemManipulation, creatingItemByOffset){
    DataFormatDescription movej_dfd = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveJ);
    using a7d = std::array<double, 7>;

    std::array<double, 7UL> joints{0.2, 0.3, 0.5, 0.7, 0.11, 0.13, 0.17};
    unsigned int seq_num = 55;
    bool success = false;
    // Add Move J Command
    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_1(EKORDItemID::eCommandMoveJ);
    success = content_item_1.addData<uint16_t>(seq_num, movej_dfd.getOffset(EKORDDataID::eSequenceNumber)); EXPECT_TRUE(success);
    success = content_item_1.addData<std::array<double, 7>>(joints, movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)); EXPECT_TRUE(success);
    success = content_item_1.addData<uint8_t>(0x55, movej_dfd.getOffset(EKORDDataID::eTOverlayType)); EXPECT_TRUE(success);
    std::uint32_t crc = CRC::Calculate(content_item_1.getItemData(), content_item_1.getItemDataLength(), CRC::CRC_16_MODBUS());
    success = content_item_1.addData<uint16_t>(crc, movej_dfd.getOffset(EKORDDataID::eCRCValue)); EXPECT_TRUE(success);
    EXPECT_EQ(movej_dfd.getMaxDataLength(), content_item_1.getItemDataLength());

    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_2(
        content_item_1.getItemID(), 
        content_item_1.getItemData(),
        content_item_1.getItemDataLength()
    );

    EXPECT_EQ(EKORDItemID::eCommandMoveJ, content_item_2.getItemID());
    EXPECT_EQ(seq_num, content_item_2.getData<uint16_t>(movej_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(joints, content_item_2.getData<a7d>(movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)));
    EXPECT_EQ(0x55, content_item_2.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTOverlayType)));
}

TEST(ContentItemManipulation, creatingItemByOffsetSizeCheckFull){
    DataFormatDescription movej_dfd = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveJ);
    using a7d = std::array<double, 7>;

    std::array<double, 7UL> joints{0.2, 0.3, 0.5, 0.7, 0.11, 0.13, 0.17};
    unsigned int seq_num = 55;
    bool success = false;
    // Add Move J Command
    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_1(EKORDItemID::eCommandMoveJ);
    success = content_item_1.addData<uint16_t>(seq_num, movej_dfd.getOffset(EKORDDataID::eSequenceNumber)); EXPECT_TRUE(success);
    success = content_item_1.addData<std::array<double, 7>>(joints, movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)); EXPECT_TRUE(success);
    // writing the last emelent in the array
    success = content_item_1.addData<uint8_t>(0x55, movej_dfd.getOffset(EKORDDataID::eTOverlayType)); EXPECT_TRUE(success);
    // writing the pre last element
    success = content_item_1.addData<uint8_t>(0x55, movej_dfd.getOffset(EKORDDataID::eTMovementType)); EXPECT_TRUE(success);
    std::uint32_t crc = CRC::Calculate(content_item_1.getItemData(), content_item_1.getItemDataLength(), CRC::CRC_16_MODBUS());
    success = content_item_1.addData<uint16_t>(crc, movej_dfd.getOffset(EKORDDataID::eCRCValue)); EXPECT_TRUE(success);
    EXPECT_LE(movej_dfd.getMaxDataLength(), content_item_1.getItemDataLength());

    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_2(
        content_item_1.getItemID(), 
        content_item_1.getItemData(),
        content_item_1.getItemDataLength()
    );

    EXPECT_EQ(EKORDItemID::eCommandMoveJ, content_item_2.getItemID());
    EXPECT_EQ(seq_num, content_item_2.getData<uint16_t>(movej_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(joints, content_item_2.getData<a7d>(movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)));
    EXPECT_EQ(0x55, content_item_2.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTOverlayType)));
}

TEST(ContentItemManipulation, creatingItemByOffsetSizeCheckLess){
    DataFormatDescription movej_dfd = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveJ);
    using a7d = std::array<double, 7>;

    std::array<double, 7UL> joints{0.2, 0.3, 0.5, 0.7, 0.11, 0.13, 0.17};
    unsigned int seq_num = 55;
    bool success = false;
    // Add Move J Command
    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_1(EKORDItemID::eCommandMoveJ);
    success = content_item_1.addData<uint16_t>(seq_num, movej_dfd.getOffset(EKORDDataID::eSequenceNumber)); EXPECT_TRUE(success);
    success = content_item_1.addData<std::array<double, 7>>(joints, movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)); EXPECT_TRUE(success);
    success = content_item_1.addData<uint8_t>(0x55, movej_dfd.getOffset(EKORDDataID::eTMovementType)); EXPECT_TRUE(success);
    EXPECT_LE(movej_dfd.getOffset(EKORDDataID::eTMovementType) + sizeof(uint8_t), content_item_1.getItemDataLength());

    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_2(
        content_item_1.getItemID(), 
        content_item_1.getItemData(),
        content_item_1.getItemDataLength()
    );

    EXPECT_EQ(EKORDItemID::eCommandMoveJ, content_item_2.getItemID());
    EXPECT_EQ(seq_num, content_item_2.getData<uint16_t>(movej_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(joints, content_item_2.getData<a7d>(movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)));
    EXPECT_EQ(0x55, content_item_2.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTMovementType)));
    EXPECT_LE(movej_dfd.getOffset(EKORDDataID::eTMovementType) + sizeof(uint8_t), content_item_2.getItemDataLength());
}

TEST(ContentItemManipulation, defaultConstruction){
    // Add Move J Command
    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_1;
    DataFormatDescription movej_dfd = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveJ);

    EXPECT_EQ(EKORDItemID::eNone, content_item_1.getItemID());
    EXPECT_EQ(0x0000, content_item_1.getData<uint16_t>(movej_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(0x00, content_item_1.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTMovementType)));
    EXPECT_LE(0, content_item_1.getItemDataLength());
}

TEST(ContentItemManipulation, alteringDefaultConstructed){
    DataFormatDescription movej_dfd = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveJ);
    using a7d = std::array<double, 7>;

    std::array<double, 7UL> joints{0.2, 0.3, 0.5, 0.7, 0.11, 0.13, 0.17};
    unsigned int seq_num = 55;
    bool success = false;
    // Add Move J Command
    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_1;
    
    // Check it is empty
    EXPECT_EQ(EKORDItemID::eNone, content_item_1.getItemID());
    EXPECT_EQ(0x0000, content_item_1.getData<uint16_t>(movej_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(0x00, content_item_1.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTMovementType)));
    EXPECT_LE(0, content_item_1.getItemDataLength());

    // Fill it with data
    content_item_1.setItemID(EKORDItemID::eCommandMoveJ);
    success = content_item_1.addData<uint16_t>(seq_num,             movej_dfd.getOffset(EKORDDataID::eSequenceNumber)); EXPECT_TRUE(success);
    success = content_item_1.addData<std::array<double, 7>>(joints, movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)); EXPECT_TRUE(success);
    success = content_item_1.addData<uint8_t>(0x55,                 movej_dfd.getOffset(EKORDDataID::eTMovementType)); EXPECT_TRUE(success);
    
    // Chec data is valid
    EXPECT_EQ(EKORDItemID::eCommandMoveJ, content_item_1.getItemID());
    EXPECT_EQ(seq_num, content_item_1.getData<uint16_t>(            movej_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(joints, content_item_1.getData<a7d>(                  movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)));
    EXPECT_EQ(0x55, content_item_1.getData<uint8_t>(                movej_dfd.getOffset(EKORDDataID::eTMovementType)));
    EXPECT_LE(movej_dfd.getOffset(EKORDDataID::eTMovementType) + sizeof(uint8_t), content_item_1.getItemDataLength());

    // Clear the Item
    content_item_1.clear();

    // Check it is empty
    EXPECT_EQ(EKORDItemID::eNone, content_item_1.getItemID());
    EXPECT_EQ(0x0000, content_item_1.getData<uint16_t>(movej_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(0x00, content_item_1.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTMovementType)));
    EXPECT_LE(0, content_item_1.getItemDataLength());
}

TEST(ContentItemManipulation, defaultConstructionCopy){
    DataFormatDescription movej_dfd = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveJ);
    using a7d = std::array<double, 7>;

    std::array<double, 7UL> joints{0.2, 0.3, 0.5, 0.7, 0.11, 0.13, 0.17};
    unsigned int seq_num = 55;
    bool success = false;
    // Add Move J Command
    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_1(EKORDItemID::eCommandMoveJ);
    success = content_item_1.addData<uint16_t>(seq_num, movej_dfd.getOffset(EKORDDataID::eSequenceNumber)); EXPECT_TRUE(success);
    success = content_item_1.addData<std::array<double, 7>>(joints, movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)); EXPECT_TRUE(success);
    success = content_item_1.addData<uint8_t>(0x55, movej_dfd.getOffset(EKORDDataID::eTMovementType)); EXPECT_TRUE(success);
    EXPECT_LE(movej_dfd.getOffset(EKORDDataID::eTMovementType) + sizeof(uint8_t), content_item_1.getItemDataLength());

    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_2(
        content_item_1.getItemID(), 
        content_item_1.getItemData(),
        content_item_1.getItemDataLength()
    );

    EXPECT_EQ(EKORDItemID::eCommandMoveJ, content_item_2.getItemID());
    EXPECT_EQ(seq_num, content_item_2.getData<uint16_t>(movej_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(joints, content_item_2.getData<a7d>(movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)));
    EXPECT_EQ(0x55, content_item_2.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTMovementType)));
    EXPECT_LE(movej_dfd.getOffset(EKORDDataID::eTMovementType) + sizeof(uint8_t), content_item_2.getItemDataLength());

    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_3;
    EXPECT_EQ(EKORDItemID::eNone, content_item_3.getItemID());
    content_item_3 = content_item_2;
    EXPECT_EQ(EKORDItemID::eCommandMoveJ, content_item_3.getItemID());
    EXPECT_EQ(seq_num, content_item_3.getData<uint16_t>(movej_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(joints, content_item_3.getData<a7d>(movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)));
    EXPECT_EQ(0x55, content_item_3.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTMovementType)));
    EXPECT_LE(movej_dfd.getOffset(EKORDDataID::eTMovementType) + sizeof(uint8_t), content_item_3.getItemDataLength());

}

TEST(ContentItemManipulation, testCopyAssing){
    using namespace kr2::kord;
    std::array<uint8_t, 16UL> data;

    std::iota(data.begin(), data.end(), 1);
    
    ContentItem<GetContentFrameSize<KORDContentFrame>()> ci1(EKORDItemID::eCommandMoveJ, data.data(), data.size());

    for (auto &d : data){
        EXPECT_TRUE(ci1.addData<uint8_t>(d));
    }
}

TEST(ContentManipulation, contentBuilderConstruction){
    ContentFrameBuilder<KORDContentFrame> cb1;
    ContentFrameBuilder<KORDContentFrame> cb2 = cb1;
}

TEST(ContentManipulation, contentBuilderAssignment){
    ContentFrameBuilder<KORDContentFrame> cb1;
    ContentFrameBuilder<KORDContentFrame> cb2;

    cb2 = cb1;
}

TEST(ContentManipulation, contentBuilderSelfAssign){
    ContentFrameBuilder<KORDContentFrame> cb1;
    cb1 = cb1;
}

TEST(ContentManipulation, contentParserConstruction){
    ContentFrameParser<KORDContentFrame> cp1;
    ContentFrameParser<KORDContentFrame> cp2 = cp1;
}

TEST(ContentManipulation, contentParserAssignment){
    ContentFrameParser<KORDContentFrame> cp1;
    ContentFrameParser<KORDContentFrame> cp2;

    cp2 = cp1;
}

TEST(ContentManipulation, contentParserSelfAssign){
    ContentFrameParser<KORDContentFrame> cp1;

    cp1 = cp1;
}

TEST(ContentManipulation, contentAddContentItem1){
    std::vector<uint8_t> data_in{10, 11, 13, 15, 17, 19};
    ContentItem<GetContentFrameSize<KORDContentFrame>()> test(EKORDItemID::eCustomData, data_in.data(), data_in.size());
    ContentFrameBuilder<KORDContentFrame> cb1;
    uint8_t buffer[MAX_ETH_DATA_LEN_B];

    EXPECT_TRUE(cb1.addContentItem(test));
    EXPECT_TRUE(cb1.getPayload(buffer, MAX_ETH_DATA_LEN_B));
    

}

TEST(ContentManipulation, contentGetContentItem1){
    std::vector<uint8_t> data_in{10, 11, 13, 15, 17, 19};
    ContentItem<GetContentFrameSize<KORDContentFrame>()> test(EKORDItemID::eCustomData, data_in.data(), data_in.size());
    KORDContentFrame buffer; buffer.reset();
    ContentFrameBuilder<KORDContentFrame> cb1;
    ContentFrameParser<KORDContentFrame> cp1;

    EXPECT_TRUE(cb1.addContentItem(test));
    EXPECT_TRUE(cb1.getPayload(reinterpret_cast<uint8_t*>(&buffer), buffer.getFrameLength()));

    EXPECT_TRUE(cp1.setFromPayload(reinterpret_cast<uint8_t*>(&buffer), buffer.getFrameLength()));
    EXPECT_EQ(1, cp1.getItemsCount());

    std::vector<uint8_t>data;
    data.resize(6);
    ContentItem<GetContentFrameSize<KORDContentFrame>()> read = cp1.getItemContent(0);
    EXPECT_EQ(EKORDItemID::eCustomData, read.getItemID());
    EXPECT_EQ(data_in.size(), read.getItemDataLength());
    memcpy(data.data(),read.getItemData(), read.getItemDataLength());
    EXPECT_EQ(data, data_in);

}

TEST(ContentManipulation, contentCommandJointFirmware){
    using namespace kr2::kord;

    DataFormatDescription cmd_joint_fw = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandJointFirmware);
    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item;
    bool success = false;
    std::array<EJointFirmwareCommand, 7UL> jnt_commands{
        EJointFirmwareCommand::eNone,
        EJointFirmwareCommand::eNone,
        EJointFirmwareCommand::eNone,
        EJointFirmwareCommand::eNone,
        EJointFirmwareCommand::eBrakeEngage,
        EJointFirmwareCommand::eBrakeEngage,
        EJointFirmwareCommand::eBrakeEngage
    };
    
    uint16_t seq_num = 56;
    int64_t tx_ts = std::chrono::system_clock::now().time_since_epoch().count();

    content_item.setItemID(EKORDItemID::eCommandJointFirmware);
    success = content_item.addData(seq_num, cmd_joint_fw.getOffset(EKORDDataID::eSequenceNumber)); EXPECT_TRUE(success);
    success = content_item.addData(tx_ts, cmd_joint_fw.getOffset(EKORDDataID::eTxStamp)); EXPECT_TRUE(success);
    success = content_item.addData(jnt_commands, cmd_joint_fw.getOffset(EKORDDataID::eJControlCMD)); EXPECT_TRUE(success);
    std::uint32_t crc = CRC::Calculate(content_item.getItemData(), content_item.getItemDataLength(), CRC::CRC_16_MODBUS()); EXPECT_TRUE(success);
    success = content_item.addData<uint16_t>(crc, cmd_joint_fw.getOffset(EKORDDataID::eCRCValue)); EXPECT_TRUE(success);
}

TEST(ContentManipulation, contentMultipleItems1){
    using namespace kr2::kord;

    DataFormatDescription movej_dfd = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveJ);
    DataFormatDescription movel_dfd = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandMoveL);
    DataFormatDescription cmd_joint_fw = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eCommandJointFirmware);
    ContentFrameBuilder<KORDContentFrame> content_builder; content_builder.clear();
    ContentFrameParser<KORDContentFrame> content_parser; content_parser.clear();
    KORDContentFrame content_frame; content_frame.reset();
    using a7d = std::array<double, 7UL>;
    using a6d = std::array<double, 6UL>;
    using a7i = std::array<unsigned int, 7UL>;

    struct {
        a7d joints{0.0, 0.5, 0.0, 1.0, 0.0, 1.57, 0.0};
        uint8_t mask = 0x01;
        uint8_t tracking_type = 3;    //  TT_NONE=0, TT_TIME=1, TT_WS_TARGET_SPEED=2, TT_JS_TARGET_SPEED=3, TT_SP_APX_SPEED=4, TT_SP_CNST_SPEED=5, TT_SP_DURATION=6
        double tracking_value = 0.50; // ~ 30deg/sec
        uint8_t blend_type = 2; // BT_NONE=0, BT_TIME=1, BT_WS_ACCELERATION=2, BT_WS_RADIUS=3, BT_JS_ACCELERATION=4
        double blend_value = 1.0; // ~60deg/sec2
        uint8_t overlay_type = 2; //  OT_NONE=0, OT_VIAPOINT, OT_STOPPOINT 
    } move_joints;

    struct {
        a6d tcp{0.3, 0.3, 0.4, 0.5, 0.0, 1.57};
        uint8_t mask = 0x01;
        uint8_t tracking_type = 2; //  TT_NONE=0, TT_TIME, TT_WS_TARGET_SPEED, TT_JS_TARGET_SPEED, TT_SP_APX_SPEED, TT_SP_CNST_SPEED, TT_SP_DURATION
        double tracking_value = 0.2; // m/s
        uint8_t blend_type = 2; // BT_NONE=0, BT_TIME, BT_WS_ACCELERATION, BT_WS_RADIUS, BT_JS_ACCELERATION
        double blend_value = 0.5; // 
        uint8_t overlay_type = 2; //  OT_NONE=0, OT_VIAPOINT, OT_STOPPOINT 
    } move_linear;

    unsigned int seq_num = 55;

    int64_t tx_ts = std::chrono::system_clock::now().time_since_epoch().count();
    std::array<EJointFirmwareCommand, 7UL> jnt_commands{
        EJointFirmwareCommand::eNone,
        EJointFirmwareCommand::eNone,
        EJointFirmwareCommand::eNone,
        EJointFirmwareCommand::eNone,
        EJointFirmwareCommand::eBrakeEngage,
        EJointFirmwareCommand::eBrakeEngage,
        EJointFirmwareCommand::eBrakeEngage
    };

    bool success = false;
    // Add Move J Command
    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_1(EKORDItemID::eCommandMoveJ);
    success = content_item_1.addData<uint16_t>(seq_num,                         movej_dfd.getOffset(EKORDDataID::eSequenceNumber)); EXPECT_TRUE(success);
    success = content_item_1.addData<uint8_t>(move_joints.mask,                 movej_dfd.getOffset(EKORDDataID::eCTRMovementMask)); EXPECT_TRUE(success);
    success = content_item_1.addData<std::array<double, 7>>(move_joints.joints, movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)); EXPECT_TRUE(success);
    success = content_item_1.addData<uint8_t>(move_joints.tracking_type,        movej_dfd.getOffset(EKORDDataID::eTMovementType)); EXPECT_TRUE(success);
    success = content_item_1.addData<double>(move_joints.tracking_value,        movej_dfd.getOffset(EKORDDataID::eTMovementValue)); EXPECT_TRUE(success);
    success = content_item_1.addData<uint8_t>(move_joints.blend_type,           movej_dfd.getOffset(EKORDDataID::eTBlendType)); EXPECT_TRUE(success);
    success = content_item_1.addData<double>(move_joints.blend_value,           movej_dfd.getOffset(EKORDDataID::eTBlendValue)); EXPECT_TRUE(success);
    success = content_item_1.addData<uint8_t>(move_joints.overlay_type,         movej_dfd.getOffset(EKORDDataID::eTOverlayType)); EXPECT_TRUE(success);
    std::uint32_t crc = CRC::Calculate(content_item_1.getItemData(), content_item_1.getItemDataLength(), CRC::CRC_16_MODBUS());EXPECT_TRUE(success);
    success = content_item_1.addData<uint16_t>(crc, movej_dfd.getOffset(EKORDDataID::eCRCValue)); EXPECT_TRUE(success);
    EXPECT_EQ(movej_dfd.getMaxDataLength(), content_item_1.getItemDataLength());
    EXPECT_TRUE(content_builder.addContentItem(content_item_1));

    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_2(EKORDItemID::eCommandMoveL);
    success = content_item_2.addData<uint16_t>(seq_num,                         movel_dfd.getOffset(EKORDDataID::eSequenceNumber)); EXPECT_TRUE(success);
    success = content_item_2.addData<uint8_t>(move_linear.mask,                 movel_dfd.getOffset(EKORDDataID::eCTRMovementMask)); EXPECT_TRUE(success);
    success = content_item_2.addData<std::array<double, 6UL>>(move_linear.tcp,  movel_dfd.getOffset(EKORDDataID::eFrmTCPPose)); EXPECT_TRUE(success);
    success = content_item_2.addData<uint8_t>(move_linear.tracking_type,        movel_dfd.getOffset(EKORDDataID::eTMovementType)); EXPECT_TRUE(success);
    success = content_item_2.addData<double>(move_linear.tracking_value,        movel_dfd.getOffset(EKORDDataID::eTMovementValue)); EXPECT_TRUE(success);
    success = content_item_2.addData<uint8_t>(move_linear.blend_type,           movel_dfd.getOffset(EKORDDataID::eTBlendType)); EXPECT_TRUE(success);
    success = content_item_2.addData<double>(move_linear.blend_value,           movel_dfd.getOffset(EKORDDataID::eTBlendValue)); EXPECT_TRUE(success);
    success = content_item_2.addData<uint8_t>(move_linear.overlay_type,         movel_dfd.getOffset(EKORDDataID::eTOverlayType)); EXPECT_TRUE(success);
    crc = CRC::Calculate(content_item_2.getItemData(), content_item_2.getItemDataLength(), CRC::CRC_16_MODBUS()); EXPECT_TRUE(success);
    success = content_item_2.addData<uint16_t>(crc, movel_dfd.getOffset(EKORDDataID::eCRCValue)); EXPECT_TRUE(success);
    EXPECT_EQ(movel_dfd.getMaxDataLength(), content_item_2.getItemDataLength());
    EXPECT_TRUE(content_builder.addContentItem(content_item_2));

    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_3(EKORDItemID::eCommandJointFirmware);
    success = content_item_3.addData<uint16_t>(seq_num, cmd_joint_fw.getOffset(EKORDDataID::eSequenceNumber)); EXPECT_TRUE(success);
    success = content_item_3.addData(tx_ts,             cmd_joint_fw.getOffset(EKORDDataID::eTxStamp)); EXPECT_TRUE(success);
    success = content_item_3.addData(jnt_commands,      cmd_joint_fw.getOffset(EKORDDataID::eJControlCMD)); EXPECT_TRUE(success);
    crc = CRC::Calculate(content_item_3.getItemData(), content_item_3.getItemDataLength(), CRC::CRC_16_MODBUS());EXPECT_TRUE(success);
    success = content_item_3.addData<uint16_t>(crc, cmd_joint_fw.getOffset(EKORDDataID::eCRCValue)); EXPECT_TRUE(success);
    EXPECT_EQ(cmd_joint_fw.getMaxDataLength(), content_item_3.getItemDataLength());
    EXPECT_TRUE(content_builder.addContentItem(content_item_3));

    EXPECT_TRUE(content_builder.getPayload(reinterpret_cast<uint8_t*>(&content_frame), content_frame.getFrameLength()));
    EXPECT_TRUE(content_parser.setFromPayload(reinterpret_cast<uint8_t*>(&content_frame), content_frame.getFrameLength()));

    EXPECT_EQ(3, content_parser.getItemsCount());

    ContentItem<GetContentFrameSize<KORDContentFrame>()> read_content_item_1 = content_parser.getItemContent(0);
    ContentItem<GetContentFrameSize<KORDContentFrame>()> read_content_item_2 = content_parser.getItemContent(1);
    ContentItem<GetContentFrameSize<KORDContentFrame>()> read_content_item_3 = content_parser.getItemContent(2);

    EXPECT_EQ(EKORDItemID::eCommandMoveJ, read_content_item_1.getItemID());
    EXPECT_EQ(seq_num,                      read_content_item_1.getData<uint16_t>(movej_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(move_joints.joints,           read_content_item_1.getData<a7d>(movej_dfd.getOffset(EKORDDataID::eJConfigurationArm)));
    EXPECT_EQ(move_joints.mask,             read_content_item_1.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eCTRMovementMask)));
    EXPECT_EQ(move_joints.tracking_type,    read_content_item_1.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTMovementType)));
    EXPECT_EQ(move_joints.tracking_value,   read_content_item_1.getData<double>(movej_dfd.getOffset(EKORDDataID::eTMovementValue)));
    EXPECT_EQ(move_joints.blend_type,       read_content_item_1.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTBlendType)));
    EXPECT_EQ(move_joints.blend_value,      read_content_item_1.getData<double>(movej_dfd.getOffset(EKORDDataID::eTBlendValue)));
    EXPECT_EQ(move_joints.overlay_type,     read_content_item_1.getData<uint8_t>(movej_dfd.getOffset(EKORDDataID::eTOverlayType)));
    
    
    EXPECT_EQ(EKORDItemID::eCommandMoveL, read_content_item_2.getItemID());
    EXPECT_EQ(seq_num,                      read_content_item_2.getData<uint16_t>(movel_dfd.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(move_linear.tcp,              read_content_item_2.getData<a6d>(movel_dfd.getOffset(EKORDDataID::eFrmTCPPose)));
    EXPECT_EQ(move_linear.mask,             read_content_item_2.getData<uint8_t>(movel_dfd.getOffset(EKORDDataID::eCTRMovementMask)));
    EXPECT_EQ(move_linear.tracking_type,    read_content_item_2.getData<uint8_t>(movel_dfd.getOffset(EKORDDataID::eTMovementType)));
    EXPECT_EQ(move_linear.tracking_value,   read_content_item_2.getData<double>(movel_dfd.getOffset(EKORDDataID::eTMovementValue)));
    EXPECT_EQ(move_linear.blend_type,       read_content_item_2.getData<uint8_t>(movel_dfd.getOffset(EKORDDataID::eTBlendType)));
    EXPECT_EQ(move_linear.blend_value,      read_content_item_2.getData<double>(movel_dfd.getOffset(EKORDDataID::eTBlendValue)));
    EXPECT_EQ(move_linear.overlay_type,     read_content_item_2.getData<uint8_t>(movel_dfd.getOffset(EKORDDataID::eTOverlayType)));

    EXPECT_EQ(EKORDItemID::eCommandJointFirmware, read_content_item_3.getItemID());
    EXPECT_EQ(seq_num, read_content_item_3.getData<uint16_t>(cmd_joint_fw.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_EQ(tx_ts, read_content_item_3.getData<int64_t>(cmd_joint_fw.getOffset(EKORDDataID::eTxStamp)));
    a7i cmds = read_content_item_3.getData<a7i>(cmd_joint_fw.getOffset(EKORDDataID::eJControlCMD));
    EXPECT_EQ(static_cast<uint32_t>(jnt_commands[0]), cmds[0]);
    EXPECT_EQ(static_cast<uint32_t>(jnt_commands[1]), cmds[1]);
    EXPECT_EQ(static_cast<uint32_t>(jnt_commands[2]), cmds[2]);
    EXPECT_EQ(static_cast<uint32_t>(jnt_commands[3]), cmds[3]);
    EXPECT_EQ(static_cast<uint32_t>(jnt_commands[4]), cmds[4]);
    EXPECT_EQ(static_cast<uint32_t>(jnt_commands[5]), cmds[5]);
    EXPECT_EQ(static_cast<uint32_t>(jnt_commands[6]), cmds[6]);
    EXPECT_EQ(crc, read_content_item_3.getData<uint16_t>(cmd_joint_fw.getOffset(EKORDDataID::eCRCValue)));
}

TEST(ContentManipulation, systemRequest_TransferLogFiles){
    using namespace kr2::kord;

    DataFormatDescription request_system = DataFormatDescription::makeItemDescriptionLatest(EKORDItemID::eRequestSystem);
    ContentItem<GetContentFrameSize<KORDContentFrame>()> tx_item, rx_item;
    ContentFrameBuilder<KORDContentFrame> builder;
    ContentFrameParser<KORDContentFrame> parser;
    std::array<uint8_t, sizeof(KORDContentFrame)> buffer; 
    buffer.fill(0x00);

    // Mock data
    uint16_t seq_num = 1245;
    int64_t tx_ts = std::chrono::system_clock::now().time_since_epoch().count();
    EControlCommandItems cmd = EControlCommandItems::eTransferLogFiles;

    //
    // Mock transmission
    //
    tx_item.clear();
    tx_item.setItemID(EKORDItemID::eRequestSystem);
    EXPECT_TRUE(tx_item.addData(seq_num, request_system.getOffset(EKORDDataID::eSequenceNumber)));
    EXPECT_TRUE(tx_item.addData(tx_ts, request_system.getOffset(EKORDDataID::eTxStamp)));
    EXPECT_TRUE(tx_item.addData(cmd, request_system.getOffset(EKORDDataID::eCTRCommandItem)));
    std::uint32_t crc = CRC::Calculate(tx_item.getItemData(), tx_item.getItemDataLength(), CRC::CRC_16_MODBUS());
    
    EXPECT_TRUE(tx_item.addData<uint16_t>(crc, request_system.getOffset(EKORDDataID::eCRCValue)));

    EXPECT_EQ(request_system.getMaxDataLength(), tx_item.getItemDataLength());
    EXPECT_TRUE(builder.addContentItem(tx_item));
    EXPECT_TRUE(builder.getPayload(buffer.data(), buffer.size()));

    //
    //Mock reception
    //
    EXPECT_TRUE(parser.setFromPayload(buffer.data(), buffer.size()));
    EXPECT_EQ(parser.getItemsCount(), 1);
    rx_item = parser.getItemContent(0);

    EXPECT_EQ(rx_item.getItemID(), EKORDItemID::eRequestSystem);
    EXPECT_EQ(rx_item.getData<uint16_t>(request_system.getOffset(EKORDDataID::eSequenceNumber)), seq_num);
    EXPECT_EQ(rx_item.getData<int64_t>(request_system.getOffset(EKORDDataID::eTxStamp)),          tx_ts);
    EXPECT_EQ(rx_item.getData<uint16_t>(request_system.getOffset(EKORDDataID::eCTRCommandItem)), EControlCommandItems::eTransferLogFiles);
    EXPECT_EQ(rx_item.getData<uint16_t>(request_system.getOffset(EKORDDataID::eCRCValue)), crc);

}