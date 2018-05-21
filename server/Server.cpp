//
//  Chat server
//

#include "Server.hpp"
#include "Client.hpp"

void Server::accept()
{
    acceptor.async_accept(socket,
                          [this](boost::system::error_code e) {
                              if (!e)
                                  clients.insert(std::unique_ptr<Client>(new Client(logger, *this, std::move(socket))));

                              accept();
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
