#include <arpa/inet.h>
#include <chrono>
#include <iostream>

#include <getopt.h>

#include <array>

struct LaunchParameters {

    static LaunchParameters processLaunchArguments(int argc, char **argv)
    {
        LaunchParameters parameters;

        if (argc <= 1) {
            parameters.init_time_ = std::chrono::steady_clock::now();
            parameters.valid_ = false;
            return parameters;
        }

        int opt;
        while ((opt = getopt(argc, argv, "r:p:c:i:")) != -1) {
            switch (opt) {
            case 'h': {
                parameters.help_ = true;
                break;
            }
            case 'r': {
                parameters.rt_prio_ = atoi(optarg);
                break;
            }
            case 'p': {
                parameters.port_ = atoi(optarg);
                if (parameters.port_ > 65535) {
                    parameters.valid_ = false;
                    std::cerr << "ERROR: Invalid port number: " << parameters.port_ << "\n";
                    return parameters;
                }
                break;
            }
            case 'c': {
                unsigned char buf[sizeof(struct in6_addr)];
                parameters.remote_controller_ = optarg;

                if (inet_pton(AF_INET, parameters.remote_controller_.c_str(), buf) <= 0) {
                    parameters.valid_ = false;
                    std::cerr << "ERROR: Malformed IP address: " << parameters.remote_controller_ << "\n";
                    return parameters;
                }
                break;
            }
            case 'i': {
                parameters.session_id_ = atoi(optarg);
                if (parameters.session_id_ > 255) {
                    parameters.valid_ = false;
                    std::cerr << "ERROR: Invalid session ID: " << parameters.session_id_ << "\n";
                    return parameters;
                }
                break;
            }
            }
        }

        parameters.init_time_ = std::chrono::steady_clock::now();
        parameters.valid_ = true;
        return parameters;
    }

    static void printUsage()
    {
        std::cout << "[OPTIONS]\n";
        std::cout << "    -h               show this help\n";
        std::cout << "    -r <prio>        execute as a realtime process with priority set to <prio>\n";
        std::cout << "    -p <port>        port number to connect to\n";
        std::cout << "    -c <IP Address>  remote controller IP address\n";
        std::cout << "    -i <Session ID>  KORD session ID | Default: 1\n";
    }

    bool useRealtime() { return rt_prio_ > 0; }

    bool valid_ = false;
    bool help_ = false;
    int rt_prio_ = -1;
    int port_ = 7582;
    int session_id_ = 1;
    std::string remote_controller_ = "192.168.38.1";

    std::chrono::time_point<std::chrono::steady_clock> init_time_;
};