//
//  Chat client
//

#include <cstdlib>
#include <iostream>
#include <boost/asio/basic_socket.hpp>
#include "args.hxx"

int main(int argc, const char * argv[])
{
    args::ArgumentParser parser("A simple command-line chat program.");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<std::string> address(parser, "address", "Address of the server", {'a', "address"}, args::Options::Required);
    args::ValueFlag<uint16_t> portNumber(parser, "portNumber", "Port number to connect to", {'p', "port"}, args::Options::Required);
    args::ValueFlag<std::string> nickname(parser, "nickname", "Nicname of the user", {'n', "nickname"}, args::Options::Required);

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

    return EXIT_SUCCESS;
}
