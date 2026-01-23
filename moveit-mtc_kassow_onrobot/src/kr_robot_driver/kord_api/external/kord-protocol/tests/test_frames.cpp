#include <kord/protocol/KORDFrames.h>

#include <iostream>
#include <vector>

#include <gtest/gtest.h>

using namespace kr2::kord::protocol;

TEST(KordFrames, checkSize){

    KORDFrame kord_frame;
    KORDContentFrame content_frame;
    KORDStatusFrame status_frame;

    EXPECT_EQ(MAX_ETH_DATA_LEN_B, sizeof(kord_frame));
    EXPECT_EQ(1442, sizeof(content_frame));
    EXPECT_EQ(1442, sizeof(status_frame));

    EXPECT_EQ(1442, content_frame.getFrameLength());
    EXPECT_EQ(1312, content_frame.getDataLength());

    EXPECT_EQ(1442, status_frame.getFrameLength());
    EXPECT_EQ(1436, status_frame.getDataLength());    
}

