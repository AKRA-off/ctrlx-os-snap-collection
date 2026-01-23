#include <kord/protocol/ContentWriter.h>
#include <kord/protocol/ContentReader.h>

#include <array>

#include <gtest/gtest.h>

using namespace kr2::kord::protocol;


TEST(ContentReadWriteTest, RandomAccessWriteRead){
    uint8_t mem[256];
    ContentWriter mywriter(mem, 256);
    ContentReader myreader(mem, 256);

    std::array<unsigned int, 7UL> tval{2, 3, 5, 7, 11, 13, 17};

    EXPECT_TRUE(mywriter.addData(tval, 10));
    
    std::array<unsigned int, 7UL> rval = myreader.getData<std::array<unsigned int, 7>>(10);

    EXPECT_EQ(tval, rval);
    EXPECT_EQ(rval[6], 17);
    EXPECT_EQ(rval[3],  7);
}