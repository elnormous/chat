//
//  Chat client
//

#pragma once

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <cereal/archives/binary.hpp>
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

    void sendMessage(const Message& message)
    {
        std::ostream outputStream(&outputBuffer);
        cereal::BinaryOutputArchive archive(outputStream);
        archive(message);

        uint16_t messageSize = boost::endian::native_to_big(static_cast<uint16_t>(outputBuffer.size()));
        socket.send(boost::asio::buffer(&messageSize, sizeof(messageSize)));

        size_t n = socket.send(outputBuffer.data());
        outputBuffer.consume(n); // remove sent data from the buffer
    }

private:
    void login()
    {
        Message message;
        message.type = Message::Type::LOGIN;
        message.nickname = nickname;

        sendMessage(message);
    }

    void disconnect()
    {

    }

    void handleMessage(Message& message)
    {
        switch (message.type)
        {
            case Message::Type::LOGIN:
                logger->info(message.body);
                break;
            case Message::Type::TEXT:
                std::cout << message.nickname << ": " << message.body << std::endl;
                break;
            case Message::Type::STATUS:
                std::cout << message.body << std::endl;
                break;
            default:
                logger->error("Invalid message received");
                disconnect();
                break;
        }
    }

    void receive()
    {
        boost::asio::streambuf::mutable_buffers_type buffers = inputBuffer.prepare(1024);

        socket.async_receive(buffers,
                             [this](const boost::system::error_code& error, std::size_t bytesTransferred)
        {
            if (error != boost::asio::error::operation_aborted)
            {
                if (error)
                {
                    logger->info("Disconnected");
                    disconnect();
                }
                else
                {
                    logger->info("Received {0} bytes", bytesTransferred);

                    inputBuffer.commit(bytesTransferred);

                    while (inputBuffer.size() >= sizeof(uint16_t))
                    {
                        std::istream inputStream(&inputBuffer);

                        if (!lastMessageSize)
                        {
                            uint16_t messageSize;
                            inputStream.read(reinterpret_cast<char*>(&messageSize), sizeof(messageSize));
                            lastMessageSize = boost::endian::big_to_native(messageSize);
                        }

                        if (inputBuffer.size() >= lastMessageSize)
                        {
                            try
                            {
                                cereal::BinaryInputArchive archive(inputStream);
                                Message message;
                                archive(message);

                                handleMessage(message);

                                lastMessageSize = 0;
                            }
                            catch (std::exception e)
                            {
                                logger->error(e.what());
                                disconnect();
                            }
                        }
                        else
                            break;
                    }

                    receive();
                }
            }
        });
    }

    std::shared_ptr<spdlog::logger> logger;
    boost::asio::io_service& ioService;
    boost::asio::ip::tcp::socket socket;

    uint16_t lastMessageSize = 0;
    boost::asio::streambuf inputBuffer;
    boost::asio::streambuf outputBuffer;

    std::string nickname;
};
