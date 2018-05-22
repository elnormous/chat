//
//  Chat client
//

#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include "args.hxx"
#include "Client.hpp"

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

        std::unordered_map<std::string, spdlog::level::level_enum> map {
            {"trace", spdlog::level::trace},
            {"debug", spdlog::level::debug},
            {"info", spdlog::level::info},
            {"warn", spdlog::level::warn},
            {"err", spdlog::level::err},
            {"critical", spdlog::level::critical},
            {"off", spdlog::level::off}
        };

        args::MapFlag<std::string, spdlog::level::level_enum> logLevel(parser, "level", "Log level", {'l', "level"}, map);

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
            console->set_level(logLevel.Get());

            boost::asio::io_service ioService;

            boost::asio::ip::tcp::resolver resolver(ioService);

            auto endpointIterator = resolver.resolve(address.Get(), port.Get());

            Client client(console, ioService, endpointIterator, nickname.Get());

            // run io service in a separate thread
            std::thread thread([&ioService]() {
                ioService.run();
            });

            std::string line;
            for (;;)
            {
                std::getline(std::cin, line);

                ioService.post([&client, line]() {
                    Message message;
                    message.type = Message::Type::TEXT;
                    message.body = line;

                    client.sendMessage(message);
                });
            }

            // thread.join();
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
