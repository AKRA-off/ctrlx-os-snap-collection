#include <array>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <future>
#include <gtest/gtest.h>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

enum class ExampleCommand {
    BrakeControl,            // kord_brake_control
    CleanAlarm,              // kord_clean_alarm
    FetchData,               // kord_fetch_data
    GetTcpAndCog,            // kord_get_tcp_and_cog
    Init,                    // kord_init
    InitAndFetch,            // kord_init_and_fetch
    MoveDirect,              // kord_move_direct
    MoveDirectInteractive,   // kord_move_direct_interactive
    MoveJoints,              // kord_move_joints
    MoveJointsDiscrete,      // kord_move_joints_discrete
    MoveLinear,              // kord_move_linear
    MoveLinearDiscrete,      // kord_move_linear_discrete
    MoveVelocity,            // kord_move_velocity
    ReadState,               // kord_read_state
    ReadTemperature,         // kord_read_temperature
    RetrieveErrors,          // kord_retrieve_errors
    SelfMotion,              // kord_self_motion
    SetFrame,                // kord_set_frame
    SetLoad,                 // kord_set_load
    SetOport,                // kord_set_oport
    SetSafeOutput,           // kord_set_safe_output
    SkipFetchData,           // kord_skip_fetch_data
    TransferCalibrationData, // kord_transfer_calibration_data
    TransferJson,            // kord_transfer_json
    TransferLogs,            // kord_transfer_logs
    TransferMoreFiles        // kord_transfer_more_files
};

std::unordered_map<ExampleCommand, std::string> commandToPath =
    {{ExampleCommand::BrakeControl, "../kord-brake-control"},
     {ExampleCommand::CleanAlarm, "../kord-clean-alarm"},
     {ExampleCommand::FetchData, "../kord-fetch-data"},
     {ExampleCommand::GetTcpAndCog, "../kord-get-tcp-and-cog"},
     {ExampleCommand::Init, "../kord-init"},
     {ExampleCommand::InitAndFetch, "../kord-init-and-fetch"},
     {ExampleCommand::MoveDirect, "../kord-move-direct"},
     {ExampleCommand::MoveDirectInteractive, "../kord-move-direct-interactive"},
     {ExampleCommand::MoveJoints, "../kord-move-joints"},
     {ExampleCommand::MoveJointsDiscrete, "../kord-move-joints-discrete"},
     {ExampleCommand::MoveLinear, "../kord-move-linear"},
     {ExampleCommand::MoveLinearDiscrete, "../kord-move-linear-discrete"},
     {ExampleCommand::MoveVelocity, "../kord-move-velocity"},
     {ExampleCommand::ReadState, "../kord-read-state"},
     {ExampleCommand::ReadTemperature, "../kord-read-temperature"},
     {ExampleCommand::RetrieveErrors, "../kord-retrieve-errors"},
     {ExampleCommand::SelfMotion, "../kord-self-motion"},
     {ExampleCommand::SetFrame, "../kord-set-frame"},
     {ExampleCommand::SetLoad, "../kord-set-load"},
     {ExampleCommand::SetOport, "../kord-set-oport"},
     {ExampleCommand::SetSafeOutput, "../kord-set-safe-output"},
     {ExampleCommand::SkipFetchData, "../kord-skip-fetch-data"},
     {ExampleCommand::TransferCalibrationData, "../kord-transfer-calibration-data"},
     {ExampleCommand::TransferJson, "../kord-transfer-json"},
     {ExampleCommand::TransferLogs, "../kord-transfer-logs"},
     {ExampleCommand::TransferMoreFiles, "../kord-transfer-more-files"}};

std::string runCommand(const std::string &command, int &exitCode, unsigned int timeoutMs = 0);
std::vector<std::string> readFromState(const std::regex &pattern);

class Environment : public ::testing::Environment {
public:
    const std::filesystem::path home_path;
    const std::filesystem::path files_folder;

    Environment()
        : home_path(getenv("HOME") ? std::filesystem::path(getenv("HOME")) : std::filesystem::path()),
          files_folder(home_path / "Workspace" / "Files")
    {
    }
    ~Environment() override = default;

    void SetUp() override
    {
        // TODO backup config, upload new one for testing, now relies on the user (should be uploaded manually)
        // TODO in Cbun - reloading config
    }

    void TearDown() override {}
};

class MoveFixture : public ::testing::Test {
protected:
    MoveFixture() = default;

    void SetUp() override
    {
        int exit_code;
        runCommand(commandToPath[ExampleCommand::CleanAlarm] + " --all", exit_code);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    void TearDown() override { std::this_thread::sleep_for(std::chrono::seconds(3)); }
};

std::string runCommand(const std::string &command, int &exitCode, unsigned int timeoutMs)
{
    std::array<char, 128> buffer{};
    std::string result;
    int pipefd[2];

    // Create a pipe for IPC
    if (pipe(pipefd) == -1) {
        throw std::runtime_error("pipe() failed!");
    }

    // Set close-on-exec flag on the read and write ends of the pipe
    // to ensure they are closed in the child after exec
    fcntl(pipefd[0], F_SETFD, FD_CLOEXEC);
    fcntl(pipefd[1], F_SETFD, FD_CLOEXEC);

    pid_t pid = fork();
    if (pid < 0) {
        // Fork failed
        throw std::runtime_error("fork() failed!");
    }

    if (pid == 0) {
        // Child process

        // Create a new process group for the child
        if (setpgid(0, 0) == -1) {
            perror("setpgid failed");
            _exit(1);
        }

        // Close the read end of the pipe in the child
        close(pipefd[0]);

        // Redirect stdout to the write end of the pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2 stdout failed");
            _exit(1); // Exit if dup2 fails
        }

        // Redirect stderr to the write end of the pipe
        if (dup2(pipefd[1], STDERR_FILENO) == -1) {
            perror("dup2 stderr failed");
            _exit(1); // Exit if dup2 fails
        }

        // Close the duplicated write end
        close(pipefd[1]);

        // Execute the command using /bin/sh -c
        execl("/bin/sh", "sh", "-c", command.c_str(), (char *)nullptr);

        // If execl fails
        perror("execl failed");
        _exit(1);
    }

    // Parent process

    // Close the write end of the pipe in the parent
    close(pipefd[1]);

    // Asynchronously read from the pipe
    std::future<std::string> futureOutput = std::async(std::launch::async, [&]() -> std::string {
        std::string output;
        ssize_t count;
        while ((count = read(pipefd[0], buffer.data(), buffer.size())) > 0) {
            output.append(buffer.data(), count);
        }
        if (count == -1) {
            perror("read failed");
        }
        close(pipefd[0]); // Close the read end after reading
        return output;
    });

    // Function to wait for the child process
    auto waitForProcess = [&](pid_t pid) -> int {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid failed");
            return -1;
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            return -1; // Child was killed by signal
        }
        return -1; // Unexpected status
    };

    // Asynchronously wait for the child process to finish
    std::future<int> futureExitCode = std::async(std::launch::async, waitForProcess, pid);

    if (timeoutMs == 0) {
        // No timeout, wait indefinitely
        exitCode = futureExitCode.get();
    }
    else {
        // Wait for the process to finish within the timeout
        if (futureExitCode.wait_for(std::chrono::milliseconds(timeoutMs)) == std::future_status::timeout) {
            // Send SIGINT to the entire process group
            if (kill(-pid, SIGINT) == -1) {
                perror("Failed to send SIGINT");
            }

            // Optional: Wait for a grace period after SIGINT
            unsigned int gracePeriodMs = 1000; // 1 second
            if (futureExitCode.wait_for(std::chrono::milliseconds(gracePeriodMs)) == std::future_status::timeout) {
                std::cout << "Process did not terminate after SIGINT. Sending SIGKILL." << std::endl;
                if (kill(-pid, SIGKILL) == -1) {
                    perror("Failed to send SIGKILL");
                }
            }

            // Now, wait for the process to terminate
            exitCode = futureExitCode.get();
        }
        else {
            // Command finished within the timeout
            exitCode = futureExitCode.get();
        }
    }

    // Retrieve the command's output
    result = futureOutput.get();

    return result;
}

std::vector<std::string> readFromState(const std::regex &pattern)
{
    int exit_code;
    std::string output = runCommand(commandToPath[ExampleCommand::ReadState], exit_code);
    std::smatch matches;
    std::vector<std::string> results;
    while (std::regex_search(output, matches, pattern)) {
        results.emplace_back(matches.suffix());
        output = matches.suffix().str();
    }
    return results;
}

std::vector<double> getTCP()
{
    int exit_code;
    std::string output = runCommand(commandToPath[ExampleCommand::ReadState], exit_code);

    std::smatch matches;
    std::vector<double> tcpValues;

    std::regex pattern(R"(TCP:\s*\[([-+eE0-9.\s]+)\])");
    std::string::const_iterator searchStart(output.cbegin());
    while (std::regex_search(searchStart, output.cend(), matches, pattern)) {
        std::string valuesString = matches[1].str();
        std::istringstream iss(valuesString);
        std::string value;

        // Split the string by spaces and convert to double
        while (std::getline(iss, value, ' ')) {
            if (!value.empty()) {
                tcpValues.emplace_back(std::stod(value));
            }
        }

        searchStart = matches.suffix().first; // Move the search start forward
    }

    return tcpValues;
}

std::vector<double> getJointDegrees()
{
    int exit_code;
    std::string output = runCommand(commandToPath[ExampleCommand::ReadState], exit_code);
    if (exit_code > 0) {
        std::cerr << "Failed to retrieve joint degrees" << std::endl;
    }
    std::smatch matches;
    std::vector<double> degrees;

    std::regex pattern(R"(Degrees:\s*\[([-+eE0-9.\s]+)\])");
    std::string::const_iterator searchStart(output.cbegin());
    while (std::regex_search(searchStart, output.cend(), matches, pattern)) {
        std::string valuesString = matches[1].str();
        std::istringstream iss(valuesString);
        std::string value;

        // Split the string by spaces and convert to double
        while (std::getline(iss, value, ' ')) {
            if (!value.empty()) {
                degrees.emplace_back(std::stod(value));
            }
        }

        searchStart = matches.suffix().first; // Move the search start forward
    }

    return degrees;
}

bool areVectorsEqual(const std::vector<double> &vec1, const std::vector<double> &vec2, double tolerance = 1e-9)
{
    // Check if sizes are different
    if (vec1.size() != vec2.size()) {
        return false;
    }

    // Compare elements with tolerance
    for (size_t i = 0; i < vec1.size(); ++i) {
        if (std::fabs(vec1[i] - vec2[i]) > tolerance) {
            return false;
        }
    }

    return true;
}

bool isSubstring(const std::string &str1, const std::string &str2) { return str1.find(str2) != std::string::npos; }

bool forceHalt()
{
    int exit_code;
    std::string output =
        runCommand(commandToPath[ExampleCommand::MoveLinearDiscrete] + " --offset=0.5,0.5,0.5,0,0,0", exit_code);
    sleep(2);
    return exit_code == 0;
}

TEST_F(MoveFixture, MOVE_JOINTS_DISCRETE)
{
    int exit_code;

    auto initial_tcp = getTCP();
    auto zero_output =
        runCommand(commandToPath[ExampleCommand::MoveJointsDiscrete] + " --target=0,0,0,0,0,0,0", exit_code);
    ASSERT_EQ(exit_code, 0) << "MOVE_JOINTS failed with error code " << exit_code << " and output:\n" << zero_output;

    std::this_thread::sleep_for(std::chrono::seconds(5));

    auto current_joint_configuration = getJointDegrees();
    ASSERT_EQ(current_joint_configuration.size(), 7) << "Failed to get joint configuration";
    for (int i = 0; i < 7; i++) {
        ASSERT_NEAR(current_joint_configuration[i], 0.0, 1e-3);
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));

    auto move_output =
        runCommand(commandToPath[ExampleCommand::MoveJointsDiscrete] + " --target=1,2,3,4,5,6,7", exit_code);
    ASSERT_EQ(exit_code, 0) << "MOVE_JOINTS_DISCRETE failed with error code " << exit_code << " and output:\n"
                            << move_output;

    std::this_thread::sleep_for(std::chrono::seconds(10));
    current_joint_configuration = getJointDegrees();
    ASSERT_EQ(current_joint_configuration.size(), 7);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::vector<double> final_conf = {1, 2, 3, 4, 5, 6, 7};
    auto n = current_joint_configuration.size();
    for (int i = 0; i < n; i++) {
        ASSERT_NEAR(current_joint_configuration[i], final_conf[i], 1e-3);
    }
}

TEST_F(MoveFixture, MOVE_LINEAR_DISCRETE)
{
    int exit_code;
    runCommand(commandToPath[ExampleCommand::MoveJointsDiscrete] + " --target=0,0,0,0,0,0,0", exit_code);
    ASSERT_EQ(exit_code, 0) << "Moving to zero position failed";

    std::this_thread::sleep_for(std::chrono::seconds(10));

    auto zero_tcp = getTCP();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    auto output =
        runCommand(commandToPath[ExampleCommand::MoveLinearDiscrete] + " --offset=-0.005,0.002,-0.001,0,0,0", exit_code);
    ASSERT_EQ(exit_code, 0) << "MOVE_LINEAR_DISCRETE failed with error code " << exit_code << " and output:\n" << output;

    std::this_thread::sleep_for(std::chrono::seconds(10));
    auto target_tcp = std::vector<double>(zero_tcp);
    target_tcp[0] -= 0.005;
    target_tcp[1] += 0.002;
    target_tcp[2] -= 0.001;
    auto n = target_tcp.size();
    for (int i = 0; i < n; i++) {
        ASSERT_NEAR(target_tcp[i], zero_tcp[i], 1e-2);
    }
}

TEST_F(MoveFixture, DISABLED_MOVE_DIRECT)
{
    int exit_code;
    ASSERT_EQ(0, 1);
}

TEST_F(MoveFixture, DISABLED_MOVE_LINEAR)
{
    int exit_code;
    ASSERT_EQ(0, 1);
}

TEST_F(MoveFixture, DISABLED_MOVE_JOINTS)
{
    int exit_code;
    ASSERT_EQ(0, 1);
}

TEST_F(MoveFixture, MOVE_SELF_MOTION)
{
    int exit_code;

    runCommand(commandToPath[ExampleCommand::MoveJointsDiscrete] + " --target=0,0,0,0,0,0,0", exit_code);
    ASSERT_EQ(exit_code, 0) << "Moving to zero position failed";
    std::this_thread::sleep_for(std::chrono::seconds(5));

    auto init_tcp = getTCP();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto current_joint_configuration = getJointDegrees();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::string command = commandToPath[ExampleCommand::SelfMotion];
    runCommand(command, exit_code, 5000);

    auto final_joint_configuration = getJointDegrees();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    auto current_tcp = getTCP();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto n = current_tcp.size();
    for (int i = 0; i < n; i++)
        ASSERT_NEAR(current_tcp[i], init_tcp[i], 1e-2);

    if (areVectorsEqual(current_joint_configuration, final_joint_configuration, 1e-2)) {
        FAIL() << "Joint configuration should have changed";
    }
}

TEST(GENERAL, CLEAR_ALARM)
{
    int exit_code;
    auto output = runCommand(commandToPath[ExampleCommand::CleanAlarm] + " --all", exit_code);

    ASSERT_TRUE(isSubstring(output, "CLEAR_HALT command status: 0")) << "Command status not found";
    ASSERT_TRUE(isSubstring(output, "CBUN_EVENT command status: 0")) << "Command status not found";
    // ASSERT_TRUE(isSubstring(output, "CONTINUE_INIT command status: 0")) << "Command status not found";
    ASSERT_TRUE(isSubstring(output, "UNSUSPEND command status: 0")) << "Command status not found";
}

TEST(GENERAL, BREAKS)
{
    int exit_code;

    // Lock each joint sequentially
    for (int i = 7; i >= 1; --i) {
        runCommand(commandToPath[ExampleCommand::BrakeControl] + " --engage=" + std::to_string(i), exit_code);
        ASSERT_EQ(exit_code, 0) << "Failed to engage break " << i;
    }

    // Unlock each joint in reverse order
    for (int i = 1; i <= 7; ++i) {
        runCommand(commandToPath[ExampleCommand::BrakeControl] + " --disengage=" + std::to_string(i), exit_code);
        ASSERT_EQ(exit_code, 0) << "Failed to engage break " << i;
    }
}

TEST(GENERAL, DISABLED_INIT) { ASSERT_EQ(true, false) << "Init test is not implemented"; }

TEST(GENERAL, SET_LOAD)
{
    int exit_code;
    auto output =
        runCommand(commandToPath[ExampleCommand::SetLoad] + " --load=0 --cog=0,0,0 --inertia=0,0,0,0,0,0 --mass=0",
                   exit_code);
    ASSERT_EQ(exit_code, 0) << "Failed to set load";
    ASSERT_TRUE(isSubstring(output, "Command status: 0"));
}

TEST(GENERAL, SET_FRAME)
{
    int exit_code;
    auto output = runCommand(commandToPath[ExampleCommand::SetFrame] + " --tcp=0,0,0,0,0,0", exit_code);
    ASSERT_EQ(exit_code, 0) << "Failed to set frame";
    ASSERT_TRUE(isSubstring(output, "Command status: 0"));
}

TEST(GENERAL, SET_OPORT)
{
    int exit_code;
    auto output = runCommand(commandToPath[ExampleCommand::SetOport], exit_code);
    ASSERT_EQ(exit_code, 0) << "Failed to set output ports";
    ASSERT_TRUE(isSubstring(output, "SUCCESS")) << "Output does not contain SUCCESS";
}

TEST(GENERAL, SET_SAFE_OUTPUT)
{
    int exit_code;
    auto output = runCommand(commandToPath[ExampleCommand::SetSafeOutput], exit_code);
    ASSERT_EQ(exit_code, 0) << "Failed to set safety output ports";
}

TEST(GENERAL, GET_TCP_AND_COG)
{
    int exit_code;
    auto output = runCommand(commandToPath[ExampleCommand::GetTcpAndCog], exit_code, 5000);
    // We don't check exit code since we interrupt the process
    ASSERT_TRUE(isSubstring(output, "Load1.cog")) << "Output does not contain Load1.cog";
    ASSERT_TRUE(isSubstring(output, "Load1.mass")) << "Output does not contain Load1.mass";
    ASSERT_TRUE(isSubstring(output, "Load1.inertia")) << "Output does not contain Load1.inertia";
    ASSERT_TRUE(isSubstring(output, "Load2.cog")) << "Output does not contain Load2.cog";
    ASSERT_TRUE(isSubstring(output, "Load2.mass")) << "Output does not contain Load2.mass";
    ASSERT_TRUE(isSubstring(output, "Load2.inertia")) << "Output does not contain Load2.inertia";
}

TEST(GENERAL, RETRIEVE_ERRORS)
{
    int exit_code;
    auto output = runCommand(commandToPath[ExampleCommand::RetrieveErrors], exit_code, 5000);
    // We don't check exit code since we interrupt the process
}

class FailureTransfer : public ::testing::Test {
    void SetUp() override {}

    void TearDown() override {}
};

TEST(TRANSFER, TRANSFER_CALIBRATION_DATA)
{
    int exit_code;
    auto output = runCommand(commandToPath[ExampleCommand::TransferCalibrationData], exit_code);
    ASSERT_EQ(exit_code, 0) << "Transfer failed" << std::endl;
    ASSERT_TRUE(isSubstring(output, "SUCCESS")) << "SUCCESS not found in the output";
}

TEST(TRANSFER, TRANSFER_JSON)
{
    int exit_code;
    auto output = runCommand(commandToPath[ExampleCommand::TransferJson], exit_code);
    ASSERT_EQ(exit_code, 0) << "Transfer failed" << std::endl;
    ASSERT_TRUE(isSubstring(output, "SUCCESS")) << "SUCCESS not found in the output";
}

TEST(TRANSFER, TRANSFER_LOGS)
{
    int exit_code;
    auto output = runCommand(commandToPath[ExampleCommand::TransferLogs], exit_code);
    ASSERT_EQ(exit_code, 0) << "Transfer failed" << std::endl;
    ASSERT_TRUE(isSubstring(output, "SUCCESS")) << "SUCCESS not found in the output";
}

TEST(TRANSFER, TRANSFER_MORE_FILES)
{
    int exit_code;
    auto output = runCommand(commandToPath[ExampleCommand::TransferMoreFiles], exit_code);
    ASSERT_EQ(exit_code, 0) << "Transfer failed" << std::endl;
    ASSERT_TRUE(isSubstring(output, "SUCCESS")) << "SUCCESS not found in the output";
}

void signal_handler(sig_atomic_t a_signum)
{
    psignal(a_signum, "[KORD-API]");
    exit(a_signum);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, signal_handler);
    // auto const test_env = testing::AddGlobalTestEnvironment(new Environment);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
