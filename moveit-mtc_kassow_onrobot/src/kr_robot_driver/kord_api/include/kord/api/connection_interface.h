/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2025, Kassow Robots
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Kassow Robots nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#ifndef KR2_KORD_API_CONNECTION_INTERFACE_H
#define KR2_KORD_API_CONNECTION_INTERFACE_H

#include <kord/asio/asio.hpp>
#include <kord/protocol/KORDFrames.h>

#include <memory>
#include <string>

using asio::ip::tcp;
using asio::ip::udp;

namespace kr2::kord {

/**
 * @enum connection
 * @brief Enumeration of connection types.
 */
enum connection {
    TCP_SERVER, /**< TCP Server connection */
    TCP_CLIENT, /**< TCP Client connection */
    UDP_SERVER, /**< UDP Server connection */
    UDP_CLIENT  /**< UDP Client connection */
};

/**
 * @class ConnectionInterface
 * @brief Abstract interface for network connections.
 *
 * Provides a unified interface for different types of network connections,
 * allowing for connection management and data transmission.
 */
class ConnectionInterface {
public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~ConnectionInterface() = default;

    /**
     * @brief Establishes a connection.
     *
     * @param device The device identifier or address to connect to.
     * @return True if the connection was successful, false otherwise.
     */
    virtual bool connect(const char *device) = 0;

    /**
     * @brief Disconnects the current connection.
     *
     * @return True if the disconnection was successful, false otherwise.
     */
    virtual bool disconnect() = 0;

    /**
     * @brief Sends a KORDFrame over the connection.
     *
     * @param a_frame Pointer to the KORDFrame to send.
     * @return Number of bytes sent.
     */
    virtual unsigned int sendFrame(const protocol::KORDFrame *a_frame) = 0;

    /**
     * @brief Sends a KORDFrameTCP over the connection.
     *
     * @param a_frame Pointer to the KORDFrameTCP to send.
     * @return Number of bytes sent.
     */
    virtual unsigned int sendFrame(const protocol::KORDFrameTCP *a_frame) = 0;

    /**
     * @brief Receives a KORDFrame from the connection.
     *
     * @param a_frame Pointer to the KORDFrame to populate with received data.
     * @return Number of bytes received.
     */
    virtual unsigned int recvFrame(protocol::KORDFrame *a_frame) = 0;

    /**
     * @brief Receives a KORDFrameTCP from the connection.
     *
     * @param a_frame Pointer to the KORDFrameTCP to populate with received data.
     * @return Number of bytes received.
     */
    virtual unsigned int recvFrame(protocol::KORDFrameTCP *a_frame) = 0;

    /**
     * @brief Checks if the connection is currently established.
     *
     * @return True if connected, false otherwise.
     */
    virtual bool isConnected() = 0;
};

/**
 * @class TCPClient
 * @brief Implementation of ConnectionInterface for TCP clients.
 *
 * Manages a TCP client connection, including connecting to a server,
 * sending and receiving frames, and disconnecting.
 */
class TCPClient : public ConnectionInterface {
    tcp::socket sock_;            /**< TCP socket for communication */
    tcp::resolver resolver_;      /**< Resolver for resolving server addresses */
    asio::io_service io_service_; /**< IO service for asynchronous operations */
    std::string hostname_;        /**< Hostname of the server */
    unsigned short port_;         /**< Port number of the server */

public:
    /**
     * @brief Constructs a TCPClient.
     *
     * @param io_service Reference to the IO service.
     * @param a_hostname Hostname of the server to connect to.
     * @param a_port Port number of the server.
     */
    TCPClient(asio::io_service &io_service, std::string a_hostname, unsigned short a_port);

    /**
     * @brief Establishes a connection to the TCP server.
     *
     * @param device Device identifier or address (unused for TCPClient).
     * @return True if the connection was successful, false otherwise.
     */
    bool connect(const char *device) override;

    /**
     * @brief Disconnects from the TCP server.
     *
     * @return True if disconnection was successful, false otherwise.
     */
    bool disconnect() override;

    /**
     * @brief Sends a KORDFrame over the TCP connection.
     *
     * @param a_frame Pointer to the KORDFrame to send.
     * @return Number of bytes sent.
     */
    unsigned int sendFrame(const protocol::KORDFrame *a_frame) override;

    /**
     * @brief Sends a KORDFrameTCP over the TCP connection.
     *
     * @param a_frame Pointer to the KORDFrameTCP to send.
     * @return Number of bytes sent.
     */
    unsigned int sendFrame(const protocol::KORDFrameTCP *a_frame) override;

    /**
     * @brief Receives a KORDFrame from the TCP connection.
     *
     * @param a_frame Pointer to the KORDFrame to populate with received data.
     * @return Number of bytes received.
     */
    unsigned int recvFrame(protocol::KORDFrame *a_frame) override;

    /**
     * @brief Receives a KORDFrameTCP from the TCP connection.
     *
     * @param a_frame Pointer to the KORDFrameTCP to populate with received data.
     * @return Number of bytes received.
     */
    unsigned int recvFrame(protocol::KORDFrameTCP *a_frame) override;

    /**
     * @brief Checks if the TCP client is connected.
     *
     * @return True if connected, false otherwise.
     */
    bool isConnected() override;
};

/**
 * @class TCPServer
 * @brief Implementation of ConnectionInterface for TCP servers.
 *
 * Manages a TCP server connection, including accepting connections,
 * sending and receiving frames, and disconnecting.
 */
class TCPServer : public ConnectionInterface {
    tcp::acceptor acceptor_;      /**< TCP acceptor for incoming connections */
    asio::io_service io_service_; /**< IO service for asynchronous operations */
    tcp::socket sock_;            /**< TCP socket for communication */

public:
    /**
     * @brief Constructs a TCPServer.
     *
     * @param io_service Reference to the IO service.
     * @param port Port number to listen on.
     */
    TCPServer(asio::io_service &io_service, unsigned short port);

    /**
     * @brief Accepts a connection from a TCP client.
     *
     * @param device Device identifier or address (unused for TCPServer).
     * @return True if a client was successfully connected, false otherwise.
     */
    bool connect(const char *device) override;

    /**
     * @brief Disconnects the current TCP client.
     *
     * @return True if disconnection was successful, false otherwise.
     */
    bool disconnect() override;

    /**
     * @brief Sends a KORDFrame to the connected TCP client.
     *
     * @param a_frame Pointer to the KORDFrame to send.
     * @return Number of bytes sent.
     */
    unsigned int sendFrame(const protocol::KORDFrame *a_frame) override;

    /**
     * @brief Sends a KORDFrameTCP to the connected TCP client.
     *
     * @param a_frame Pointer to the KORDFrameTCP to send.
     * @return Number of bytes sent.
     */
    unsigned int sendFrame(const protocol::KORDFrameTCP *a_frame) override;

    /**
     * @brief Receives a KORDFrame from the connected TCP client.
     *
     * @param a_frame Pointer to the KORDFrame to populate with received data.
     * @return Number of bytes received.
     */
    unsigned int recvFrame(protocol::KORDFrame *a_frame) override;

    /**
     * @brief Receives a KORDFrameTCP from the connected TCP client.
     *
     * @param a_frame Pointer to the KORDFrameTCP to populate with received data.
     * @return Number of bytes received.
     */
    unsigned int recvFrame(protocol::KORDFrameTCP *a_frame) override;

    /**
     * @brief Checks if the TCP server has an active connection.
     *
     * @return True if a client is connected, false otherwise.
     */
    bool isConnected() override;
};

/**
 * @class UDPClient
 * @brief Implementation of ConnectionInterface for UDP clients.
 *
 * Manages a UDP client connection, including connecting to a server,
 * sending and receiving frames, and disconnecting.
 */
class UDPClient : public ConnectionInterface {
    udp::socket sock_;                                 /**< UDP socket for communication */
    udp::resolver resolver_;                           /**< Resolver for resolving server addresses */
    std::string hostname_;                             /**< Hostname of the server */
    asio::ip::basic_resolver_iterator<udp> endpoints_; /**< Iterator to resolved endpoints */
    udp::endpoint sender_endpoint_;                    /**< Endpoint of the sender */

public:
    /**
     * @brief Constructs a UDPClient.
     *
     * @param io_service Reference to the IO service.
     * @param a_hostname Hostname of the server to connect to.
     * @param a_port Port number of the server.
     */
    UDPClient(asio::io_service &io_service, const std::string &a_hostname, unsigned short a_port);

    /**
     * @brief Establishes a connection to the UDP server.
     *
     * @param device Device identifier or address (unused for UDPClient).
     * @return True if the connection was successful, false otherwise.
     */
    bool connect(const char *device) override;

    /**
     * @brief Disconnects from the UDP server.
     *
     * @return True if disconnection was successful, false otherwise.
     */
    bool disconnect() override;

    /**
     * @brief Sends a KORDFrame over the UDP connection.
     *
     * @param a_frame Pointer to the KORDFrame to send.
     * @return Number of bytes sent.
     */
    unsigned int sendFrame(const protocol::KORDFrame *a_frame) override;

    /**
     * @brief Sends a KORDFrameTCP over the UDP connection.
     *
     * @param a_frame Pointer to the KORDFrameTCP to send.
     * @return Number of bytes sent.
     */
    unsigned int sendFrame(const protocol::KORDFrameTCP *a_frame) override;

    /**
     * @brief Receives a KORDFrame from the UDP connection.
     *
     * @param a_frame Pointer to the KORDFrame to populate with received data.
     * @return Number of bytes received.
     */
    unsigned int recvFrame(protocol::KORDFrame *a_frame) override;

    /**
     * @brief Receives a KORDFrameTCP from the UDP connection.
     *
     * @param a_frame Pointer to the KORDFrameTCP to populate with received data.
     * @return Number of bytes received.
     */
    unsigned int recvFrame(protocol::KORDFrameTCP *a_frame) override;

    /**
     * @brief Checks if the UDP client is connected.
     *
     * @return True if connected, false otherwise.
     */
    bool isConnected() override;
};

/**
 * @class UDPServer
 * @brief Implementation of ConnectionInterface for UDP servers.
 *
 * Manages a UDP server connection, including binding to an address,
 * sending and receiving frames, and disconnecting.
 */
class UDPServer : public ConnectionInterface {
    udp::socket sock_;                                 /**< UDP socket for communication */
    asio::io_service io_service_;                      /**< IO service for asynchronous operations */
    udp::resolver resolver_;                           /**< Resolver for resolving addresses */
    std::string hostname_;                             /**< Hostname associated with the server */
    asio::ip::basic_resolver_iterator<udp> endpoints_; /**< Iterator to resolved endpoints */
    udp::endpoint sender_endpoint_;                    /**< Endpoint of the sender */

public:
    /**
     * @brief Constructs a UDPServer.
     *
     * @param io_service Reference to the IO service.
     * @param a_hostname Hostname to bind the server to.
     * @param a_port Port number to listen on.
     */
    UDPServer(asio::io_service &io_service, std::string a_hostname, unsigned short a_port);

    /**
     * @brief Binds the UDP server to a specific device or address.
     *
     * @param device Device identifier or address to bind to.
     * @return True if binding was successful, false otherwise.
     */
    bool connect(const char *device) override;

    /**
     * @brief Disconnects the UDP server.
     *
     * @return True if disconnection was successful, false otherwise.
     */
    bool disconnect() override;

    /**
     * @brief Sends a KORDFrame to the UDP client.
     *
     * @param a_frame Pointer to the KORDFrame to send.
     * @return Number of bytes sent.
     */
    unsigned int sendFrame(const protocol::KORDFrame *a_frame) override;

    /**
     * @brief Sends a KORDFrameTCP to the UDP client.
     *
     * @param a_frame Pointer to the KORDFrameTCP to send.
     * @return Number of bytes sent.
     */
    unsigned int sendFrame(const protocol::KORDFrameTCP *a_frame) override;

    /**
     * @brief Receives a KORDFrame from the UDP client.
     *
     * @param a_frame Pointer to the KORDFrame to populate with received data.
     * @return Number of bytes received.
     */
    unsigned int recvFrame(protocol::KORDFrame *a_frame) override;

    /**
     * @brief Receives a KORDFrameTCP from the UDP client.
     *
     * @param a_frame Pointer to the KORDFrameTCP to populate with received data.
     * @return Number of bytes received.
     */
    unsigned int recvFrame(protocol::KORDFrameTCP *a_frame) override;

    /**
     * @brief Checks if the UDP server is connected to a client.
     *
     * @return True if connected, false otherwise.
     */
    bool isConnected() override;
};

} // namespace kr2::kord

#endif // KR2_KORD_API_CONNECTION_INTERFACE_H
