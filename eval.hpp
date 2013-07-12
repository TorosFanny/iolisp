#ifndef IOLISP_EVAL_HPP
#define IOLISP_EVAL_HPP

#include <array>
#include <cstddef>
#include <functional>
#include <string>
#include <boost/assert.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/mpl/string.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/algorithm_ext/insert.hpp>
#include <boost/range/any_range.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/numeric.hpp>
#include "./error.hpp"
#include "./value.hpp"

namespace iolisp
{
namespace eval_detail
{
using arguments = boost::any_range<
    value,
    boost::random_access_traversal_tag,
    value,
    std::ptrdiff_t>;

template <class T>
value::rep<T> unpack(value const &v);

template <>
inline value::rep<number> unpack<number>(value const &v)
{
    if (v.is<number>())
        return v.get<number>();
    else if (v.is<string>())
        try
        {
            return boost::lexical_cast<int>(v.get<string>());
        }
        catch (boost::bad_lexical_cast const &)
        {
            throw type_mismatch("number", v);
        }
    else if (v.is<list>())
    {
        auto const &vec = v.get<list>();
        if (vec.size() == 1)
            return unpack<number>(vec[0]);
    }
    throw type_mismatch("number", v);
}

template <>
inline value::rep<string> unpack<string>(value const &v)
{
    if (v.is<string>())
        return v.get<string>();
    else if (v.is<number>())
        return boost::lexical_cast<std::string>(v.get<number>());
    else if (v.is<bool_>())
        return v.get<bool_>() ? "True" : "False";
    throw type_mismatch("string", v);
}

template <>
inline value::rep<bool_> unpack<bool_>(value const &v)
{
    if (v.is<bool_>())
        return v.get<bool_>();
    throw type_mismatch("boolean", v);
}

inline value number_binary_op(std::function<int (int, int)> fn, arguments args)
{
    if (boost::size(args) < 2)
        throw wrong_number_of_arguments(2, args);
    auto const ns = args | boost::adaptors::transformed(&unpack<number>);
    return value::make<number>(boost::accumulate(
        ns | boost::adaptors::sliced(1, boost::size(ns)),
        *boost::begin(ns),
        fn));
}

template <class Type>
inline value bool_binary_op(
    std::function<bool (value::rep<Type> const &, value::rep<Type> const &)> fn,
    arguments args)
{
    if (boost::size(args) != 2)
        throw wrong_number_of_arguments(2, args);
    return value::make<bool_>(fn(
        unpack<Type>(*boost::begin(args)),
        unpack<Type>(*(boost::begin(args) + 1))));
}

inline value car(arguments args)
{
    if (boost::size(args) == 1)
    {
        auto const &v = *boost::begin(args);
        if (v.is<list>())
        {
            auto const &vec = v.get<list>();
            if (!vec.empty())
                return vec[0];
        }
        else if (v.is<dotted_list>())
        {
            auto const &init = boost::fusion::at_c<0>(v.get<dotted_list>());
            if (init.size() == 2)
                return init[0];
        }
        throw type_mismatch("pair", v);
    }
    throw wrong_number_of_arguments(1, args);
}

inline value cdr(arguments args)
{
    if (boost::size(args) == 1)
    {
        auto const &v = *boost::begin(args);
        if (v.is<list>())
        {
            auto const &vec = v.get<list>();
            if (!vec.empty())
                return value::make<list>({vec.begin() + 1, vec.end()});
        }
        else if (v.is<dotted_list>())
        {
            auto const &p = v.get<dotted_list>();
            auto const &init = boost::fusion::at_c<0>(p);
            auto const &last = boost::fusion::at_c<1>(p);
            if (init.size() == 1)
                return last;
            else if (init.size() > 1)
                return value::make<dotted_list>({{init.begin() + 1, init.end()}, last});
        }
        throw type_mismatch("pair", v);
    }
    throw wrong_number_of_arguments(1, args);
}

inline value cons(arguments args)
{
    if (boost::size(args) == 2)
    {
        auto const &x1 = *boost::begin(args);
        auto const &x2 = *(boost::begin(args) + 1);
        if (x2.is<list>())
        {
            std::vector<value> v(1, x1);
            boost::insert(v, v.begin(), x2.get<list>());
            return value::make<list>(v);
        }
        else if (x2.is<dotted_list>())
        {
            auto const &p = x2.get<dotted_list>();
            std::vector<value> v(1, x1);
            boost::insert(v, v.begin(), boost::fusion::at_c<0>(p));
            return value::make<dotted_list>({v, boost::fusion::at_c<1>(p)});
        }
        else
            return value::make<dotted_list>({{x1}, x2});
    }
    throw wrong_number_of_arguments(2, args);
}

inline value eqv(arguments args)
{
    if (boost::size(args) == 2)
    {
        auto const &x1 = *boost::begin(args);
        auto const &x2 = *(boost::begin(args) + 1);
        if (x1.is<bool_>() && x2.is<bool_>())
            return value::make<bool_>(x1.get<bool_>() == x2.get<bool_>());
        else if (x1.is<number>() && x2.is<number>())
            return value::make<bool_>(x1.get<number>() == x2.get<number>());
        else if (x1.is<string>() && x2.is<string>())
            return value::make<bool_>(x1.get<string>() == x2.get<string>());
        else if (x1.is<atom>() && x2.is<atom>())
            return value::make<bool_>(x1.get<atom>() == x2.get<atom>());
        else if (x1.is<dotted_list>() && x2.is<dotted_list>())
        {
            auto const to_list = [](value const &v) -> value
            {
                auto const &p = v.get<dotted_list>();
                auto vec = boost::fusion::at_c<0>(p);
                vec.push_back(boost::fusion::at_c<1>(p));
                return value::make<list>(vec);
            };
            return eqv(std::array<value, 2>{{to_list(x1), to_list(x2)}});
        }
        else if (x1.is<list>() && x2.is<list>())
            return value::make<bool_>(boost::equal(
                x1.get<list>(),
                x2.get<list>(),
                [](value const &v1, value const &v2) -> bool
                {
                    auto const r = eqv(std::array<value, 2>{{v1, v2}});
                    BOOST_ASSERT(r.is<bool_>());
                    return r.get<bool_>();
                }));
        return value::make<bool_>(false);
    }
    throw wrong_number_of_arguments(2, args);
}

template <class Type>
inline bool unpack_equals(value const &v1, value const &v2)
try
{
    return unpack<Type>(v1) == unpack<Type>(v2);
}
catch (type_mismatch const &)
{
    return false;
}

inline value equal(arguments args)
{
    if (boost::size(args) == 2)
    {
        auto const &v1 = *boost::begin(args);
        auto const &v2 = *(boost::begin(args) + 1);
        if (unpack_equals<number>(v1, v2) ||
            unpack_equals<string>(v1, v2) ||
            unpack_equals<bool_>(v1, v2))
            return value::make<bool_>(true);
        return eqv(std::array<value, 2>{{v1, v2}});
    }
    throw wrong_number_of_arguments(2, args);
}

template <class Range>
inline value apply(std::string const &fn, Range const &args)
{
    using std::placeholders::_1;
    static std::map<std::string, std::function<value (arguments)>> const primitives = {
        {"+", std::bind(&number_binary_op, std::plus<int>(), _1)},
        {"-", std::bind(&number_binary_op, std::minus<int>(), _1)},
        {"*", std::bind(&number_binary_op, std::multiplies<int>(), _1)},
        {"/", std::bind(&number_binary_op, std::divides<int>(), _1)},
        {"mod", std::bind(&number_binary_op, std::modulus<int>(), _1)},
        {"quotient", std::bind(&number_binary_op, std::divides<int>(), _1)},
        {"remainder", std::bind(&number_binary_op, std::modulus<int>(), _1)},
        {"=", std::bind(&bool_binary_op<number>, std::equal_to<int>(), _1)},
        {"<", std::bind(&bool_binary_op<number>, std::less<int>(), _1)},
        {">", std::bind(&bool_binary_op<number>, std::greater<int>(), _1)},
        {"/=", std::bind(&bool_binary_op<number>, std::not_equal_to<int>(), _1)},
        {">=", std::bind(&bool_binary_op<number>, std::greater_equal<int>(), _1)},
        {"<=", std::bind(&bool_binary_op<number>, std::less_equal<int>(), _1)},
        {"&&", std::bind(&bool_binary_op<bool_>, std::logical_and<bool>(), _1)},
        {"||", std::bind(&bool_binary_op<bool_>, std::logical_or<bool>(), _1)},
        {"string=?", std::bind(&bool_binary_op<string>, std::equal_to<std::string>(), _1)},
        {"string<?", std::bind(&bool_binary_op<string>, std::less<std::string>(), _1)},
        {"string>?", std::bind(&bool_binary_op<string>, std::greater<std::string>(), _1)},
        {"string<=?", std::bind(&bool_binary_op<string>, std::less_equal<std::string>(), _1)},
        {"string>=?", std::bind(&bool_binary_op<string>, std::greater_equal<std::string>(), _1)},
        {"car", &car},
        {"cdr", &cdr},
        {"cons", &cons},
        {"eq?", &eqv},
        {"eqv?", &eqv},
        {"equal?", &equal}};
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
    if (v.is<number>())
        return v;
    if (v.is<string>())
        return v;
    if (v.is<bool_>())
        return v;
    if (v.is<list>())
    {
        auto const &vec = v.get<list>();
        if (vec.size() == 2 && vec[0].is<atom>() && vec[0].get<atom>() == "quote")
            return vec[1];
        if (vec.size() == 4 && vec[0].is<atom>() && vec[0].get<atom>() == "if")
        {
            auto const result = eval(vec[1]);
            return eval(result.is<bool_>() && !result.is<bool_>() ? vec[3] : vec[2]);
        }
        if (!vec.empty() && vec[0].is<atom>())
            return eval_detail::apply(
                vec[0].get<atom>(),
                vec | boost::adaptors::sliced(1, vec.size()) | boost::adaptors::transformed(&eval));
    }
    throw bad_special_form("Unrecognized special form", v);
}
}

#endif
