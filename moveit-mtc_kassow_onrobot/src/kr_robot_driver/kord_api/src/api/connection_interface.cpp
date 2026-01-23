#include <iostream>
#include <memory>

#include <kord/api/connection_interface.h>
#include <kord/asio/asio.hpp>
#include <kord/utils/logger.h>

#ifdef __APPLE__
#include <kord/utils/endian.h>
#endif

using namespace kr2::kord;
using namespace kr2::kord::protocol;

/*#############################################################################*/
/*############################ TCP_client #####################################*/
/*#############################################################################*/

TCPClient::TCPClient(asio::io_context &io_context, std::string a_hostname, unsigned short a_port)
    : sock_(io_context), resolver_(io_context), hostname_(std::move(a_hostname)), port_(a_port)
{
}

bool TCPClient::connect(const char *device)
{
    try {
        // Resolve the remote hostname and port
        std::stringstream ss;
        ss << port_;

        // Resolve the query to get a list of endpoints
        tcp::resolver::iterator endpoint_iterator = resolver_.resolve(hostname_, ss.str());

        // If a device is specified, resolve the local address and bind the socket
        if (device != nullptr && std::strlen(device) > 0) {
            asio::ip::address local_address = asio::ip::address::from_string(device);
            asio::ip::tcp::endpoint local_endpoint(local_address, 0); // Port 0 means any available port
            sock_.open(local_endpoint.protocol());
            sock_.bind(local_endpoint);
        }

        // Attempt to connect to one of the remote endpoints
        auto it = asio::connect(sock_, endpoint_iterator);

        if (sock_.is_open()) {
            return true;
        }

        KORD_LOG_ERROR("Connection was not established.");
        return false;
    }
    catch (const asio::system_error &e) {
        KORD_LOG_ERROR("Connect failed: " << e.what());
        return false;
    }
}

bool TCPClient::disconnect()
{
    asio::error_code ec;

    // Shutdown the socket to disable both send and receive operations
    sock_.shutdown(tcp::socket::shutdown_both, ec);
    if (ec) {
        KORD_LOG_ERROR("Shutdown failed: " << ec.message());
        // Proceed to close even if shutdown fails
    }

    // Close the socket
    sock_.close(ec);
    if (ec) {
        KORD_LOG_ERROR("Close failed: " << ec.message());
        return false;
    }

    KORD_LOG_INFO("Disconnected successfully.");
    return true;
}

unsigned int TCPClient::sendFrame(const KORDFrame *a_frame)
{
    try {
        // Ensure that the frame pointer is valid
        if (!a_frame) {
            throw std::invalid_argument("sendFrame received a null pointer.");
        }

        // Send the frame data
        return asio::write(sock_, asio::buffer(a_frame, sizeof(KORDFrame)));
    }
    catch (const asio::system_error &e) {
        KORD_LOG_ERROR("Send failed: " << e.what());
        return 0;
    }
    catch (const std::exception &e) {
        KORD_LOG_ERROR("Send failed: " << e.what());
        return 0;
    }
}

unsigned int TCPClient::sendFrame(const KORDFrameTCP *a_frame)
{
    try {
        // Ensure that the frame pointer is valid
        if (!a_frame) {
            throw std::invalid_argument("sendFrame received a null pointer.");
        }
        // Send the frame data
        return asio::write(sock_, asio::buffer(a_frame, sizeof(KORDFrameTCP)));
    }
    catch (const asio::system_error &e) {
        KORD_LOG_ERROR("Send failed: " << e.what());
        return 0;
    }
    catch (const std::exception &e) {
        KORD_LOG_ERROR("Send failed: " << e.what());
        return 0;
    }
}

unsigned int TCPClient::recvFrame(KORDFrame *a_frame)
{
    try {
        // Ensure that the frame pointer is valid
        if (!a_frame) {
            throw std::invalid_argument("recvFrame received a null pointer.");
        }

        // Wait for the socket to become ready for reading using select()
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock_.native_handle(), &read_fds);

        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = 500;

        int select_result = select(sock_.native_handle() + 1, &read_fds, nullptr, nullptr, &timeout);

        if (select_result > 0 && FD_ISSET(sock_.native_handle(), &read_fds)) {
            // Socket is ready, proceed with reading data
            std::size_t bytes_read = sock_.read_some(asio::buffer(a_frame, sizeof(KORDFrame)));
            a_frame->frame_id_ = ntohs(a_frame->frame_id_);
            a_frame->session_id_ = ntohs(a_frame->session_id_);
            a_frame->tx_timestamp_ = be64toh(a_frame->tx_timestamp_);
            a_frame->kord_version_ = ntohs(a_frame->kord_version_);
            a_frame->payload_length_ = ntohs(a_frame->payload_length_);
            return static_cast<unsigned int>(bytes_read);
        }
        else if (select_result == 0) {
            // Timeout occurred, no data available
            return 0;
        }
        else {
            // Error occurred with select()
            KORD_LOG_ERROR("Select error: " << strerror(errno));
            return -1;
        }
    }
    catch (const std::exception &e) {
        KORD_LOG_ERROR("Receive failed: " << e.what());
        return -1;
    }
}

unsigned int TCPClient::recvFrame(KORDFrameTCP *a_frame)
{
    if (!a_frame) {
        KORD_LOG_ERROR("recvFrame received a null pointer.");
        return 0;
    }

    try {
        if (sock_.available() == 0) {
            return 0;
        }

        std::size_t total_bytes_read = 0;
        std::size_t frame_size = sizeof(KORDFrameTCP);
        char *buffer = reinterpret_cast<char *>(a_frame);

        while (total_bytes_read < frame_size) {
            std::size_t bytes_read =
                sock_.read_some(asio::buffer(buffer + total_bytes_read, frame_size - total_bytes_read));
            if (bytes_read == 0) {
                KORD_LOG_ERROR("Connection closed by peer.");
                return 0; // Connection closed
            }
            total_bytes_read += bytes_read;
        }

        // Convert fields to host byte order
        a_frame->frame_id_ = ntohs(a_frame->frame_id_);
        a_frame->session_id_ = ntohs(a_frame->session_id_);
        a_frame->tx_timestamp_ = be64toh(a_frame->tx_timestamp_);
        a_frame->kord_version_ = ntohs(a_frame->kord_version_);
        a_frame->payload_length_ = ntohs(a_frame->payload_length_);

        return static_cast<unsigned int>(total_bytes_read);
    }
    catch (const std::exception &e) {
        KORD_LOG_ERROR("Receive failed: " << e.what());
        return -1;
    }
}

bool TCPClient::isConnected() { return sock_.is_open(); }

/*#############################################################################*/
/*############################ TCP_server #####################################*/
/*#############################################################################*/

TCPServer::TCPServer(asio::io_service &, unsigned short port)
    : acceptor_(io_service_, tcp::endpoint(tcp::v4(), port)), sock_(io_service_)
{
}

bool TCPServer::connect(const char *)
{

    int native_socket = sock_.native_handle();

    timeval rcv_timeout{0, 10000};
    setsockopt(native_socket, SOL_SOCKET, SO_RCVTIMEO, &rcv_timeout, sizeof(rcv_timeout));

    // TODO: specify endpoints for acceptor
    acceptor_.accept(sock_);
    if (!sock_.is_open()) {
        return false;
    }
    return true;
}

bool TCPServer::disconnect()
{
    asio::error_code ec;
    sock_.close(ec);
    if (ec) {
        // An error occurred.
        return false;
    }
    return true;
}

unsigned int TCPServer::sendFrame(const KORDFrame *a_frame)
{
    return asio::write(sock_, asio::buffer(a_frame, sizeof(KORDFrame)));
}

unsigned int TCPServer::sendFrame(const KORDFrameTCP *)
{
    throw std::runtime_error("TCP server does not support sending TCP frames.");
}

unsigned int TCPServer::recvFrame(KORDFrame *a_frame)
{
    try {
        return asio::read(sock_, asio::buffer(a_frame, sizeof(KORDFrame)));
    }
    catch (asio::system_error &e) {
        return -1;
    }
}

unsigned int TCPServer::recvFrame(protocol::KORDFrameTCP *)
{
    throw std::runtime_error("TCP server does not support receiving TCP frames.");
}

bool TCPServer::isConnected() { return sock_.is_open(); }

/*#############################################################################*/
/*############################ UDP_client #####################################*/
/*#############################################################################*/

UDPClient::UDPClient(asio::io_service &io_service, const std::string &a_hostname, unsigned short a_port)
    : sock_(io_service, udp::endpoint(udp::v4(), a_port)), resolver_(io_service), hostname_(a_hostname)
{

    std::stringstream ss;
    ss << a_port;
    // endpoints_ = resolver_.resolve(udp::v4(), a_hostname, ss.str());
    udp::resolver::query q(udp::v4(), a_hostname, ss.str());
    endpoints_ = resolver_.resolve(q);
}

bool UDPClient::connect(const char *device)
{

    struct hostent *hp;
    asio::error_code ec;
    struct ifreq ifr {};
    memset(&ifr, 0, sizeof(ifr));
    // TODO: refactor the magic constans
    int native_socket = sock_.native_handle();

    timeval rcv_timeout{0, 10};
    setsockopt(native_socket, SOL_SOCKET, SO_RCVTIMEO, &rcv_timeout, sizeof(rcv_timeout));

    if (device) {
        snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", device);
        // boost cannot set rcv_timeout so native_handle will return native socket representation that provides it

#ifdef __APPLE__
        int idx = if_nametoindex(device);
        if (setsockopt(native_socket, IPPROTO_IP, IP_BOUND_IF, &idx, sizeof(idx)) < 0) {
            KORD_LOG_ERROR("Unable to bind " << device << " to the socket.");
            return false;
        }
#else
        if (setsockopt(native_socket, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
            KORD_LOG_ERROR("Unable to bind " << device << " to the socket.");
            return false;
        }
#endif
    }

    hp = gethostbyname(hostname_.c_str());
    if (!hp) {
        KORD_LOG_ERROR("Unknown host");
        sock_.close(ec);
        if (ec) {
            // An error occurred
        }
        return false;
    }

    return true;
}

bool UDPClient::disconnect()
{
    asio::error_code ec;
    sock_.close(ec);
    if (ec) {
        // An error occurred.
        return false;
    }
    return true;
}

unsigned int UDPClient::sendFrame(const KORDFrame *a_frame)
{
    return sock_.send_to(asio::buffer(a_frame, sizeof(KORDFrame)), *endpoints_);
}

unsigned int UDPClient::sendFrame(const KORDFrameTCP *)
{
    throw std::runtime_error("TCP server does not support sending TCP frames.");
}

unsigned int UDPClient::recvFrame(KORDFrame *a_frame)
{
    if (!a_frame) {
        KORD_LOG_ERROR("UDPClient::recvFrame received a null KORDFrame pointer.");
        return 0; 
    }

    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(sock_.native_handle(), &read_set);

    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    int result = select(sock_.native_handle() + 1, &read_set, nullptr, nullptr, &timeout);

    if (result > 0) {
        if (FD_ISSET(sock_.native_handle(), &read_set)) {
            size_t last_packet_size = 0;

            while (true) {
                asio::error_code ec;
                udp::endpoint current_sender;

                size_t current_read = sock_.receive_from(asio::buffer(a_frame, sizeof(KORDFrame)),
                                                         current_sender, 0, ec);

                if (ec) {
                    if (ec == asio::error::timed_out ||
                        ec == asio::error::would_block ||
                        ec == asio::error::try_again) {
                        break;
                    }
                    KORD_LOG_ERROR("UDP recvFrame: Receive error: " << ec.message());
                    if (last_packet_size == 0) {
                        return -1;
                    }
                    break;
                }

                if (current_read == 0 && !ec) {
                    KORD_LOG_DEBUG("UDP recvFrame: Received 0-byte datagram.");
                    break;
                }
                
                last_packet_size = current_read;
                sender_endpoint_ = current_sender;

                asio::error_code available_ec;
                size_t num_bytes_available = sock_.available(available_ec);
                if (available_ec || num_bytes_available == 0) {
                    break;
                }
            }

            if (last_packet_size == 0) {
                return 0;
            }

            a_frame->frame_id_ = ntohs(a_frame->frame_id_);
            a_frame->session_id_ = ntohs(a_frame->session_id_);
            a_frame->tx_timestamp_ = be64toh(a_frame->tx_timestamp_);
            a_frame->kord_version_ = ntohs(a_frame->kord_version_);
            a_frame->payload_length_ = ntohs(a_frame->payload_length_);
            
            return static_cast<unsigned int>(last_packet_size);
        } else {
            KORD_LOG_WARN("UDP recvFrame: select() indicated activity, but not for the expected socket FD's read readiness.");
            return 0;
        }
    }
    else if (result == 0) {
        return 0;
    }

    return -1;
}

unsigned int UDPClient::recvFrame(protocol::KORDFrameTCP *)
{
    throw std::runtime_error("UDP client does not support receiving TCP frames.");
}

bool UDPClient::isConnected() { return sock_.is_open(); }

/*#############################################################################*/
/*############################ UDP_server #####################################*/
/*#############################################################################*/

UDPServer::UDPServer(asio::io_service &, std::string a_hostname, unsigned short a_port)
    : sock_(io_service_, udp::endpoint(udp::v4(), a_port)), resolver_(io_service_), hostname_(a_hostname)
{

    std::stringstream ss;
    ss << a_port;

    udp::resolver::query q(udp::v4(), hostname_, ss.str());
    endpoints_ = resolver_.resolve(q);
}

bool UDPServer::connect(const char *device)
{

    struct hostent *hp;
    asio::error_code ec;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    // TODO: refactor the magic constans
    snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "%s", device);

    // boost cannot set rcv_timeout so native_handle will return native socket representation that provides it

    int native_socket = sock_.native_handle();
    timeval rcv_timeout{0, 10000};
    setsockopt(native_socket, SOL_SOCKET, SO_RCVTIMEO, &rcv_timeout, sizeof(rcv_timeout));
#ifdef __APPLE__
    int idx = if_nametoindex(device);
    if (setsockopt(native_socket, IPPROTO_IP, IP_BOUND_IF, &idx, sizeof(idx)) < 0) {
        KORD_LOG_ERROR("Unable to bind ethnet to the socket.");
        // TODO: pbbly return false
    }
#else
    if (setsockopt(native_socket, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        KORD_LOG_ERROR("Unable to bind ethnet to the socket.");
        // TODO: pbbly return false
    }
#endif

    hp = (struct hostent *)gethostbyname(hostname_.c_str());
    if (!hp) {
        KORD_LOG_ERROR("Unknown host");
        sock_.close(ec);
        if (ec) {
            // An error occured
        }
        return false;
    }

    KORD_LOG_DEBUG("Connection successful.");
    return true;
}

bool UDPServer::disconnect()
{
    asio::error_code ec;
    sock_.close(ec);
    if (ec) {
        // An error occurred.
        return false;
    }
    return true;
}

unsigned int UDPServer::sendFrame(const KORDFrame *a_frame)
{
    return sock_.send_to(asio::buffer(a_frame, sizeof(KORDFrame)), *endpoints_);
}

unsigned int UDPServer::sendFrame(const KORDFrameTCP *)
{
    throw std::runtime_error("UDP server does not support sending TCP frames.");
}

unsigned int UDPServer::recvFrame(KORDFrame *a_frame)
{
    try {
        return sock_.receive_from(asio::buffer(a_frame, sizeof(KORDFrame)), sender_endpoint_);
    }
    catch (asio::system_error &e) {
        return -1;
    }
}

unsigned int UDPServer::recvFrame(protocol::KORDFrameTCP *)
{
    throw std::runtime_error("UDP server does not support receiving TCP frames.");
}

bool UDPServer::isConnected() { return sock_.is_open(); }
