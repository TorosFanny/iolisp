#ifndef IOLISP_PRIMITIVES_HPP
#define IOLISP_PRIMITIVES_HPP

#include <array>
#include <memory>
#include <utility>
#include <vector>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm_ext/insert.hpp>
#include <boost/range/functions.hpp>
#include <boost/range/numeric.hpp>
#include "./value.hpp"

namespace iolisp
{
namespace primitives_detail
{
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

inline value numeric_binary_op(std::function<int (int, int)> fn, arguments args)
{
    if (boost::size(args) == 1)
        throw wrong_number_of_arguments(2, args);
    auto const ns = args | boost::adaptors::transformed(&unpack<number>);
    return value::make<number>(boost::accumulate(
        ns | boost::adaptors::sliced(1, boost::size(ns)),
        *boost::begin(ns),
        fn));
}

template <class Type>
inline value bool_binary_op(
    std::function<bool (value::rep<Type> const &, value::rep<Type> const &)> func,
    arguments args)
{
    if (boost::size(args) != 2)
        throw wrong_number_of_arguments(2, args);
    return value::make<bool_>(func(
        unpack<Type>(*boost::begin(args)),
        unpack<Type>(*(boost::begin(args) + 1))));
}

inline value car(arguments args)
{
    if (boost::size(args) == 1)
    {
        auto const &val = *boost::begin(args);
        if (val.is<list>())
        {
            auto const &vec = val.get<list>();
            if (!vec.empty())
                return vec[0];
        }
        else if (val.is<dotted_list>())
        {
            auto const &init = val.get<dotted_list>().first;
            if (init.size() == 2)
                return init[0];
        }
        throw type_mismatch("pair", val);
    }
    throw wrong_number_of_arguments(1, args);
}

inline value cdr(arguments args)
{
    if (boost::size(args) == 1)
    {
        auto const &val = *boost::begin(args);
        if (val.is<list>())
        {
            auto const &vec = val.get<list>();
            if (!vec.empty())
                return value::make<list>({vec.begin() + 1, vec.end()});
        }
        else if (val.is<dotted_list>())
        {
            auto const &init = val.get<dotted_list>().first;
            auto const &last = val.get<dotted_list>().second;
            if (init.size() == 1)
                return last;
            else if (init.size() >= 2)
                return value::make<dotted_list>({{init.begin() + 1, init.end()}, last});
        }
        throw type_mismatch("pair", val);
    }
    throw wrong_number_of_arguments(1, args);
}

inline value cons(arguments args)
{
    if (boost::size(args) == 2)
    {
        auto const &head = *boost::begin(args);
        auto const &tail = *(boost::begin(args) + 1);
        if (tail.is<list>())
        {
            std::vector<value> vec{head};
            boost::insert(vec, vec.end(), tail.get<list>());
            return value::make<list>(vec);
        }
        else if (tail.is<dotted_list>())
        {
            std::vector<value> vec{head};
            boost::insert(vec, vec.end(), tail.get<dotted_list>().first);
            return value::make<dotted_list>({vec, tail.get<dotted_list>().second});
        }
        else
            return value::make<dotted_list>({{head}, tail});
    }
    throw wrong_number_of_arguments(2, args);
}

inline value eqv(arguments args)
{
    if (boost::size(args) == 2)
    {
        auto const &lhs = *boost::begin(args);
        auto const &rhs = *(boost::begin(args) + 1);
        if (lhs.is<bool_>() && rhs.is<bool_>())
            return value::make<bool_>(lhs.get<bool_>() == rhs.get<bool_>());
        else if (lhs.is<number>() && rhs.is<number>())
            return value::make<bool_>(lhs.get<number>() == rhs.get<number>());
        else if (lhs.is<string>() && rhs.is<string>())
            return value::make<bool_>(lhs.get<string>() == rhs.get<string>());
        else if (lhs.is<atom>() && rhs.is<atom>())
            return value::make<bool_>(lhs.get<atom>() == rhs.get<atom>());
        else if (lhs.is<dotted_list>() && rhs.is<dotted_list>())
        {
            auto const to_list = [](value const &val) -> value
            {
                auto vec = val.get<dotted_list>().first;
                vec.push_back(val.get<dotted_list>().second);
                return value::make<list>(vec);
            };
            return eqv(std::array<value, 2>{{to_list(lhs), to_list(rhs)}});
        }
        else if (lhs.is<list>() && rhs.is<list>())
            return value::make<bool_>(boost::equal(
                lhs.get<list>(),
                rhs.get<list>(),
                [](value const &val1, value const &val2) -> bool
                {
                    auto const res = eqv(std::array<value, 2>{{val1, val2}});
                    BOOST_ASSERT(res.is<bool_>());
                    return res.get<bool_>();
                }));
        return value::make<bool_>(false);
    }
    throw wrong_number_of_arguments(2, args);
}

template <class Type>
inline bool unpack_equals(value const &lhs, value const &rhs)
try
{
    return unpack<Type>(lhs) == unpack<Type>(rhs);
}
catch (type_mismatch const &)
{
    return false;
}

inline value equal(arguments args)
{
    if (boost::size(args) == 2)
    {
        auto const &lhs = *boost::begin(args);
        auto const &rhs = *(boost::begin(args) + 1);
        if (unpack_equals<number>(lhs, rhs) ||
            unpack_equals<string>(lhs, rhs) ||
            unpack_equals<bool_>(lhs, rhs))
            return value::make<bool_>(true);
        return eqv(std::array<value, 2>{{lhs, rhs}});
    }
    throw wrong_number_of_arguments(2, args);
}
}

inline std::map<std::string, std::function<value (arguments)>> primitives()
{
    using namespace primitives_detail;
    using namespace std::placeholders;
    return {
        {"+", std::bind(&numeric_binary_op, std::plus<int>(), _1)},
        {"-", std::bind(&numeric_binary_op, std::minus<int>(), _1)},
        {"*", std::bind(&numeric_binary_op, std::multiplies<int>(), _1)},
        {"/", std::bind(&numeric_binary_op, std::divides<int>(), _1)},
        {"mod", std::bind(&numeric_binary_op, std::modulus<int>(), _1)},
        {"quotient", std::bind(&numeric_binary_op, std::divides<int>(), _1)},
        {"remainder", std::bind(&numeric_binary_op, std::modulus<int>(), _1)},
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
}
}
#endif
