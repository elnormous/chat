//
//  Chat server
//

#pragma once

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <cereal/archives/binary.hpp>
#include "Server.hpp"
#include "Message.hpp"

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

    inline bool isLoggedIn() const
    {
        return loggedIn;
    }

    inline const std::string& getNickname() const
    {
        return nickname;
    }

private:
    void sendMessage(const Message& message)
    {
        std::ostream outputStream(&outputBuffer);
        cereal::BinaryOutputArchive archive(outputStream);
        archive(message);

        uint16_t length = boost::endian::native_to_big(static_cast<uint16_t>(outputBuffer.size()));
        socket.send(boost::asio::buffer(&length, sizeof(length)));

        size_t n = socket.send(outputBuffer.data());
        outputBuffer.consume(n); // remove sent data from the buffer
    }

    void login(const std::string& newNickname)
    {
        loggedIn = true;
        nickname = newNickname;
    }

    void handleMessage(Message& message)
    {
        switch (message.type)
        {
            case Message::Type::LOGIN:
                if (server.isNicknameAvailable(message.nickname))
                {
                    logger->info("{0} logged in", message.nickname);

                    loggedIn = true;

                    Message reply;
                    reply.type = Message::Type::LOGIN;
                    reply.body = "Logged in with nickname " + message.nickname;
                    sendMessage(reply);
                }
                else
                {
                    logger->error("Nickname {0} unavailable", message.nickname);

                    Message reply;
                    reply.type = Message::Type::LOGIN;
                    reply.body = "Nickname " + message.nickname + " is unavailable";
                    sendMessage(reply);

                    disconnect();
                }
                break;
            case Message::Type::CLIENT_TEXT:
                std::cout << "CLIENT_TEXT" << std::endl;
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
                             [this](const boost::system::error_code& error, std::size_t bytesTransferred) {
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
                             });
    }

    void disconnect()
    {
        server.removeClient(*this);
    }

    std::shared_ptr<spdlog::logger> logger;
    Server& server;
    boost::asio::ip::tcp::socket socket;

    uint16_t lastMessageSize = 0;
    boost::asio::streambuf inputBuffer;
    boost::asio::streambuf outputBuffer;

    bool loggedIn = false;
    std::string nickname;
};
