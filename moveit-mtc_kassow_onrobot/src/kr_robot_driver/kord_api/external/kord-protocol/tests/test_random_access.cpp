#include <kord/protocol/KORDFrames.h>
#include <kord/protocol/StatusFrameBuilder.h>
#include <kord/protocol/StatusFrameParser.h>
#include <kord/protocol/ContentWriter.h>
#include <kord/protocol/ContentReader.h>
#include <kord/protocol/KORDDataIDs.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

#include <gtest/gtest.h>

using namespace kr2::kord::protocol;

std::ostream &operator<<(std::ostream &out, std::array<double,7> data) {
    for (auto &it : data){
        out << it << ", ";
    }
    out << " sizeof: " << sizeof(data);
    out << "\n";

    return out;
};

struct TestData{
    TestData(std::array<double, 7> aa, std::array<double, 7>bb, int cc, double dd, int ee, double ff):
        q(aa),
        qd(bb),
        ai(cc),
        a(dd),
        bi(ee),
        b(ff)
    {}

    TestData():
        q{},
        qd{},
        ai(0),
        a(0.0),
        bi(0),
        b(0.0)
    {}

    std::string asString(){
        std::stringstream ss;

        ss << "q: " << q << "qd: " << qd;
        ss << " ai: " << ai;
        ss << " a: " << a;
        ss << " bi: " << bi;
        ss << " b: " << b << "\n";

        return ss.str();
    }

    bool operator==(const TestData &rhs) const{
        bool equality = true;

        equality &= (this->q == rhs.q);
        equality &= (this->qd == rhs.qd);
        equality &= (this->ai == rhs.ai);
        equality &= (this->a == rhs.a);
        equality &= (this->bi == rhs.bi);
        equality &= (this->b == rhs.b);

        return equality;
    }

    std::array<double, 7> q;
    std::array<double, 7> qd;
    int ai;
    double a;
    int bi;
    double b;
};


TEST(TestWriterReader, dataRandomAccess){

    uint8_t mymem[2000];
    memset(mymem, 0x00, 2000);
    TestData testdwrite({0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7}, {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7}, 2, 5.0, 3, 6.0);
    TestData testdread;

    std::cout << testdwrite.asString();
    std::cout << testdread.asString();

    ContentWriter wr(mymem, 2000);
    wr.addData(testdwrite.q);
    wr.addData(testdwrite.qd);
    wr.addData(testdwrite.ai);
    wr.addData(testdwrite.a);
    wr.addData(testdwrite.bi);
    wr.addData(testdwrite.b);

    ContentReader rd(mymem, 2000);
    testdread.q = rd.getData<std::array<double, 7>>();
    testdread.qd = rd.getData<std::array<double, 7>>();
    testdread.ai = rd.getData<int>();
    testdread.a = rd.getData<double>();
    testdread.bi = rd.getData<int>();
    testdread.b = rd.getData<double>();
    std::cout << testdread.asString();

    EXPECT_EQ(testdread, testdwrite);


    KORDFrame kord_frame;
    StatusFrameBuilder status_builder(250);
    StatusFrameParser status;

    // Test data
    std::array<double, 7> a{1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7};
    std::array<double, 7> b{2.1, 3.2, 4.3, 5.4, 6.5, 7.6, 8.7};

    std::array<double, 7> x, y, z;

    kord_frame.frame_id_ = KORD_FRAME_ID_STATUS;
    kord_frame.session_id_ = 0;
    
    status_builder.addData(a);
    status_builder.addData(b);
    status_builder.addData<int>(10);
    status_builder.addData(10.45);
    status_builder.addData(b);

    EXPECT_TRUE(status_builder.getPayload(kord_frame.payload_, sizeof(kord_frame.payload_)));

    EXPECT_TRUE(status.setFromPayload(kord_frame.payload_, sizeof(kord_frame.payload_)));

    x = status.getData<std::array<double, 7>>();
    y = status.getData<std::array<double, 7>>();
    std::array<double, 7> test_y;
    test_y = status.getData<std::array<double, 7>>(56);
    std::cout << y;
    std::cout << test_y;
    EXPECT_EQ(test_y, y);
    EXPECT_EQ(test_y[2], y[2]);
    EXPECT_NE(test_y[2], 0);
    EXPECT_EQ(test_y[2], 4.3);

    int r1 = status.getData<int>();
    double r2 = status.getData<double>();
    z = status.getData<std::array<double, 7>>();
    
    EXPECT_EQ(a, x);
    EXPECT_EQ(b, y);
    EXPECT_EQ(r1, 10);
    EXPECT_EQ(r2, 10.45);
    EXPECT_EQ(b, z);
}