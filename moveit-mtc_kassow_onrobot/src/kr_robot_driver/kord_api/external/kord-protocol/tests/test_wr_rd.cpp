
#include <kord/protocol/ContentWriter.h>
#include <kord/protocol/ContentReader.h>

#include <iostream>
#include <sstream>

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


TEST(TestManipulation, addDataWriter){
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
}