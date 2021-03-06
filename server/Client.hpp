//
//  Chat server
//

#pragma once

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include "cereal/archives/binary.hpp"
#include "Server.hpp"
#include "Message.hpp"

namespace chat
{
    static const size_t BUFFER_SIZE = 1024;
    static const size_t INACTIVITY_TIMEOUT = 10;

    class Client final
    {
    public:
        Client(const std::shared_ptr<spdlog::logger>& l,
               boost::asio::io_service& service,
               Server& serv,
               boost::asio::ip::tcp::socket sock):
            logger(l),
            ioService(service),
            server(serv),
            socket(std::move(sock)),
            inputDeadlineTimer(service)
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

        void sendMessage(const Message& message)
        {
            std::ostream outputStream(&outputBuffer);
            cereal::BinaryOutputArchive archive(outputStream);
            archive(message);

            uint16_t length = ntohs(static_cast<uint16_t>(outputBuffer.size()));
            socket.send(boost::asio::buffer(&length, sizeof(length)));

            size_t n = socket.send(outputBuffer.data());
            outputBuffer.consume(n); // remove sent data from the buffer
        }

    private:
        void disconnect()
        {
            socket.close();
            ioService.post([this]() { server.removeClient(*this); });
        }

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
            boost::asio::streambuf::mutable_buffers_type buffers = inputBuffer.prepare(BUFFER_SIZE);

            inputDeadlineTimer.expires_from_now(boost::posix_time::seconds(INACTIVITY_TIMEOUT));
            checkDeadline();

            socket.async_receive(buffers,
                                 [this](const boost::system::error_code& error, std::size_t bytesTransferred)
            {
                if (error != boost::asio::error::operation_aborted)
                {
                    if (error)
                    {
                        logger->info("Disconnected");
                        disconnect();
                        return;
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
                                lastMessageSize = htons(messageSize);
                            }

                            if (lastMessageSize > BUFFER_SIZE ||
                                inputBuffer.size() > BUFFER_SIZE)
                            {
                                logger->error("Buffer too big");
                                disconnect();
                                return;
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
                                catch (const std::exception& e)
                                {
                                    logger->error(e.what());
                                    disconnect();
                                    return;
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

        void checkDeadline()
        {
            inputDeadlineTimer.async_wait([this](const boost::system::error_code& error)
            {
                if (!error) // not boost::asio::error::operation_aborted
                {
                    logger->info("{0} disconnected due to inactivity", nickname.empty() ? "Client" : nickname);

                    Message statusMessage;
                    statusMessage.type = Message::Type::STATUS;
                    statusMessage.nickname = nickname;
                    statusMessage.body = (nickname.empty() ? "Client" : nickname) + " disconnected due to inactivity";
                    server.broadcastMessage(statusMessage);

                    disconnect();
                }
            });
        }

        std::shared_ptr<spdlog::logger> logger;
        boost::asio::io_service& ioService;
        Server& server;
        boost::asio::ip::tcp::socket socket;

        boost::asio::deadline_timer inputDeadlineTimer;
        uint16_t lastMessageSize = 0;
        boost::asio::streambuf inputBuffer;
        boost::asio::streambuf outputBuffer;

        bool loggedIn = false;
        std::string nickname;
    };
}
