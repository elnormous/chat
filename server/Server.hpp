//
//  Chat server
//

#pragma once

#include <memory>
#include <set>
#include <string>
#include <boost/asio.hpp>
#include "cereal/cereal.hpp"
#include "spdlog/spdlog.h"
#include "Message.hpp"

class Client;

class Server final
{
public:
    Server(const std::shared_ptr<spdlog::logger>& l,
           boost::asio::io_service& s,
           const boost::asio::ip::tcp::endpoint& endpoint):
        logger(l),
        ioService(s),
        acceptor(s, endpoint),
        socket(s)
    {
        logger->info("Server started (port: {0})", endpoint.port());
        accept();
    }

    void removeClient(Client& client);
    bool isNicknameAvailable(const std::string& nickname) const;
    void broadcastMessage(const Message& message);

private:
    void accept();

    std::shared_ptr<spdlog::logger> logger;
    boost::asio::io_service& ioService;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;
    std::set<std::unique_ptr<Client>> clients;
};
