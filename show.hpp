#ifndef IOLISP_SHOW_HPP
#define IOLISP_SHOW_HPP

#include <ostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include "./value.hpp"

namespace iolisp
{
template <class C, class CT>
inline std::basic_ostream<C, CT> &operator<<(std::basic_ostream<C, CT> &os, value const &val)
{
    if (val.is<atom>())
        os << val.get<atom>();
    else if (val.is<list>())
    {
        os << '(';
        auto const &vec = val.get<list>();
        if (!vec.empty())
        {
            os << vec.front();
            for (auto const &elem : vec | boost::adaptors::sliced(1, vec.size()))
                os << ' ' << elem;
        }
        os << ')';
    }
    else if (val.is<dotted_list>())
    {
        os << '(';
        for (auto const &elem : val.get<dotted_list>().first)
            os << elem << ' ';
        os << ". " << val.get<dotted_list>().second << ')';
    }
    else if (val.is<number>())
        os << val.get<number>();
    else if (val.is<string>())
        os << '"' << val.get<string>() << '"';
    else if (val.is<bool_>())
        os << (val.get<bool_>() ? "#t" : "#f");
    else if (val.is<port>())
        os << "<IO port>";
    else if (val.is<primitive_function>())
        os << "<primitive>";
    else if (val.is<io_function>())
        os << "<IO primitive>";
    else if (val.is<function>())
    {
        auto const &rep = val.get<function>();
        os << "(lambda (";
        if (!rep.parameters.empty())
        {
            os << '"' << rep.parameters.front() << '"';
            for (
                auto const &elem :
                rep.parameters | boost::adaptors::sliced(1, rep.parameters.size()))
                os << " \"" << elem << '"';
        }
        if (rep.variadic_argument)
            os << " . " << *rep.variadic_argument;
        os << ") ...)";
    }
    return os;
}

inline std::string show(value const &val)
{
    return boost::lexical_cast<std::string>(val);
}
}

#endif
