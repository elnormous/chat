//
//  Chat server
//

#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include "args.hxx"

class Client final
{
public:
    explicit Client(boost::asio::ip::tcp::socket sock):
        socket(std::move(sock))
    {
        std::cout << "Client connected" << std::endl;
    }

private:
    boost::asio::ip::tcp::socket socket;
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

private:
    void accept()
    {
        acceptor.async_accept(socket,
                              [this](boost::system::error_code e) {
                                   if (!e)
                                       clients.push_back(Client(std::move(socket)));

                                   accept();
                               });
    }

    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::ip::tcp::socket socket;
    std::vector<Client> clients;
};

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
        std::cout << e.what();
        return EXIT_SUCCESS;
    }
    catch (args::Help)
    {
        std::cout << parser;
        return EXIT_SUCCESS;
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
