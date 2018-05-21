//
//  Chat client
//

#pragma once

#include <boost/asio.hpp>
#include "cereal/cereal.hpp"
#include "spdlog/spdlog.h"

class Client final
{
public:
    Client(const std::shared_ptr<spdlog::logger>& l,
           boost::asio::io_service& s,
           boost::asio::ip::tcp::resolver::iterator endpoints):
        logger(l),
        ioService(s),
        socket(s)
    {
        boost::system::error_code error;
        boost::asio::connect(socket, endpoints, error);

        if (!error)
            logger->info("Connected");
        else
            throw std::runtime_error("Failed to connect");

        receive();
    }

    void sendMessage(const std::string& message)
    {
        socket.send(boost::asio::buffer(message.data(), message.length()));
    }

private:
    void disconnect()
    {

    }

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

    std::shared_ptr<spdlog::logger> logger;
    boost::asio::io_service& ioService;
    boost::asio::ip::tcp::socket socket;
    std::vector<uint8_t> buffer = std::vector<uint8_t>(1024);
};
