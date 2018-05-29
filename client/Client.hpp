//
//  Chat client
//

#pragma once

#include <cstdlib>
#include <unistd.h>
#include <boost/asio.hpp>
#include "cereal/archives/binary.hpp"
#include "cereal/cereal.hpp"
#include "spdlog/spdlog.h"
#include "Message.hpp"

namespace chat
{
    static const size_t BUFFER_SIZE = 1024;
    static const size_t CONNECTION_TIMEOUT = 3;
    static const size_t RECONNECT_INTERVAL = 5;

    class Client final
    {
    public:
        Client(const std::shared_ptr<spdlog::logger>& l,
            boost::asio::io_service& s,
            boost::asio::ip::tcp::endpoint endpoint,
            const std::string n):
            logger(l),
            ioService(s),
            socket(s),
            nickname(n),
            commandLine(ioService, ::dup(STDIN_FILENO)),
            connectDeadlineTimer(s),
            reconnectDeadlineTimer(s),
            signals(s, SIGINT, SIGTERM)
        {
            connect(endpoint);

            signals.async_wait([this](const boost::system::error_code& error,
                                    int signalNumber)
            {
                if (!error)
                {
                    logger->info("Received signal {0}", signalNumber);
                    disconnect();
                }
            });
        }

        void sendMessage(const Message& message)
        {
            std::ostream outputStream(&outputBuffer);
            cereal::BinaryOutputArchive archive(outputStream);
            archive(message);

            uint16_t messageSize = ntohs(static_cast<uint16_t>(outputBuffer.size()));
            socket.send(boost::asio::buffer(&messageSize, sizeof(messageSize)));

            size_t n = socket.send(outputBuffer.data());
            outputBuffer.consume(n); // remove sent data from the buffer
        }

    private:
        void connect(boost::asio::ip::tcp::endpoint endpoint)
        {
            logger->info("Connecting");

            // wait 3 seconds for connection to finish
            connectDeadlineTimer.expires_from_now(boost::posix_time::seconds(CONNECTION_TIMEOUT));
            connectDeadlineTimer.async_wait([this, endpoint](const boost::system::error_code& error)
            {
                if (!error) // not boost::asio::error::operation_aborted
                    socket.close();
            });

            socket.async_connect(endpoint, [this, endpoint](const boost::system::error_code& error) {
                if (!error)
                {
                    logger->info("Connected");

                    connectDeadlineTimer.cancel();
                    reconnectDeadlineTimer.cancel();

                    login();
                    receive();
                    readCommandLine();
                }
                else
                {
                    connectDeadlineTimer.cancel();

                    // wait 5 seconds for reconnect
                    reconnectDeadlineTimer.expires_from_now(boost::posix_time::seconds(RECONNECT_INTERVAL));
                    reconnectDeadlineTimer.async_wait([this, endpoint](const boost::system::error_code& error)
                    {
                        if (!error) // not boost::asio::error::operation_aborted
                        {
                            connect(endpoint);
                        }
                    });
                }
            });
        }

        void disconnect()
        {
            signals.cancel();
            connectDeadlineTimer.cancel();
            reconnectDeadlineTimer.cancel();
            socket.close();
            commandLine.close();
        }

        void login()
        {
            Message message;
            message.type = Message::Type::LOGIN;
            message.nickname = nickname;

            sendMessage(message);
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
            boost::asio::streambuf::mutable_buffers_type buffers = inputBuffer.prepare(BUFFER_SIZE);

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

        void readCommandLine()
        {
            boost::asio::async_read_until(commandLine, commandLineBuffer, '\n',
                                        [this](const boost::system::error_code& error,
                                                std::size_t length)
            {
                if (!error)
                {
                    boost::asio::streambuf::const_buffers_type buffers = commandLineBuffer.data();

                    std::string line(boost::asio::buffers_begin(buffers),
                                    boost::asio::buffers_begin(buffers) + length - 1);

                    commandLineBuffer.consume(length);

                    Message message;
                    message.type = Message::Type::TEXT;
                    message.body = line;

                    sendMessage(message);

                    readCommandLine();
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

        boost::asio::posix::stream_descriptor commandLine;
        boost::asio::streambuf commandLineBuffer;

        boost::asio::deadline_timer connectDeadlineTimer;
        boost::asio::deadline_timer reconnectDeadlineTimer;

        boost::asio::signal_set signals;
    };
}
