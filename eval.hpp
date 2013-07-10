#ifndef IOLISP_EVAL_HPP
#define IOLISP_EVAL_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/any_range.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/numeric.hpp>
#include "./error.hpp"
#include "./value.hpp"

namespace iolisp
{
namespace detail
{
using arguments = boost::any_range<
    value,
    boost::random_access_traversal_tag,
    value,
    std::ptrdiff_t>;

inline value number_variadic_op(std::function<int (int, int)> fn, arguments args)
{
    if (boost::size(args) < 2)
        throw wrong_number_of_arguments(2, args);
    auto const ns = args | boost::adaptors::transformed(
        [](value const &v) -> int
        {
            if (!v.is<number>())
                throw type_mismatch("number", v);
            return v.get<number>();
        });
    return value::make<number>(boost::accumulate(
        ns | boost::adaptors::sliced(1, boost::size(ns)),
        *boost::begin(ns),
        fn));
}

template <class Range>
inline value apply(std::string const &fn, Range const &args)
{
    using std::placeholders::_1;
    static std::map<std::string, std::function<value (arguments)>> const primitives = {
        {"+", std::bind(&number_variadic_op, std::plus<int>(), _1)},
        {"-", std::bind(&number_variadic_op, std::minus<int>(), _1)},
        {"*", std::bind(&number_variadic_op, std::multiplies<int>(), _1)},
        {"/", std::bind(&number_variadic_op, std::divides<int>(), _1)},
        {"mod", std::bind(&number_variadic_op, std::modulus<int>(), _1)},
        {"quotient", std::bind(&number_variadic_op, std::divides<int>(), _1)},
        {"remainder", std::bind(&number_variadic_op, std::modulus<int>(), _1)}};
    auto const it = primitives.find(fn);
    if (it != primitives.end())
        return (it->second)(args);
    throw not_function("Unrecognized primitive function args", fn);
}
}

inline value eval(value const &v)
{
    if (v.is<atom>())
        return v;
    else if (v.is<number>())
        return v;
    else if (v.is<string>())
        return v;
    else if (v.is<bool_>())
        return v;
    else if (v.is<list>())
    {
        auto const &vec = v.get<list>();
        if (vec.size() == 2 && vec[0].is<atom>() && vec[0].get<atom>() == "quote")
            return vec[1];
        else if (!vec.empty() && vec[0].is<atom>())
            return detail::apply(
                vec[0].get<atom>(),
                vec | boost::adaptors::sliced(1, vec.size()) | boost::adaptors::transformed(&eval));
    }
    throw bad_special_form("Unrecognized special form", v);
}
}

#endif
