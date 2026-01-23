#include <csignal>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <pthread.h>
#include <sstream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <kord/utils/logger.h>
#include <kord/utils/utils.h>

#define DEFAULT_PRIORITY 20
#define DEFAULT_PRIORITY_MAX 50

using namespace kr2::utils;

StampsLog::StampsLog() { reset(); }

void StampsLog::reset()
{
    t0_ = INT64_MAX;
    t1_ = INT64_MAX;
    t2_ = INT64_MAX;
}

std::string StampsLog::header()
{
    std::stringstream ss;
    ss << "t0, t1, t2";
    return ss.str();
}

std::string StampsLog::asString()
{
    std::stringstream ss;
    ss << t0_ << ", " << t1_ << ", " << t2_;
    return ss.str();
}

StatGeneral::StatGeneral() { reset(); }

void StatGeneral::reset()
{
    avg_ = 0;
    max_ = INT64_MIN;
    min_ = INT64_MAX;
    counter_ = 0;
}

void StatGeneral::update(long a_uval)
{
    if (a_uval < min_)
        min_ = a_uval;
    if (a_uval > max_)
        max_ = a_uval;

    if (counter_ == 0) {
        avg_ = a_uval;
        counter_++;
        return;
    }

    long avg_t = (counter_ * avg_) + a_uval;
    avg_ = avg_t / (counter_ + 1);
    counter_++;
}

void StatGeneral::printStat(std::ostream &a_stream) const
{
    a_stream << "min, avg, max\n";
    a_stream << min_ << ", " << avg_ << ", " << max_ << " \n";
}

StatsTimepointDifference::StatsTimepointDifference() { this->reset(); }

void StatsTimepointDifference::reset()
{
    using namespace std::chrono_literals;
    min_ = 1000s;
    max_ = 0us;
}

void StatsTimepointDifference::updateFailure(const std::chrono::time_point<std::chrono::steady_clock> &)
{
    failed_capture_cnt_++;
}

void StatsTimepointDifference::updateCapture(const std::chrono::time_point<std::chrono::steady_clock> &a_capture_ts)
{
    if (captured_message_cnt_ == 0) {
        cap_first_ = a_capture_ts;
        cap_latest_ = a_capture_ts;
        captured_message_cnt_++;
        return;
    }

    timespan elapsed = a_capture_ts - cap_latest_;
    if (elapsed < min_) {
        min_ = elapsed;
    }

    if (elapsed > max_) {
        max_ = elapsed;
    }

    cap_latest_ = a_capture_ts;
    captured_message_cnt_++;
}

void StatsTimepointDifference::printStats(std::ostream &a_stream) const
{
    int64_t avg_cap = std::chrono::duration_cast<timespan>(cap_latest_ - cap_first_).count();
    avg_cap /= static_cast<int64_t>(captured_message_cnt_);
    a_stream << "Captured msgs; Failed msgs; min[ns]; avg capture[ns]; max[ns]\n";
    a_stream << captured_message_cnt_ << ";  " << failed_capture_cnt_ << "; " << min_.count() << "; " << avg_cap << "; "
             << max_.count() << "\n";
}

bool realtime::set_latency_target()
{
    struct stat s {};
    int latency_target_fd = -1;
    int err;

    err = stat("/dev/cpu_dma_latency", &s);
    if (err == -1) {
        KORD_LOG_ERROR("[RealTime] getting file '/dev/cpu_dma_latency' status failed");
        return false;
    }

    latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
    if (latency_target_fd == -1) {
        KORD_LOG_ERROR("[RealTime] opening '/dev/cpu_dma_latency' failed");
        return false;
    }

    int32_t latency_target_value = 0;

    ssize_t written = write(latency_target_fd, &latency_target_value, 4);
    if (written < 1) {
        close(latency_target_fd);
        KORD_LOG_ERROR("[RealTime] setting the cpu_dma_latency to " + std::to_string(latency_target_value) +
                       "us failed");
        return false;
    }

    close(latency_target_fd);

    return true;
}

bool realtime::init_realtime_params(int a_prio)
{
    if (!set_latency_target()) {
        KORD_LOG_ERROR("[RealTime] setting latency target failed");
        return false;
    }

    // We'll operate on the currently running thread.
    pthread_t this_thread = pthread_self();

    // sched_param is used to store the scheduling priority
    sched_param params{};
    // We'll set the priority to the maximum.
    params.sched_priority = DEFAULT_PRIORITY;

    if ((a_prio > 0) && (a_prio <= DEFAULT_PRIORITY_MAX)) {
        params.sched_priority = a_prio;
    }

    // Attempt to set thread real-time priority to the SCHED_FIFO policy
    KORD_LOG_INFO("[RealTime] setting thread realtime priority = " << params.sched_priority);
    int ret_value = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
    if (ret_value != 0) {
        // Print the error
        KORD_LOG_ERROR("[RealTime] setting thread real-time priority failed");
        return false;
    }
    // Now verify the change in thread priority
    int policy = 0;
    ret_value = pthread_getschedparam(this_thread, &policy, &params);
    if (ret_value != 0) {
        KORD_LOG_ERROR("[RealTime] retrieving real-time scheduling parameters failed");
        return false;
    }

    // Check the correct policy was applied
    if (policy != SCHED_FIFO) {
        KORD_LOG_ERROR("[RealTime] Scheduling is NOT SCHED_FIFO! Setting scheduler failed");
        return false;
    }

    KORD_LOG_INFO("[RealTime] 'SCHED_FIFO' ok");
    KORD_LOG_INFO("[RealTime] thread priority is: " << params.sched_priority);

    // block SIGALRM
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigset, nullptr);

    // Lock process pages in RAM from swapping
    KORD_LOG_INFO("[RealTime] memory lock all");
    if (mlockall(MCL_FUTURE)) {
        KORD_LOG_ERROR("[RealTime] mlockall failed");
        return false;
    }

    KORD_LOG_INFO("[RealTime] mlockall successful");

    // Try detaching this thread
    pthread_detach(this_thread);

    return true;
}

std::array<double, 7> kr2::utils::convertEulerToQuaternion(
    double x, double y, double z, double roll, double pitch, double yaw)
{
    // Compute half angles
    double half_roll = roll * 0.5;
    double half_pitch = pitch * 0.5;
    double half_yaw = yaw * 0.5;

    // Compute sine and cosine for each half angle
    double cr = cos(half_roll);
    double sr = sin(half_roll);
    double cp = cos(half_pitch);
    double sp = sin(half_pitch);
    double cy = cos(half_yaw);
    double sy = sin(half_yaw);

    // Compute quaternion components
    double qw = cr * cp * cy + sr * sp * sy;
    double qx = sr * cp * cy - cr * sp * sy;
    double qy = cr * sp * cy + sr * cp * sy;
    double qz = cr * cp * sy - sr * sp * cy;

    // Normalize the quaternion
    double norm = sqrt(qw * qw + qx * qx + qy * qy + qz * qz);
    qw /= norm;
    qx /= norm;
    qy /= norm;
    qz /= norm;

    // Assemble the 7-element vector
    std::array pose_quat = {x, y, z, qx, qy, qz, qw};
    return pose_quat;
}
