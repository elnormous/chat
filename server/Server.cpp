//
//  Chat server
//

#include "Server.hpp"
#include "Client.hpp"

namespace chat
{
    void Server::accept()
    {
        acceptor.async_accept(socket,
                            [this](boost::system::error_code error)
        {
            if (!error)
            {
                clients.insert(std::unique_ptr<Client>(new Client(logger, ioService, *this, std::move(socket))));

                accept();
            }
        });
    }

    void Server::removeClient(Client& client)
    {
        for (auto i = clients.begin(); i != clients.end(); ++i)
        {
            if (i->get() == &client)
            {
                clients.erase(i);
                break;
            }
        }
    }

    bool Server::isNicknameAvailable(const std::string& nickname) const
    {
        for (const auto& client : clients)
        {
            if (client->isLoggedIn() && client->getNickname() == nickname)
                return false;
        }

        return true;
    }

    void Server::broadcastMessage(const Message& message)
    {
        for (const auto& client : clients)
        {
            if (client->isLoggedIn()) client->sendMessage(message);
        }
    }
}
