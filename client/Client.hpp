//
//  Chat client
//

#pragma once

#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <boost/asio.hpp>
#include "cereal/cereal.hpp"
#include "spdlog/spdlog.h"
#include "Message.hpp"

class Client final
{
public:
    Client(const std::shared_ptr<spdlog::logger>& l,
           boost::asio::io_service& s,
           boost::asio::ip::tcp::resolver::iterator endpoints,
           const std::string n):
        logger(l),
        ioService(s),
        socket(s),
        nickname(n)
    {
        boost::system::error_code error;
        boost::asio::connect(socket, endpoints, error);

        if (!error)
            logger->info("Connected");
        else
            throw std::runtime_error("Failed to connect");

        login();
        receive();
    }

    void sendMessage(const std::string& str)
    {
        boost::asio::streambuf buffer;
        std::ostream os(&buffer);

        cereal::BinaryOutputArchive archive(os);

        Message message;
        message.type = Message::Type::CLIENT_MESSAGE;
        message.body = str;
        archive(message);

        socket.send(buffer.data());
    }

private:
    void login()
    {
        boost::asio::streambuf buffer;
        std::ostream os(&buffer);

        cereal::BinaryOutputArchive archive(os);

        Message message;
        message.type = Message::Type::LOGIN;
        message.nickname = nickname;
        archive(message);

        socket.send(buffer.data());
    }

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
    std::string nickname;
};
