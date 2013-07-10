#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include <iostream>
#include <string>
#include "./eval.hpp"
#include "./read.hpp"
#include "./show.hpp"

using namespace iolisp;

void eval_and_print(std::string const &input)
try
{
    std::cout << eval(read(input)) << std::endl;
}
catch (error const &e)
{
    std::cerr << e.what() << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
        while (true)
        {
            std::cout << "Lisp>>> ";
            std::string input;
            std::getline(std::cin, input);
            if (input == "quit")
                break;
            else
                eval_and_print(input);
        }
    else if (argc == 2)
        eval_and_print(argv[1]);
    else
        std::cout << "Program takes only 0 or 1 argument" << std::endl;
}
