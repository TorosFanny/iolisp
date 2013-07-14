#ifndef IOLISP_IO_PRIMITIVES_HPP
#define IOLISP_IO_PRIMITIVES_HPP

#include <array>
#include <cstddef>
#include <fstream>
#include <functional>
#include <ios>
#include <map>
#include <memory>
#include <string>
#include <boost/assert.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/range/functions.hpp>
#include <boost/scope_exit.hpp>
#include "./eval.hpp"
#include "./read.hpp"
#include "./show.hpp"
#include "./value.hpp"

namespace iolisp
{
namespace io_primitives_detail
{
inline value apply_proc(arguments args)
{
    if (boost::size(args) == 2 && (boost::begin(args) + 1)->is<list>())
        return apply(*boost::begin(args), (boost::begin(args) + 1)->get<list>());
    else if (boost::size(args) >= 1)
        return apply(*boost::begin(args), args | boost::adaptors::sliced(1, boost::size(args)));
    BOOST_ASSERT(false);
}

inline value make_port(std::ios::openmode mode, arguments args)
{
    if (boost::size(args) == 1 && boost::begin(args)->is<string>())
    {
        return value::make<port>(
            std::make_shared<std::fstream>(boost::begin(args)->get<string>(), mode));
    }
    BOOST_ASSERT(false);
}

inline value close_port(arguments args)
{
    if (boost::size(args) == 1 && boost::begin(args)->is<port>())
    {
        boost::begin(args)->get<port>()->close();
        return value::make<bool_>(true);
    }
    return value::make<bool_>(false);
}

inline value read_proc(arguments args)
{
    if (boost::empty(args))
    {
        std::string ret;
        std::getline(std::cin, ret);
        return value::make<string>(ret);
    }
    else if (boost::size(args) == 1 && boost::begin(args)->is<port>())
    {
        std::string ret;
        std::getline(*boost::begin(args)->get<port>(), ret);
        return value::make<string>(ret);
    }
    BOOST_ASSERT(false);
}

inline value write_proc(arguments args)
{
    if (boost::size(args) == 1)
    {
        std::cout << *boost::begin(args) << std::endl;
        return value::make<bool_>(true);
    }
    else if (boost::size(args) == 2 && (boost::begin(args) + 1)->is<port>())
    {
        *(boost::begin(args) + 1)->get<port>() << *boost::begin(args) << std::endl;
        return value::make<bool_>(true);
    }
    BOOST_ASSERT(false);
}

inline std::string read_file(std::string const &filename)
{
    std::ifstream ifs(filename);
    ifs.seekg(0, std::ifstream::end);
    auto const len = ifs.tellg();
    ifs.seekg(0, std::ifstream::beg);
    std::string buf(len, '\0');
    if (len != 0)
        ifs.read(&buf.front(), len);
    return buf;
}

inline value read_contents(arguments args)
{
    if (boost::size(args) == 1 && boost::begin(args)->is<string>())
        return value::make<string>(read_file(boost::begin(args)->get<string>()));
    BOOST_ASSERT(false);
}

inline value read_all(arguments args)
{
    if (boost::size(args) == 1 && boost::begin(args)->is<string>())
        return value::make<list>(load(boost::begin(args)->get<string>()));
    BOOST_ASSERT(false);
}
}

inline std::vector<value> load(std::string const &filename)
{
    return read_expr_list(io_primitives_detail::read_file(filename));
}

inline std::map<std::string, std::function<value (arguments)>> io_primitives()
{
    using namespace io_primitives_detail;
    using namespace std::placeholders;
    return {
        {"apply", &apply_proc},
        {"open-input-file", std::bind(&make_port, std::fstream::in, _1)},
        {"open-output-file", std::bind(&make_port, std::fstream::out, _1)},
        {"close-input-port", &close_port},
        {"close-output-port", &close_port},
        {"read", &read_proc},
        {"write", &write_proc},
        {"read-contents", &read_contents},
        {"read-all", &read_all}};
}
}

#endif
