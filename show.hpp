#ifndef IOLISP_SHOW_HPP
#define IOLISP_SHOW_HPP

#include <ostream>
#include <string>
#include <boost/fusion/include/at_c.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include "./value.hpp"

namespace iolisp
{
template <class C, class CT>
inline std::basic_ostream<C, CT> &operator<<(std::basic_ostream<C, CT> &s, value const &v)
{
    if (v.is<atom>())
        s << v.get<atom>();
    else if (v.is<list>())
    {
        s << '(';
        auto const &vec = v.get<list>();
        if (!vec.empty())
        {
            s << vec.front();
            for (auto const &val : vec | boost::adaptors::sliced(1, vec.size()))
                s << ' ' << val;
        }
        s << ')';
    }
    else if (v.is<dotted_list>())
    {
        s << '(';
        auto const &p = v.get<dotted_list>();
        for (auto const &val : boost::fusion::at_c<0>(p))
            s << val << ' ';
        s << ". " << boost::fusion::at_c<1>(p) << ')';
    }
    else if (v.is<number>())
        s << v.get<number>();
    else if (v.is<string>())
        s << '"' << v.get<string>() << '"';
    else if (v.is<bool_>())
        s << (v.get<bool_>() ? "#t" : "#f");
    return s;
}

inline std::string show(value const &v)
{
    return boost::lexical_cast<std::string>(v);
}
}

#endif
