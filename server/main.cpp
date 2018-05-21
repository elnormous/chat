//
//  Chat server
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <vector>
#include <boost/asio.hpp>
#include "args.hxx"

class Server;

class Client final
{
public:
    Client(Server& s,
           boost::asio::ip::tcp::socket sock):
        server(s),
        socket(std::move(sock))
    {
        std::cout << "Client connected" << std::endl;

        receive();
    }

private:
    void receive()
    {
        socket.async_receive(boost::asio::buffer(buffer.data(), buffer.size()),
                             [this](const boost::system::error_code& error, std::size_t bytesTransferred) {
                                 if (error)
                                 {
                                     std::cout << "Disconnected" << std::endl;
                                     disconnect();
                                 }
                                 else
                                 {
                                     std::cout << "Received " << bytesTransferred << " bytes" << std::endl;
                                     receive();
                                 }
                             });
    }

    void disconnect();

    Server& server;
    boost::asio::ip::tcp::socket socket;
    std::vector<uint8_t> buffer = std::vector<uint8_t>(1024);
};

class Server final
{
public:
    Server(boost::asio::io_service& ioService,
           const boost::asio::ip::tcp::endpoint& endpoint):
        acceptor(ioService, endpoint),
        socket(ioService)
    {
        std::cout << "Server started (port: " << endpoint.port() << ")" << std::endl;
        accept();
    }

    void removeClient(Client& client)
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

private:
    void accept()
    {
        acceptor.async_accept(socket,
                              [this](boost::system::error_code e) {
                                   if (!e)
                                       clients.insert(std::unique_ptr<Client>(new Client(*this, std::move(socket))));

                                   accept();
                               });
    }

    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;
    std::set<std::unique_ptr<Client>> clients;
};

void Client::disconnect()
{
    server.removeClient(*this);
}

int main(int argc, const char * argv[])
{
    args::ArgumentParser parser("A simple chat server.");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<uint16_t> port(parser, "port", "Port number to listen to", {'p', "port"}, args::Options::Required);

    try
    {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Completion e)
    {
        std::cout << e.what() << std::endl;
        return EXIT_SUCCESS;
    }
    catch (args::Help)
    {
        std::cout << parser << std::endl;
        return EXIT_SUCCESS;
    }
    catch (args::RequiredError e)
    {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (args::ParseError e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return EXIT_FAILURE;
    }

    try
    {
        boost::asio::io_service ioService;

        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port.Get());

        Server server(ioService, endpoint);

        ioService.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return EXIT_SUCCESS;
}
