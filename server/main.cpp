//
//  Chat server
//

#include <cstdlib>
#include <iostream>
#include "args.hxx"

int main(int argc, const char * argv[])
{
    args::ArgumentParser parser("A simple chat server.");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::ValueFlag<uint16_t> portNumber(parser, "port number", "Port number to listen to", {'p', "port"}, args::Options::Required);

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
