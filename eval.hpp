#ifndef IOLISP_EVAL_HPP
#define IOLISP_EVAL_HPP

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <boost/range/adaptors.hpp>
#include <boost/range/functions.hpp>
#include <boost/optional.hpp>
#include "./errors.hpp"
#include "./value.hpp"

namespace iolisp
{
namespace eval_detail
{
inline bool is_bound(environment const &env, std::string const &var)
{
    return env->find(var) != env->end();
}

inline value get_variable(environment const &env, std::string const &var)
{
    auto const it = env->find(var);
    if (it != env->end())
        return *(it->second);
    throw unbound_variable("Getting an unbound variable: ", var);
}

inline value set_variable(environment const &env, std::string const &var, value const &val)
{
    auto const it = env->find(var);
    if (it != env->end())
    {
        *(it->second) = val;
        return val;
    }
    throw unbound_variable("Setting an unbound variable: ", var);
}

inline value define_variable(environment const &env, std::string const &var, value const &val)
{
    auto const it = env->find(var);
    if (it != env->end())
    {
        *(it->second) = val;
        return val;
    }
    else
    {
        env->insert(it, {var, std::make_shared<value>(val)});
        return val;
    }
}

template <class Parameters, class Body>
inline value make_function(
    Parameters const &params,
    boost::optional<value> const &varargs,
    Body const &body,
    environment const &env)
{
    auto const param_strs = params | boost::adaptors::transformed(&show);
    return value::make<function>({
        {boost::begin(param_strs), boost::end(param_strs)},
        varargs ? boost::make_optional(show(*varargs)) : boost::none,
        {boost::begin(body), boost::end(body)},
        env});
}
}
value eval(environment const &envm, value const &val);

template <class Args>
inline value apply(value const &func, Args const &args)
{
    if (func.is<primitive_function>())
        return func.get<primitive_function>()(args);
    else if (func.is<io_function>())
        return func.get<io_function>()(args);
    else if (func.is<function>())
    {
        auto const &rep = func.get<function>();
        if (rep.parameters.size() != boost::size(args) && !rep.variadic_argument)
            throw wrong_number_of_arguments(rep.parameters.size(), args);
        else
        {
            auto const closure =
                std::make_shared<std::map<std::string, std::shared_ptr<value>>>(*rep.closure);
            auto it2 = boost::begin(args);
            for (
                auto it1 = rep.parameters.begin();
                it1 != rep.parameters.end() && it2 != boost::end(args);
                ++it1, ++it2)
                (*closure)[*it1] = std::make_shared<value>(*it2);
            if (rep.variadic_argument)
                (*closure)[*rep.variadic_argument] =
                    std::make_shared<value>(value::make<list>({it2, boost::end(args)}));
            value ret;
            for (auto const &val : rep.body)
                ret = eval(closure, val);
            return ret;
        }
    }
    throw not_function("Unrecognized primitive function args", show(func));
}

std::vector<value> load(std::string const &filename);

inline value eval(environment const &env, value const &val)
{
    // eval env val@(Number _) = val
    if (val.is<number>())
        return val;
    // eval env val@(String _) = val
    else if (val.is<string>())
        return val;
    // eval env val@(Bool _) = val
    else if (val.is<bool_>())
        return val;
    // eval env val@(Atom var) = getVar env var
    if (val.is<atom>())
        return eval_detail::get_variable(env, val.get<atom>());
    else if (val.is<list>())
    {
        auto const &vec = val.get<list>();
        // eval env (List [Atom "quote", val]) = val
        if (vec.size() == 2 && vec[0].is<atom>() && vec[0].get<atom>() == "quote")
            return vec[1];
        // eval env (List [Atom "if", pred, conseq, alt]) = case eval env pred of
        //   Bool False -> eval alt
        //   _          -> eval conseq
        else if (vec.size() == 4 && vec[0].is<atom>() && vec[0].get<atom>() == "if")
        {
            auto const res = eval(env, vec[1]);
            return res.is<bool_>() && !res.get<bool_>() ? eval(env, vec[3]) : eval(env, vec[2]);
        }
        // eval env (List [Atom "set!", Atom var, form]) = setVar env var (eval env form)
        else if (
            vec.size() == 3 &&
            vec[0].is<atom>() && vec[0].get<atom>() == "set!" &&
            vec[1].is<atom>())
            return eval_detail::set_variable(env, vec[1].get<atom>(), eval(env, vec[2]));
        // eval env (List [Atom "define", Atom var, form]) = defineVar env var (eval env form)
        else if (
            vec.size() == 3 &&
            vec[0].is<atom>() && vec[0].get<atom>() == "define" &&
            vec[1].is<atom>())
            return eval_detail::define_variable(env, vec[1].get<atom>(), eval(env, vec[2]));
        // eval env (List (Atom "define" : List (Atom var : params) : body)) = ...
        else if (
            vec.size() >= 2 &&
            vec[0].is<atom>() && vec[0].get<atom>() == "define" &&
            vec[1].is<list>())
        {
            auto const &var_params = vec[1].get<list>();
            if (!var_params.empty() && var_params[0].is<atom>())
                return eval_detail::define_variable(
                    env,
                    var_params[0].get<atom>(),
                    eval_detail::make_function(
                        var_params | boost::adaptors::sliced(1, var_params.size()),
                        boost::none,
                        vec | boost::adaptors::sliced(2, vec.size()),
                        env));
        }
        // eval env (DottedList (Atom "define" : List (Atom var : params)) varargs : body)) = ...
        else if (
            vec.size() >= 2 &&
            vec[0].is<atom>() && vec[0].get<atom>() == "define" &&
            vec[1].is<dotted_list>())
        {
            auto const &var_params = vec[1].get<dotted_list>().first;
            if (!var_params.empty() && var_params[0].is<atom>())
                return eval_detail::define_variable(
                    env,
                    var_params[0].get<atom>(),
                    eval_detail::make_function(
                        var_params | boost::adaptors::sliced(1, var_params.size()),
                        vec[1].get<dotted_list>().second,
                        vec | boost::adaptors::sliced(2, vec.size()),
                        env));
        }
        // eval env (List [Atom "lambda" : List params : body]) = ...
        else if (
            vec.size() >= 2 &&
            vec[0].is<atom>() && vec[0].get<atom>() == "lambda" &&
            vec[1].is<list>())
            return eval_detail::make_function(
                vec[1].get<list>(),
                boost::none,
                vec | boost::adaptors::sliced(2, vec.size()),
                env);
        // eval env (List [Atom "lambda" : DottedList params varargs : body]) = ...
        else if (
            vec.size() >= 2 &&
            vec[0].is<atom>() && vec[0].get<atom>() == "lambda" &&
            vec[1].is<dotted_list>())
            return eval_detail::make_function(
                vec[1].get<dotted_list>().first,
                vec[2].get<dotted_list>().second,
                vec | boost::adaptors::sliced(2, vec.size()),
                env);
        // eval env (List [Atom "lambda" : varargs@(Atom _) : body]) = ...
        else if (
            vec.size() >= 2 &&
            vec[0].is<atom>() && vec[0].get<atom>() == "lambda" &&
            vec[1].is<atom>())
            return eval_detail::make_function(
                std::array<value, 0>(),
                boost::none,
                vec | boost::adaptors::sliced(2, vec.size()),
                env);
        // eval env (List [Atom "load", String filename]) = ...
        else if (
            vec.size() == 2 &&
            vec[0].is<atom>() && vec[0].get<atom>() == "load" &&
            vec[1].is<string>())
        {
            auto const exprs = load(vec[1].get<string>());
            value ret;
            for (auto const &expr : exprs)
                ret = eval(env, expr);
            return ret;
        }
        // eval env (List (function : args)) = ...
        else if (!vec.empty())
        {
            struct eval_env
            {
                environment env;
                value operator()(value const &val) const
                {
                    return eval(env, val);
                }
            };
            return apply(
                eval(env, vec[0]),
                vec | boost::adaptors::sliced(1, vec.size()) 
                    | boost::adaptors::transformed(eval_env{env}));
        }
    }
    throw bad_special_form("Unrecognized special form", val);
}

using eval_detail::define_variable;
}

#include "./io_primitives.hpp"

#endif
