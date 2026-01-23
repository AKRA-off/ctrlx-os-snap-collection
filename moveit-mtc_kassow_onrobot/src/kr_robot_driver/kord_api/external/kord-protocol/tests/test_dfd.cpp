#include <kord/protocol/KORDFrames.h>
#include <kord/protocol/StatusFrameBuilder.h>
#include <kord/protocol/StatusFrameParser.h>
#include <kord/protocol/ContentWriter.h>
#include <kord/protocol/ContentReader.h>
#include <kord/protocol/KORDDataIDs.h>
#include <kord/protocol/DataFormatDescription.h>

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


TEST(TestDataFormatDescription, test1){

    DataFormatDescription dfd;

    // std::cout << "Size of DFD: " << sizeof(dfd) << "\n";
    // std::cout << "DFD str: " << dfd.asString() << "\n";

    dfd.setID(KORD_FRAME_ID_STATUS)
        .addItem(EKORDDataID::eJConfigurationArm)
        .addItem(EKORDDataID::eJSpeedArm)
        .addItem(EKORDDataID::eJAccelerationArm)
        .addItem(EKORDDataID::eJTorqueArm)
        .addItem(EKORDDataID::eJSupplyVoltage)
        .addItem(EKORDDataID::eJErrorBits)
        .addItem(EKORDDataID::eJStatusBits)
        .addItem(EKORDDataID::eJTemperatureBoard);

    // std::cout << "Size of DFD: " << sizeof(dfd) << "\n";

    // std::cout << "DFD str: " << dfd.asString() << "\n";

    DataFormatDescription sdfd = DataFormatDescription::makeStatusFrameDescription(0);
    
    // std::cout << "Size of DFD: " << sizeof(sdfd) << "\n";
    // std::cout << "SDFD ID: " << sdfd.ID() << "\n";
    // std::cout << "SDFD ID: " << sdfd.asString() << "\n";
    // std::cout << "DFD ID: " << dfd.ID() << "\n";

    EXPECT_EQ(dfd.ID(), KORD_FRAME_ID_STATUS);
    EXPECT_EQ( dfd.getItemsCount(), 8);

    DataItem di_invalid = dfd.getItem(-1);
    EXPECT_EQ(di_invalid.did_, UINT32_MAX);
    
    // Get Item by offset
    DataItem di = dfd.getItem(6);
    EXPECT_EQ(di.did_, static_cast<unsigned int>(EKORDDataID::eJStatusBits));
    EXPECT_EQ(di.type_, EKORDType::eVector7i);
    EXPECT_EQ(di.offset_, 308);

    // Get offset and type by ID
    EXPECT_EQ(dfd.getOffset(EKORDDataID::eJStatusBits), 308);
    EXPECT_EQ(dfd.getType(EKORDDataID::eJStatusBits), EKORDType::eVector7i);

    // Get Item existing item by ID
    EXPECT_EQ(dfd.getItem(EKORDDataID::eJSupplyVoltage, di), true);
    EXPECT_EQ(di.did_, static_cast<unsigned int>(EKORDDataID::eJSupplyVoltage));
    EXPECT_EQ(di.type_, EKORDType::eVector7d);
    EXPECT_EQ(di.offset_, 224);

    // Get Item non-existing item by ID
    EXPECT_EQ(dfd.getItem(EKORDDataID::eFrmTCPPose, di), false);
    EXPECT_EQ(di.did_, static_cast<unsigned int>(EKORDDataID::eJSupplyVoltage));
    EXPECT_EQ(di.type_, EKORDType::eVector7d);
    EXPECT_EQ(di.offset_, 224);

}