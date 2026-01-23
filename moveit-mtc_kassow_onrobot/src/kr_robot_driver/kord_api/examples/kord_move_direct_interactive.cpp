#include <chrono>
#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

#define NCURSES_NOMACROS
#include <ncurses.h>

#include <kord/api/kord.h>
#include <kord/api/kord_control_interface.h>
#include <kord/api/kord_receive_interface.h>
#include <kord/utils/utils.h>

#include <kord/polynomials/from_constraints.h>

using namespace kr2;
using namespace kswx_weaving_generator;
static bool g_run = true;

class InteractiveExample {
private:
    enum EVENT { NO_ARROW_PRESSED, LEFT_ARROW_HOLD, RIGHT_ARROW_HOLD };
    enum STATE { IDLE, SPEED_UP, SPEED_DOWN, CONST_SPEED };

    STATE now_state = STATE::IDLE;

    std::atomic<bool> keep_running_{};
    std::atomic<bool> positive_speed_{};
    std::thread calc_;
    std::mutex mux_capture_;

    const double ts = 0.002; // 500 Hz, for 250Hz choose 0.004
    const double small_move_duration = 0.2;
    const double small_move_ramp_pose_step = 0.005;
    const double const_speed = 0.05;

    double now_pose = 0;

    std::chrono::steady_clock::time_point traj_start;
    Degree5Polynomial ramp_;
    std::array<double, 7UL> start_q{};
    std::array<double, 7UL> q{}, qd{}, qdd{}, torque{};

    std::shared_ptr<kord::KordCore> kord;

    Degree5Polynomial rampInit(double a_duration, double start_pose, double now_speed, double target_speed)
    {
        auto x0 = start_pose;
        auto dx0 = now_speed;
        auto ddx0 = 0;

        double x1;
        x1 = start_pose + small_move_ramp_pose_step;
        if (!positive_speed_) {
            x1 = start_pose - small_move_ramp_pose_step;
        }
        double dx1 = target_speed;
        double ddx1 = 0;

        auto polynomial = PolynomialFromConstraints_C2At2Points_0A()
                              .setPoint(a_duration)
                              .setValues(x0, x1)
                              .setDerivatives(dx0, dx1)
                              .setSecondDerivatives(ddx0, ddx1)
                              .compute();
        return polynomial;
    }

    void handleStop()
    {
        switch (now_state) {
        case CONST_SPEED: {
            printw("\nStopping smoothly...");
            auto target_speed = const_speed;
            if (!positive_speed_) {
                target_speed *= -1;
            }
            mux_capture_.lock();
            auto cur_pose = now_pose;
            mux_capture_.unlock();
            auto new_polynom = rampInit(small_move_duration, cur_pose, target_speed, 0);
            mux_capture_.lock();
            traj_start = std::chrono::steady_clock::now();
            ramp_ = new_polynom;
            now_state = STATE::SPEED_DOWN;
            mux_capture_.unlock();
            break;
        }
        default: {
            break;
        }
        }
    }

    void handleArrow(bool is_left)
    {
        double target_speed = const_speed;
        if (is_left) {
            target_speed = -1 * const_speed;
        }
        switch (now_state) {
        case IDLE: {
            printw("\nStarting the ramp...");
            if (target_speed < 0)
                positive_speed_ = false;
            else
                positive_speed_ = true;
            mux_capture_.lock();
            auto cur_pose = now_pose;
            mux_capture_.unlock();
            auto new_polynom = rampInit(small_move_duration, cur_pose, 0, target_speed);

            mux_capture_.lock();
            traj_start = std::chrono::steady_clock::now();
            ramp_ = new_polynom;

            now_state = STATE::SPEED_UP;
            mux_capture_.unlock();

            break;
        }
        default:
            break;
        }
    }

    void handleEvent(EVENT new_event)
    {
        if (std::chrono::steady_clock::now() - traj_start > std::chrono::milliseconds(int(small_move_duration * 1000))) {
            switch (now_state) {
            case SPEED_UP: {
                printw("\nChanging to constant speed mode...");
                mux_capture_.lock();
                now_state = STATE::CONST_SPEED;
                mux_capture_.unlock();
                break;
            }
            case SPEED_DOWN: {
                mux_capture_.lock();
                now_state = STATE::IDLE;
                mux_capture_.unlock();
                break;
            }
            default: {
                break;
            }
            }
        }
        switch (new_event) {
        case NO_ARROW_PRESSED:
            // printw("\nStop event");
            handleStop();
            break;
        case LEFT_ARROW_HOLD:
            // printw("\nLeft event");
            handleArrow(true);
            break;
        case RIGHT_ARROW_HOLD:
            // printw("\nRight event");
            handleArrow(false);
            break;
        }
    }

    void moving(kord::ControlInterface &ctl_iface,
                kord::ReceiverInterface &rcv_iface,
                const std::shared_ptr<kord::KordCore> &a_kord)
    {
        unsigned int k = 0; // main time counter

        while (keep_running_) {
            mux_capture_.lock();
            now_pose = q[0] - start_q[0];
            auto polynomial = ramp_;
            auto capt_traj_start = traj_start;
            auto cur_state = now_state;
            mux_capture_.unlock();
            auto time_offset = std::chrono::duration_cast<std::chrono::duration<double>>(
                std::chrono::steady_clock::now() - capt_traj_start);
            if (cur_state == STATE::CONST_SPEED) { // constant speed - not based on polynomial, setting manually
                auto target_speed = const_speed;
                if (!positive_speed_)
                    target_speed *= -1;
                for (size_t i = 0; i < 7; i++) {
                    q[i] = now_pose + target_speed * ts + start_q[i];
                    qd[i] = target_speed;
                    qdd[i] = 0;
                    torque[i] = 0.0; // it will be computed automatically in case of zero
                }
            }
            else {
                if (time_offset > std::chrono::milliseconds(int(small_move_duration * 1000))) {
                    // doing nothing, stopped IDLE state
                }
                else {
                    for (size_t i = 0; i < 7; i++) {
                        q[i] = polynomial.valueAt(time_offset.count()) + start_q[i];
                        qd[i] = polynomial.derivativeAt(time_offset.count());
                        qdd[i] = polynomial.secondDerivativeAt(time_offset.count());
                        torque[i] = 0.0; // it will be computed automatically in case of zero
                    }
                }
            }

            // torque = {-3.0, -16., 0.15, 11.2, 0.7, -0.36, 0.5}; // some torque for testing
            k++;

            if (!a_kord->waitSync(std::chrono::milliseconds(10))) {
                printw("\nSync wait timed out, exit");
                break;
            }

            ctl_iface.directJControl(q, qd, qdd, torque);

            rcv_iface.fetchData();
            if (rcv_iface.systemAlarmState()) {
                break;
            }
        }
        keep_running_ = false;
        stop();
    }

    void runCalcAndEventsCatch()
    {
        initscr();            // Initialize the ncurses library
        raw();                // Disable line buffering
        keypad(stdscr, TRUE); // Enable special keys, like arrow keys
        noecho();             // Do not display pressed keys
        timeout(50);          // Set a timeout of 50 milliseconds

        // + disable keyboard delay

        printw("\nPress arrow keys to move joints. Press 'q' to quit.");

        int ch;
        int count_not_pressed = 0;
        while (keep_running_) {
            ch = getch();
            if (ch != ERR) {
                switch (ch) {
                case KEY_LEFT:
                    handleEvent(EVENT::LEFT_ARROW_HOLD);
                    break;
                case KEY_RIGHT:
                    handleEvent(EVENT::RIGHT_ARROW_HOLD);
                    break;
                case 'q':
                    keep_running_ = false;
                    break;
                }
                /// if (!was_not_pressed) printw("Not pressed: %d", count_not_pressed);
                count_not_pressed = 0;
            }
            else {
                // if (was_not_pressed) printw("\n No arrow press");
                if (count_not_pressed > 1)
                    handleEvent(EVENT::NO_ARROW_PRESSED);
                count_not_pressed += 1;
            }

            refresh();
        }

        endwin(); // End the ncurses mode
    }

public:
    explicit InteractiveExample(std::shared_ptr<kord::KordCore> a_kord) : kord{std::move(a_kord)} {}
    InteractiveExample() = default;

    void start()
    {
        keep_running_.store(true);

        kord::ControlInterface ctl_iface(kord);
        kord::ReceiverInterface rcv_iface(kord);
        if (!kord->connect()) {
            KORD_LOG_ERROR("Connecting to KR failed");
            return;
        }

        // Obtain initial q values
        if (!kord->syncRC()) {
            KORD_LOG_ERROR("Sync RC failed.");
            return;
        }
        KORD_LOG_INFO("Sync Captured");
        rcv_iface.fetchData();
        start_q = rcv_iface.getJoint(kord::ReceiverInterface::EJointValue::S_ACTUAL_Q);

        std::string initial_j_conf = "Read initial joint configuration: ";
        for (double angl : start_q)
            initial_j_conf += std::to_string(angl / 3.14 * 180) + " ";
        KORD_LOG_INFO(initial_j_conf);

        // reset of the sent values
        for (int i = 0; i < 7; i++) {
            q[i] = start_q[i];
            qd[i] = 0;
            qdd[i] = 0;
            torque[i] = 0;
        }

        calc_ = std::thread([this]() { runCalcAndEventsCatch(); });
        moving(ctl_iface, rcv_iface, kord);
    }

    void stop()
    {
        keep_running_ = false;
        if (calc_.joinable())
            calc_.join();
    }
};

std::shared_ptr<InteractiveExample> example;

void signal_handler(int a_signum)
{
    g_run = false;
    example->stop();
    psignal(a_signum, "[KORD-API]");
}

int main(int argc, char *argv[])
{
    utils::LaunchParameters lp = utils::LaunchParameters::processLaunchArguments(argc, argv);

    if (lp.help_ || !lp.valid_) {
        return EXIT_SUCCESS;
    }

    signal(SIGINT, signal_handler);

    if (lp.useRealtime()) {
        if (!utils::realtime::init_realtime_params(lp.rt_prio_)) {
            KORD_LOG_ERROR("Failed to start with realtime priority");
            utils::LaunchParameters::printUsage(true);
            return EXIT_FAILURE;
        }
    }

    KORD_LOG_INFO("Connecting to: " << lp.remote_controller_ << ":" << lp.port_);
    KORD_LOG_INFO("Session ID: " << lp.session_id_);

    std::shared_ptr<kord::KordCore> kord(
        new kord::KordCore(lp.remote_controller_, lp.port_, lp.session_id_, kord::UDP_CLIENT));

    example = std::make_shared<InteractiveExample>(kord);

    example->start();
    return 0;
}