#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/iterator_range.hpp>
#include "./eval.hpp"
#include "./io_primitives.hpp"
#include "./primitives.hpp"
#include "./read.hpp"
#include "./show.hpp"

using namespace iolisp;

environment primitive_bindings()
{
    auto const env = std::make_shared<std::map<std::string, std::shared_ptr<value>>>();
    for (auto const &prim : primitives())
        env->insert({
            prim.first,
            std::make_shared<value>(value::make<primitive_function>(prim.second))});
    for (auto const &io_prim : io_primitives())
        env->insert({
            io_prim.first,
            std::make_shared<value>(value::make<io_function>(io_prim.second))});
    return env;
}

void eval_and_print(environment const &env, std::string const &input)
try
{
    std::cout << eval(env, read(input)) << std::endl;
}
catch (error const &e)
{
    std::cerr << e.what() << std::endl;
}

void run_repl()
{
    auto const env = primitive_bindings();
    while (true)
    {
        std::cout << "iolisp>>> ";
        std::string input;
        std::getline(std::cin, input);
        if (input == "quit")
            break;
        else
            eval_and_print(env, input);
    }
}

void run_one(boost::iterator_range<char **> rng)
{
    auto const env = primitive_bindings();
    auto const args = rng | boost::adaptors::transformed(&value::make<string>);
    define_variable(env, "args", value::make<list>({boost::begin(args), boost::end(args)}));
    auto const val = value::make<list>({
        value::make<atom>("load"),
        value::make<string>(*rng.begin())});
    std::cout << eval(env, val) << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
        run_repl();
    else
        run_one(boost::make_iterator_range(argv + 2, argv + argc));
}
