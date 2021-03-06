#ifndef IOLISP_READ_HPP
#define IOLISP_READ_HPP

#include <ios>
#include <istream>
#include <string>
#include <vector>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_istream_iterator.hpp>
#include "./errors.hpp"
#include "./value.hpp"

namespace iolisp
{
namespace read_detail
{
namespace ascii = boost::spirit::ascii;
namespace phx = boost::phoenix;
namespace qi = boost::spirit::qi;

template <class Iterator>
class value_grammar
  : public qi::grammar<Iterator, value (), ascii::space_type>
{
public:
    value_grammar()
      : value_grammar::base_type(expr_)
    {
        symbol_ = ascii::char_("!#$%&|*+/:<=>?@^_~") | ascii::char_('-');

        atom_ = qi::lexeme[qi::as_string[(ascii::alpha | symbol_) >> *(ascii::alnum | symbol_)][
            phx::bind(
                [](value &val, std::string const &attr)
                {
                    if (attr == "#t")
                        val = value::make<bool_>(true);
                    else if (attr == "#f")
                        val = value::make<bool_>(false);
                    else
                        val = value::make<atom>(attr);
                },
                qi::_val, qi::_1)]];

        list_ = ('(' >> *expr_ >> ')')[
            phx::bind(
                [](value &val, std::vector<value> const &attr)
                {
                    val = value::make<list>(attr);
                },
                qi::_val, qi::_1)];

        dotted_list_ = ('(' >> *expr_ > '.' > expr_ > ')')[
            phx::bind(
                [](value &val, std::vector<value> const &init, value const &last)
                {
                    val = value::make<dotted_list>({init, last});
                },
                qi::_val, qi::_1, qi::_2)];

        string_ = qi::lexeme[qi::as_string['"' >> *(ascii::char_ - '"') >> '"'][
            phx::bind(
                [](value &val, std::string const &attr)
                {
                    val = value::make<string>(attr);
                },
                qi::_val, qi::_1)]];

        number_ = qi::uint_[
            phx::bind(
                [](value &val, unsigned int attr)
                {
                    val = value::make<number>(attr);
                },
                qi::_val, qi::_1)];

        quoted_ = (qi::lexeme['\'' >> !ascii::space] > expr_)[
            phx::bind(
                [](value &val, value const &attr)
                {
                    val = value::make<list>({value::make<atom>("quote"), attr});
                },
                qi::_val, qi::_1)];

        expr_ = atom_ | list_ | dotted_list_ | string_ | number_ | quoted_;

        expr_.name("expr");

        qi::on_error(
            expr_,
            phx::bind(
                [this](Iterator first, Iterator, Iterator error_pos, qi::info const &info)
                {
                    std::ostringstream s;
                    s << "column " << (error_pos - first + 1) << ": expecting " << info;
                    error_ = s.str();
                },
                qi::_1, qi::_2, qi::_3, qi::_4));
    }

    std::string get_error() const
    {
        return error_;
    }

private:
    qi::rule<Iterator, value (), ascii::space_type>
    expr_, atom_, list_, dotted_list_, string_, number_, quoted_;
    qi::rule<Iterator, char ()> symbol_;
    std::string error_;
};
}

template <class C, class CT>
inline std::basic_istream<C, CT> &operator>>(std::basic_istream<C, CT> &is, value &val)
{
    using iterator = boost::spirit::basic_istream_iterator<C, CT>;
    read_detail::value_grammar<iterator> expr;
    auto it = iterator(is);
    auto const res = boost::spirit::qi::phrase_parse(
        it,
        iterator(),
        expr,
        boost::spirit::ascii::space,
        val);
    if (!res)
        is.setstate(std::ios::failbit);
    return is;
}

inline value read(std::string const &input)
{
    read_detail::value_grammar<std::string::const_iterator> expr;
    auto it = input.begin();
    value val;
    auto const res = boost::spirit::qi::phrase_parse(
        it,
        input.end(),
        expr,
        boost::spirit::ascii::space,
        val);
    if (!res)
        throw parse_error(expr.get_error());
    return val;
}

inline std::vector<value> read_expr_list(std::string const &input)
{
    read_detail::value_grammar<std::string::const_iterator> expr;
    auto it = input.begin();
    std::vector<value> vals;
    auto const res = boost::spirit::qi::phrase_parse(
        it,
        input.end(),
        *expr,
        boost::spirit::ascii::space,
        vals);
    if (!res)
        throw parse_error(expr.get_error());
    return vals;
}
}

#endif
