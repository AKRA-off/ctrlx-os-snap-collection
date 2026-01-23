#include <kord/protocol/KORDFrames.h>
#include <kord/protocol/ContentFrameBuilder.h>
#include <kord/protocol/ContentFrameParser.h>
#include <kord/protocol/KORDItemIDs.h>

#include <iostream>
#include <vector>

#include <gtest/gtest.h>

struct CommandIOControl{
    uint16_t outputs_;
    uint16_t mapping_;
    double analog_out1_;
    double analog_out2_;
}__attribute__((packed));

std::ostream &operator<<(std::ostream &out, std::array<double,7> data) {
    for (auto &it : data){
        out << it << ", ";
    }
    out << " sizeof: " << sizeof(data);
    out << "\n";

    return out;
};

TEST(TestContentFrames, test1){
    using namespace kr2::kord::protocol;
    KORDFrame kord_frame;

    ContentFrameBuilder<KORDContentFrame> content_builder;
    ContentFrameParser<KORDContentFrame> content_parser;

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

        void reset(){
            memset(this, 0x00, 56+8+4);
        }

        inline uint16_t getLength() const {
            return 56+8+4;
        }

    }__attribute__((packed)) cmd_move_j, cmd_rcv_j;

    cmd_move_j.reset();
    cmd_rcv_j.reset();
    cmd_move_j.sequence_number_ = 55;
    cmd_move_j.joint_4_position_ = 1.568;
    cmd_move_j.crc_ = 55678;

    CommandIOControl io_control, io_rcv;
    memset(&io_control, 0x00, sizeof(io_control));
    memset(&io_rcv, 0x00, sizeof(io_rcv));
    io_control.mapping_ = 0x0105;
    io_control.analog_out1_ = 12.5;

    // Add Move J Command
    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_1(EKORDItemID::eCommandMoveJ);
    content_item_1.addData(cmd_move_j.sequence_number_);
    content_item_1.addData(cmd_move_j.tx_ts_ns_uh_);
    content_item_1.addData(cmd_move_j.tx_ts_ns_lh_);
    content_item_1.addData(cmd_move_j.joint_1_position_);
    content_item_1.addData(cmd_move_j.joint_2_position_);
    content_item_1.addData(cmd_move_j.joint_3_position_);
    content_item_1.addData(cmd_move_j.joint_4_position_);
    content_item_1.addData(cmd_move_j.joint_5_position_);
    content_item_1.addData(cmd_move_j.joint_6_position_);
    content_item_1.addData(cmd_move_j.joint_7_position_);
    content_item_1.addData(cmd_move_j.crc_);
    EXPECT_TRUE(content_builder.addContentItem(content_item_1));
    
    // Add IO Control command
    ContentItem<GetContentFrameSize<KORDContentFrame>()> content_item_2(EKORDItemID::eCommandSetIOAnalogOut);
    content_item_2.addData(io_control.outputs_);
    content_item_2.addData(io_control.mapping_);
    content_item_2.addData(io_control.analog_out1_);
    content_item_2.addData(io_control.analog_out2_);
    EXPECT_TRUE(content_builder.addContentItem(content_item_2));

    EXPECT_TRUE(content_builder.getPayload(kord_frame.payload_, sizeof(kord_frame.payload_)));
    EXPECT_TRUE(content_builder.clear());

    content_parser.setFromPayload(kord_frame.payload_, sizeof(KORDContentFrame));
    
    EXPECT_EQ(2,                                              content_parser.getItemsCount());
    EXPECT_EQ(EKORDItemID::eCommandMoveJ,          content_parser.getItemID(0));
    EXPECT_EQ(EKORDItemID::eCommandSetIOAnalogOut, content_parser.getItemID(1));

    ContentItem read_content_item_1 = content_parser.getItemContent(0);
    ContentItem read_content_item_2 = content_parser.getItemContent(1);

    EXPECT_EQ(EKORDItemID::eCommandMoveJ,          read_content_item_1.getItemID());
    EXPECT_EQ(EKORDItemID::eCommandSetIOAnalogOut, read_content_item_2.getItemID());

    
    EXPECT_EQ(sizeof(cmd_move_j), read_content_item_1.getItemDataLength());
    memcpy(&cmd_rcv_j, read_content_item_1.getItemData(), read_content_item_1.getItemDataLength());

    EXPECT_EQ(cmd_rcv_j.sequence_number_, 55);
    EXPECT_EQ(cmd_rcv_j.joint_4_position_, 1.568);
    EXPECT_EQ(cmd_rcv_j.crc_, 55678);

    cmd_rcv_j.reset();

    EXPECT_EQ(cmd_rcv_j.sequence_number_, 0);
    EXPECT_EQ(cmd_rcv_j.joint_4_position_, 0.0);
    EXPECT_EQ(cmd_rcv_j.crc_, 0);


    cmd_rcv_j.sequence_number_ = read_content_item_1.getData<uint16_t>();
    cmd_rcv_j.tx_ts_ns_uh_ = read_content_item_1.getData<uint32_t>();
    cmd_rcv_j.tx_ts_ns_lh_ = read_content_item_1.getData<uint32_t>();
    cmd_rcv_j.joint_1_position_ = read_content_item_1.getData<double>();
    cmd_rcv_j.joint_2_position_ = read_content_item_1.getData<double>();
    cmd_rcv_j.joint_3_position_ = read_content_item_1.getData<double>();
    cmd_rcv_j.joint_4_position_ = read_content_item_1.getData<double>();
    cmd_rcv_j.joint_5_position_ = read_content_item_1.getData<double>();
    cmd_rcv_j.joint_6_position_ = read_content_item_1.getData<double>();
    cmd_rcv_j.joint_7_position_ = read_content_item_1.getData<double>();
    cmd_rcv_j.crc_ = read_content_item_1.getData<uint16_t>();

    EXPECT_EQ(cmd_rcv_j.sequence_number_, 55);
    EXPECT_EQ(cmd_rcv_j.joint_4_position_, 1.568);
    EXPECT_EQ(cmd_rcv_j.crc_, 55678);

    io_rcv.outputs_ = read_content_item_2.getData<uint16_t>();
    io_rcv.mapping_ = read_content_item_2.getData<uint16_t>();
    io_rcv.analog_out1_ = read_content_item_2.getData<double>();
    io_rcv.analog_out2_ = read_content_item_2.getData<double>();

    EXPECT_EQ(io_rcv.mapping_, 0x0105);
    EXPECT_EQ(io_rcv.analog_out1_, 12.5);

    content_parser.clear();
}
