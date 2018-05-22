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
           boost::asio::io_service& ioService,
           const boost::asio::ip::tcp::endpoint& endpoint):
        logger(l),
        acceptor(ioService, endpoint),
        socket(ioService)
    {
        logger->info("Server started (port: {0})", endpoint.port());
        accept();
    }

    void removeClient(Client& client);
    bool isNicknameAvailable(const std::string& nickname) const;
    void broadcast(Client& client, const std::string& text);

private:
    void accept();

    std::shared_ptr<spdlog::logger> logger;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;
    std::set<std::unique_ptr<Client>> clients;
};
