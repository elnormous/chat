//
//  Chat client
//

#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include "args.hxx"

class Client final
{
public:
    Client(boost::asio::io_service& s,
           boost::asio::ip::tcp::resolver::iterator endpoints):
        ioService(s),
        socket(s)
    {
        boost::system::error_code error;
        boost::asio::connect(socket, endpoints, error);

        if (!error)
            std::cout << "Connected" << std::endl;
        else
            throw std::runtime_error("Failed to connect");
    }

private:
    boost::asio::io_service& ioService;
    boost::asio::ip::tcp::socket socket;
};

int main(int argc, const char * argv[])
{
    args::ArgumentParser parser("A simple command-line chat program.");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> address(parser, "address", "Address of the server", {'a', "address"}, args::Options::Required);
    args::ValueFlag<std::string> port(parser, "port", "Port number to connect to", {'p', "port"}, args::Options::Required);
    args::ValueFlag<std::string> nickname(parser, "nickname", "Nicname of the user", {'n', "nickname"}, args::Options::Required);

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

        boost::asio::ip::tcp::resolver resolver(ioService);

        auto endpointIterator = resolver.resolve(address.Get(), port.Get());

        Client client(ioService, endpointIterator);

        // run io service in a separate thread
        std::thread thread([&ioService]() {
            ioService.run();
        });

        std::string line;
        for (;;)
        {
            std::getline(std::cin, line);

            std::cout << line << std::endl;

            // TODO: encode and send message
        }

        thread.join();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return EXIT_SUCCESS;
}
