#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>

using namespace kr2;

int main()
{
    // Create an instance of KordCore for handling RX/TX KORD frames.
    std::shared_ptr<kord::KordCore> kord(new kord::KordCore("192.168.39.1", 7582, 1, kord::UDP_CLIENT));

    // Initialize Control and Receiver Interfaces.
    kord::ControlInterface ctl_iface(kord);
    kord::ReceiverInterface rcv_iface(kord);

    // Establish connection to the robot's CBun.
    if (!kord->connect()) {
        std::cout << "Connecting to KR failed\n";
        return EXIT_FAILURE;
    }

    // Send ArmStatus request to initiate control session.
    if (!kord->syncRC()){
        std::cout << "Sync RC failed.\n";
        return EXIT_FAILURE;
    }

    std::cout << "Sync Captured \n";

    // Fetch and retrieve joint positions.
    rcv_iface.fetchData();
    std::array<double, 7UL> start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

    // Print initial joint configuration in degrees.
    std::cout << "Read initial joint configuration:\n";
    for(double angle : start_q)
        std::cout << (angle / 3.14) * 180 << " ";
    std::cout << "\n";

    return 0;
}
