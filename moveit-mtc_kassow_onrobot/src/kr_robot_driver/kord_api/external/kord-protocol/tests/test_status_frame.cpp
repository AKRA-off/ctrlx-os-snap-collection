#include <kord/protocol/KORDFrames.h>
#include <kord/protocol/StatusFrameBuilder.h>
#include <kord/protocol/StatusFrameParser.h>
#include <kord/protocol/ContentWriter.h>
#include <kord/protocol/ContentReader.h>
#include <kord/protocol/DataFormatDescription.h>
#include <kord/protocol/KORDItemIDs.h>
#include <kord/protocol/DataDescriptions/Requests/ControlCommandItems.h>
#include <kord/protocol/DataDescriptions/Requests/ControlCommandStatus.h>
#include <kord/system/SystemEvent.h>

#include <iostream>
#include <sstream>
#include <chrono>
#include <random>
#include <array>
#include <vector>
#include <map>
#include <functional>

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


TEST(TestStatusFrame, test1){
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
    int r1 = status.getData<int>();
    double r2 = status.getData<double>();
    z = status.getData<std::array<double, 7>>();
    
    EXPECT_EQ(a, x);
    EXPECT_EQ(b, y);
    EXPECT_EQ(r1, 10);
    EXPECT_EQ(r2, 10.45);
    EXPECT_EQ(b, z);

}

TEST(TestSystemEvents, testPackUnpack){
    SystemEvent tx_event, rx_event;
    std::vector<uint8_t> buffer;
    std::mt19937_64 gen64;
    std::mt19937 gen32;

    tx_event.timestamp_ = gen64();
    tx_event.event_id_ =  gen32();
    tx_event.event_group_ = static_cast<uint16_t>(gen32());

    EXPECT_FALSE(tx_event == rx_event);

    tx_event.toByteArray(buffer);

    EXPECT_EQ(buffer.size(), sizeof(tx_event));

    EXPECT_TRUE(rx_event.initFromByteArray(buffer));

    EXPECT_EQ(tx_event, rx_event);
}

TEST(TestStatusFrame, completeStatusFrame){

    StatusFrameBuilder status_bldr;
    StatusFrameParser status_prsr;
    KORDStatusFrame kord_status_frame{};
    DataFormatDescription status_dfd = DataFormatDescription::makeStatusFrameDescription(0);
    std::mt19937_64 gen64;
    std::mt19937 gen32;
    std::uniform_real_distribution<> joint_range(-2.0*M_PI, 2.0*M_PI);
    std::uniform_real_distribution<> master_speed_range(0, 1.0);

    using SystemEventsArray = std::array<SystemEvent, 18>;
    using Vector7d = std::array<double, 7ul>;
    using Vector6d = std::array<double, 6ul>;
    using Vector7i = std::array<uint32_t, 7ul>;
    using Vector6i = std::array<uint32_t, 6ul>;
    using Vector7f = std::array<float, 7ul>;

    std::vector<uint8_t> tx_buffer;
    std::vector<uint8_t> rx_buffer;

    SystemEvent event;
    event.timestamp_ = gen64();
    event.event_id_ = 1;
    event.event_group_ = 100;
    SystemEventsArray tx_events_array;
    SystemEventsArray rx_events_array;
    for (auto& item : tx_events_array){
        item = event;
        event.timestamp_ = gen64();
        event.event_group_++;
        event.event_id_++;
    }

    std::function gen7d([&]()->std::array<double,7ul>{
         return {
            joint_range(gen64),
            joint_range(gen64),
            joint_range(gen64),
            joint_range(gen64),
            joint_range(gen64),
            joint_range(gen64),
            joint_range(gen64)
        };
    });

    std::function gen7i([&]()->std::array<uint32_t,7ul>{
         return {
             static_cast<uint32_t>(gen32()),
         static_cast<uint32_t>(gen32()),
         static_cast<uint32_t>(gen32()),
         static_cast<uint32_t>(gen32()),
         static_cast<uint32_t>(gen32()),
         static_cast<uint32_t>(gen32()),
         static_cast<uint32_t>(gen32())
        };
    });

    std::function gen7f([&]()->std::array<float,7ul>{
         return {
            static_cast<float>(joint_range(gen64)),
            static_cast<float>(joint_range(gen64)),
            static_cast<float>(joint_range(gen64)),
            static_cast<float>(joint_range(gen64)),
            static_cast<float>(joint_range(gen64)),
            static_cast<float>(joint_range(gen64)),
            static_cast<float>(joint_range(gen64))
        };
    });

    std::function<std::array<double,6ul>()> gen6d([&]()->std::array<double,6ul>{
         return {
            joint_range(gen64),
            joint_range(gen64),
            joint_range(gen64),
            joint_range(gen64),
            joint_range(gen64),
            joint_range(gen64)
        };
    });

    std::map<EKORDDataID, int64_t> map_64_ids = {
        {EKORDDataID::eTxStampEcho, gen64()},
        {EKORDDataID::eTxStamp, gen64()},
        {EKORDDataID::eCTRCommandTS, gen64()}};

    std::map<EKORDDataID, Vector7d> map_v7d_ids = {
        {EKORDDataID::eJConfigurationArm, gen7d()},
        {EKORDDataID::eJSpeedArm, gen7d()},
        {EKORDDataID::eJAccelerationArm, gen7d()},
        {EKORDDataID::eJTorqueArm, gen7d()},
        {EKORDDataID::eJStatorVoltage, gen7d()},
        {EKORDDataID::eJStatorCurrent, gen7d()}};

    std::map<EKORDDataID, uint32_t> map_32_ids = {
        {EKORDDataID::eFailToReadError, gen32()},
        {EKORDDataID::eFailToReadEmpty, gen32()},
        {EKORDDataID::eRCSafetyFlag, gen32()},
        {EKORDDataID::eRCSafetyMode, gen32()},
        {EKORDDataID::eRCMotionFlags, gen32()}};

    std::map<EKORDDataID, Vector7f> map_v7f_ids = {
        {EKORDDataID::eJTorqueDeviation, gen7f()}};

    Vector7i eJErrorBits = gen7i();
    Vector7i eJStatusBits = gen7i();

    for (auto i =0; i < 7; i++){
        std::cout << "eJErrorBits[" << i << "]: " << eJErrorBits[i] << std::endl;
    }

    Vector6d eFrmTCPPose = gen6d(); 
    Vector6d eFrmTFCModel = gen6d();

    double eRCMasterSpeed = master_speed_range(gen64);

    uint64_t eIODigitalInput = gen64();
    uint64_t eIODigitalOutput = gen64();
    uint16_t eCTRCommandItem = EControlCommandItems::eTransferLogFiles;

    uint16_t eCTRCommandStatus = EControlCommandStatus::eSuccess;

    auto eCRCValue = static_cast<uint16_t>(gen32());
    std::cout << "<<<<<< offset: " << status_dfd.getOffset(EKORDDataID::eJErrorBits) << std::endl;
    status_bldr.addData<Vector7i>(eJErrorBits, status_dfd.getOffset(EKORDDataID::eJErrorBits));
    status_bldr.addData<Vector7i>(eJStatusBits, status_dfd.getOffset(EKORDDataID::eJStatusBits));

    status_bldr.addData<Vector6d>(eFrmTCPPose, status_dfd.getOffset(EKORDDataID::eFrmTCPPose));
    status_bldr.addData<Vector6d>(eFrmTFCModel, status_dfd.getOffset(EKORDDataID::eFrmTFCModel));

    status_bldr.addData<double>(eRCMasterSpeed, status_dfd.getOffset(EKORDDataID::eRCMasterSpeed));

    status_bldr.addData<uint64_t>(eIODigitalInput, status_dfd.getOffset(EKORDDataID::eIODigitalInput));
    status_bldr.addData<uint64_t>(eIODigitalOutput, status_dfd.getOffset(EKORDDataID::eIODigitalOutput));

    status_bldr.addData<uint16_t>(eCTRCommandItem, status_dfd.getOffset(EKORDDataID::eCTRCommandItem));
    status_bldr.addData<uint16_t>(eCTRCommandStatus, status_dfd.getOffset(EKORDDataID::eCTRCommandStatus));
    status_bldr.addData<uint16_t>(eCRCValue, status_dfd.getOffset(EKORDDataID::eCRCValue));

    for (auto item : map_64_ids) {
        if (!status_bldr.addData(item.second, status_dfd.getOffset(item.first))) {
            std::cerr << "Failed to add data to the builder for item: " << static_cast<int>(item.first) << std::endl;
        }
    }

    for (auto item : map_v7d_ids) {
        status_bldr.addData<Vector7d>(item.second, status_dfd.getOffset(item.first));
    }

    for (auto item : map_32_ids) {
        status_bldr.addData<uint32_t>(item.second, status_dfd.getOffset(item.first));
    }

    for (auto item : map_v7f_ids) {
        status_bldr.addData<Vector7f>(item.second, status_dfd.getOffset(item.first));
    }

    tx_buffer.reserve(256);
    tx_buffer.clear();
    for (auto item : tx_events_array){
        item.toByteArray(tx_buffer);
    }

    EXPECT_EQ(18*sizeof(SystemEvent), tx_buffer.size());
    EXPECT_TRUE(status_bldr.addData(tx_buffer, status_dfd.getOffset(EKORDDataID::eEventsArray)));

    status_bldr.addSequenceNumber(5);

    EXPECT_TRUE(status_bldr.getPayload(reinterpret_cast<uint8_t*>(&kord_status_frame), sizeof(kord_status_frame)));
    EXPECT_TRUE(status_prsr.setFromPayload(reinterpret_cast<uint8_t*>(&kord_status_frame), sizeof(kord_status_frame)));

    std::cout << "<<<<<< offset: " << status_dfd.getOffset(EKORDDataID::eJErrorBits) << std::endl;

    for (auto i =0; i < 7; i++){
        std::cout << "eJErrorBits[" << i << "]: " << status_prsr.getData<Vector7i>( status_dfd.getOffset(EKORDDataID::eJErrorBits))[i] << std::endl;
    }

    auto v = status_prsr.getData<Vector7i>( status_dfd.getOffset(EKORDDataID::eJErrorBits));
    EXPECT_EQ(eJErrorBits, status_prsr.getData<Vector7i>( status_dfd.getOffset(EKORDDataID::eJErrorBits)));
    EXPECT_EQ(eJStatusBits, status_prsr.getData<Vector7i>( status_dfd.getOffset(EKORDDataID::eJStatusBits)));
    EXPECT_EQ(eFrmTCPPose, status_prsr.getData<Vector6d>( status_dfd.getOffset(EKORDDataID::eFrmTCPPose)));
    EXPECT_EQ(eFrmTFCModel, status_prsr.getData<Vector6d>( status_dfd.getOffset(EKORDDataID::eFrmTFCModel)));
    EXPECT_EQ(eRCMasterSpeed, status_prsr.getData<double>(status_dfd.getOffset(EKORDDataID::eRCMasterSpeed)));
    EXPECT_EQ(eIODigitalInput, status_prsr.getData<uint64_t>( status_dfd.getOffset(EKORDDataID::eIODigitalInput)));
    EXPECT_EQ(eIODigitalOutput, status_prsr.getData<uint64_t>( status_dfd.getOffset(EKORDDataID::eIODigitalOutput)));
    EXPECT_EQ(eCTRCommandStatus, status_prsr.getData<uint16_t>( status_dfd.getOffset(EKORDDataID::eCTRCommandStatus)));
    EXPECT_EQ(eCRCValue, status_prsr.getData<uint16_t>( status_dfd.getOffset(EKORDDataID::eCRCValue)));
    EXPECT_EQ(eCTRCommandItem, status_prsr.getData<uint16_t>(status_dfd.getOffset(EKORDDataID::eCTRCommandItem)));

    for (auto item : map_64_ids) {
        EXPECT_EQ(item.second, status_prsr.getData<int64_t>(status_dfd.getOffset(item.first)));
    }

    for (auto item : map_v7d_ids) {
        EXPECT_EQ(item.second, status_prsr.getData<Vector7d>(status_dfd.getOffset(item.first)));
    }

    for (auto item : map_32_ids) {
        EXPECT_EQ(item.second, status_prsr.getData<uint32_t>(status_dfd.getOffset(item.first)));
    }

    for (auto item : map_v7f_ids) {
        EXPECT_EQ(item.second, status_prsr.getData<Vector7f>(status_dfd.getOffset(item.first)));
    }

    EXPECT_TRUE(status_prsr.getData(rx_buffer, status_dfd.getOffset(EKORDDataID::eEventsArray)));
    EXPECT_EQ(18*sizeof(SystemEvent), rx_buffer.size());

    for (auto &item : rx_events_array){
        item.initFromByteArray(rx_buffer);
        rx_buffer.erase(rx_buffer.begin(), rx_buffer.begin()+sizeof(SystemEvent));
    }

    for (auto tx_it = tx_events_array.begin(), rx_it = rx_events_array.begin(); tx_it != tx_events_array.end() && rx_it != rx_events_array.end(); rx_it++, tx_it++){
        EXPECT_EQ(*tx_it, *rx_it);
    }
    // SytemEventsArray rx_events;
    // rx_events.
    // status_prsr.getData<SystemEventsArray>(status_dfd.getOffset(EKORDDataID::eEventsArray));

}

TEST(SystemEventsStructTest, basicMethods) {

    SystemEvent se1;
    se1.timestamp_ = 1666882555276119949;
    se1.event_id_  = 20;
    se1.event_group_ = 30;

    SystemEvent se2;
    se2.timestamp_ = 1666882647938090568;
    se2.event_id_  = 20;
    se2.event_group_ = 30;

    SystemEvent se3;
    se3.timestamp_ = 1666882825096147732;
    se3.event_id_  = 20;
    se3.event_group_ = 30;

    SystemEvent se4;
    se4.timestamp_ = 1666883202760102496;
    se4.event_id_  = 20;
    se4.event_group_ = 30;

    SystemEvents ses;

    ses.addEvent(se1);
    ses.addEvent(se2);
    ses.addEvent(se3);
    ses.addEvent(se4);

    // Expect equality.
    EXPECT_TRUE(ses.isEventPresent(se1)) << "Event1 not in the list";
    EXPECT_TRUE(ses.isEventPresent(se2)) << "Event2 not in the list";

    EXPECT_EQ(ses.events_.size(), 4) << "Oh no the size is not 4!";

    std::cerr << "[          ] t1 " << se1.timestamp_ << std::endl;
    std::cerr << "[          ] t2 " << se2.timestamp_ << std::endl;
    std::cerr << "[          ] t1 " << se1.toString() << std::endl;
    std::cerr << "[          ] t2(t1) " << se2.toStringWithRef(se1, 1e-9) << std::endl;
}