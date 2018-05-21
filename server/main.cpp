//
//  Chat server
//

#include <cstdlib>
#include <iostream>
#include <vector>
#include "args.hxx"
#include "Server.hpp"
#include "Client.hpp"

class Server;

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
