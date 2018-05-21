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
#include "spdlog/spdlog.h"

class Server;

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

private:
    void receive()
    {
        socket.async_receive(boost::asio::buffer(buffer.data(), buffer.size()),
                             [this](const boost::system::error_code& error, std::size_t bytesTransferred) {
                                 if (error)
                                 {
                                     logger->info("Disconnected");
                                     disconnect();
                                 }
                                 else
                                 {
                                     logger->info("Received {0} bytes", bytesTransferred);
                                     receive();
                                 }
                             });
    }

    void disconnect();

    std::shared_ptr<spdlog::logger> logger;
    Server& server;
    boost::asio::ip::tcp::socket socket;
    std::vector<uint8_t> buffer = std::vector<uint8_t>(1024);
};

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
                                       clients.insert(std::unique_ptr<Client>(new Client(logger, *this, std::move(socket))));

                                   accept();
                               });
    }

    std::shared_ptr<spdlog::logger> logger;
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
    try
    {
        auto console = spdlog::stdout_color_mt("console");

        args::ArgumentParser parser("A simple chat server.");
        args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
        args::ValueFlag<uint16_t> port(parser, "port", "Port number to listen to", {'p', "port"}, args::Options::Required);

        try
        {
            parser.ParseCLI(argc, argv);
        }
        catch (args::Completion e)
        {
            // log this in a user friendly way (without spdlog)
            std::cerr << e.what() << std::endl;
            return EXIT_SUCCESS;
        }
        catch (args::Help)
        {
            // log this in a user friendly way (without spdlog)
            std::cout << parser << std::endl;
            return EXIT_SUCCESS;
        }
        catch (args::RequiredError e)
        {
            // log this in a user friendly way (without spdlog)
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        catch (args::ParseError e)
        {
            // log this in a user friendly way (without spdlog)
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }

        try
        {
            boost::asio::io_service ioService;
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port.Get());

            Server server(console, ioService, endpoint);

            ioService.run();
        }
        catch (std::exception& e)
        {
            console->error("{0}", e.what());
        }
    }
    catch (const spdlog::spdlog_ex& e)
    {
        std::cerr << "Log initialization failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
