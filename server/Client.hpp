//
//  Chat server
//

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include <boost/endian/conversion.hpp>
#include <cereal/archives/binary.hpp>
#include "Server.hpp"
#include "Message.hpp"

class Client final
{
public:
    Client(const std::shared_ptr<spdlog::logger>& l,
           boost::asio::io_service& ioService,
           Server& serv,
           boost::asio::ip::tcp::socket sock):
        logger(l),
        server(serv),
        socket(std::move(sock)),
        inputDeadlineTimer(ioService)
    {
        logger->info("Client connected");

        inputDeadlineTimer.async_wait(boost::bind(&Client::checkDeadline, this));

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

private:
    void login(const std::string& newNickname)
    {
        if (server.isNicknameAvailable(newNickname))
        {
            logger->info("{0} logged in", newNickname);

            loggedIn = true;
            nickname = newNickname;

            Message reply;
            reply.type = Message::Type::LOGIN;
            reply.body = "Logged in with nickname " + newNickname;
            sendMessage(reply);
        }
        else
        {
            logger->error("Nickname {0} unavailable", newNickname);

            Message reply;
            reply.type = Message::Type::LOGIN;
            reply.body = "Nickname " + newNickname + " is unavailable";
            sendMessage(reply);

            disconnect();
        }
    }

    void handleMessage(const Message& message)
    {
        switch (message.type)
        {
            case Message::Type::LOGIN:
                login(message.nickname);
                break;
            case Message::Type::TEXT:
                if (loggedIn)
                {
                    logger->info("{0} sent message: {1}", nickname, message.body);

                    Message textMessage;
                    textMessage.type = Message::Type::TEXT;
                    textMessage.nickname = nickname;
                    textMessage.body = message.body;
                    server.broadcastMessage(textMessage);
                }
                else
                {
                    logger->error("User not logged in");

                    disconnect();
                }
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

        inputDeadlineTimer.expires_from_now(boost::posix_time::seconds(10));

        socket.async_receive(buffers,
                             [this](const boost::system::error_code& error, std::size_t bytesTransferred) {
                                 if (error)
                                 {
                                     logger->info("Disconnected");
                                     server.removeClient(*this);
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
        socket.close();
        inputDeadlineTimer.cancel();
    }

    void checkDeadline()
    {
        if (inputDeadlineTimer.expires_at() <= boost::asio::deadline_timer::traits_type::now())
        {
            logger->info("{0} disconnected due to inactivity", nickname.empty() ? "Client" : nickname);

            Message statusMessage;
            statusMessage.type = Message::Type::STATUS;
            statusMessage.nickname = nickname;
            statusMessage.body = (nickname.empty() ? "Client" : nickname) + " disconnected due to inactivity";
            server.broadcastMessage(statusMessage);

            disconnect();
        }
        else
            inputDeadlineTimer.async_wait(boost::bind(&Client::checkDeadline, this));
    }

    std::shared_ptr<spdlog::logger> logger;
    Server& server;
    boost::asio::ip::tcp::socket socket;

    boost::asio::deadline_timer inputDeadlineTimer;
    uint16_t lastMessageSize = 0;
    boost::asio::streambuf inputBuffer;
    boost::asio::streambuf outputBuffer;

    bool loggedIn = false;
    std::string nickname;
};
