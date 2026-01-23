#include <kord/protocol/StatusFrameBuilder.h>
#include <kord/protocol/StatusFrameParser.h>
#include <kord/protocol/DataFormatDescription.h>
#include <kord/protocol/KORDFrames.h>

#include <gtest/gtest.h>

using namespace kr2::kord::protocol;

TEST(StatusReadWrite, structureTest){
    StatusFrameBuilder sbuilder;
    StatusFrameParser sparser;
    KORDStatusFrame frame;

    frame.reset();
    EXPECT_TRUE(sbuilder.getPayload(reinterpret_cast<uint8_t*>(&frame), frame.getFrameLength()));
    EXPECT_TRUE(sparser.setFromPayload(reinterpret_cast<uint8_t*>(&frame), frame.getFrameLength()));
}

TEST(StatusProcessing, structureTestParser1){
    StatusFrameParser sparser1;
    StatusFrameParser sparser2 = sparser1;

    EXPECT_EQ(sparser2.getSequenceNumber(), sparser1.getSequenceNumber());
    EXPECT_EQ(sparser2.getSequenceNumber(), 0);
    EXPECT_EQ(sparser1.getSequenceNumber(), 0);
}

TEST(StatusProcessing, structureTestParser2){
    StatusFrameParser sparser1;
    sparser1 = sparser1;

    EXPECT_EQ(sparser1.getFrequency(), 0);
}

TEST(StatusProcessing, structureTestParser3){
    StatusFrameParser sparser1;
    StatusFrameParser sparser2;

    sparser2 = sparser1;

    EXPECT_EQ(sparser2.getSequenceNumber(), sparser1.getSequenceNumber());
    EXPECT_EQ(sparser2.getSequenceNumber(), 0);
    EXPECT_EQ(sparser1.getSequenceNumber(), 0);
}

TEST(StatusProcessing, structureTestBuilder1){
    StatusFrameBuilder sbuild1;
    StatusFrameBuilder sbuild2 = sbuild1;
}

TEST(StatusProcessing, structureTestBuilder2){
    StatusFrameBuilder sbuild1;
    sbuild1 = sbuild1;
}

TEST(StatusProcessing, structureTestBuilder3){
    StatusFrameBuilder sbuild1;
    StatusFrameBuilder sbuild2;

    sbuild2 = sbuild1;
}

TEST(StatusProcessing, accessViaDescription){
    StatusFrameParser sparser;
    StatusFrameBuilder sbuild;
    KORDStatusFrame frame;
    DataFormatDescription sdfd = DataFormatDescription::makeStatusFrameDescription(0);

    EXPECT_TRUE(
        sbuild.addData(
            std::array<double, 7UL>{2.,3.,5.,7.,11.,13.,17.}, 
            sdfd.getOffset(EKORDDataID::eJConfigurationArm)
        )
    );

    EXPECT_TRUE(
        sbuild.addData<unsigned int>(
            0x5555,
            sdfd.getOffset(EKORDDataID::eRCMotionFlags)
        )
    );

    EXPECT_TRUE(
        sbuild.getPayload(reinterpret_cast<uint8_t*>(&frame), frame.getFrameLength())
    );

    EXPECT_TRUE(
        sparser.setFromPayload(reinterpret_cast<uint8_t*>(&frame), frame.getFrameLength())
    );

    std::array<double, 7UL> out = sparser.getData<std::array<double, 7UL>>(sdfd.getOffset(EKORDDataID::eJConfigurationArm));
    unsigned int mf = sparser.getData<unsigned int>(sdfd.getOffset(EKORDDataID::eRCMotionFlags));
    std::array<double, 7UL> trq = sparser.getData<std::array<double, 7UL>>(sdfd.getOffset(EKORDDataID::eJTorqueArm));

    EXPECT_NEAR(out[0], 2.0, 1e-9);
    EXPECT_NEAR(out[6], 17.0, 1e-9);
    EXPECT_EQ(mf, 0x5555);
    EXPECT_EQ(trq[0], 0.0);
    EXPECT_EQ(trq[1], 0.0);
    EXPECT_EQ(trq[2], 0.0);
    EXPECT_EQ(trq[3], 0.0);
    EXPECT_EQ(trq[4], 0.0);
    EXPECT_EQ(trq[5], 0.0);
    EXPECT_EQ(trq[6], 0.0);
}