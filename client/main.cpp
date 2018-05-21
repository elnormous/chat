//
//  Chat client
//

#include <cstdlib>
#include <iostream>
#include <boost/asio.hpp>
#include "args.hxx"
#include "spdlog/spdlog.h"

class Client final
{
public:
    Client(const std::shared_ptr<spdlog::logger>& l,
           boost::asio::io_service& s,
           boost::asio::ip::tcp::resolver::iterator endpoints):
        logger(l),
        ioService(s),
        socket(s)
    {
        boost::system::error_code error;
        boost::asio::connect(socket, endpoints, error);

        if (!error)
            logger->info("Connected");
        else
            throw std::runtime_error("Failed to connect");

        receive();
    }

    void sendMessage(const std::string& message)
    {
        socket.send(boost::asio::buffer(message.data(), message.length()));
    }

private:
    void disconnect()
    {

    }

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

    std::shared_ptr<spdlog::logger> logger;
    boost::asio::io_service& ioService;
    boost::asio::ip::tcp::socket socket;
    std::vector<uint8_t> buffer = std::vector<uint8_t>(1024);
};

int main(int argc, const char * argv[])
{
    try
    {
        auto console = spdlog::stdout_color_mt("console");

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

            boost::asio::ip::tcp::resolver resolver(ioService);

            auto endpointIterator = resolver.resolve(address.Get(), port.Get());

            Client client(console, ioService, endpointIterator);

            // run io service in a separate thread
            std::thread thread([&ioService]() {
                ioService.run();
            });

            std::string line;
            for (;;)
            {
                std::getline(std::cin, line);

                ioService.post([&client, line]() {
                    client.sendMessage(line);
                });

                // TODO: encode and send message
            }

            thread.join();
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
