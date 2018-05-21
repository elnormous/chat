//
//  Chat server
//

#pragma once

#include "Server.hpp"

class Client final
{
public:
    Client(const std::shared_ptr<spdlog::logger>& l,
           Server& s,
           boost::asio::ip::tcp::socket sock):
        logger(l),
        server(s),
        socket(std::move(sock))
    {
        logger->info("Client connected");

        receive();
    }

    void login(const std::string& newNickname)
    {
        loggedIn = true;
        nickname = newNickname;
    }

    inline bool isLoggedIn() const
    {
        return loggedIn;
    }

    inline const std::string& getNickname() const
    {
        return nickname;
    }

private:
    void receive()
    {
        socket.async_receive(boost::asio::buffer(buffer.data(), buffer.size()),
                             [this](const boost::system::error_code& error, std::size_t bytesTransferred) {
                                 if (error)
                                 {
                                     logger->info("Disconnected");
                                     disconnect();
                                 }
                                 else
                                 {
                                     logger->info("Received {0} bytes", bytesTransferred);
                                     receive();
                                 }
                             });
    }

    void disconnect()
    {
        server.removeClient(*this);
    }

    std::shared_ptr<spdlog::logger> logger;
    Server& server;
    boost::asio::ip::tcp::socket socket;
    std::vector<uint8_t> buffer = std::vector<uint8_t>(1024);
    bool loggedIn = false;
    std::string nickname;
};
