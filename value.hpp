#ifndef IOLISP_VALUE_HPP
#define IOLISP_VALUE_HPP

#include <string>
#include <type_traits>
#include <vector>
#include <boost/assert.hpp>
#include <boost/fusion/include/pair.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/variant.hpp>

namespace iolisp
{
struct atom {};
struct list {};
struct dotted_list {};
struct number {};
struct string {};
struct bool_ {};

class value
{
private:
    using reps = boost::mpl::map<
        boost::mpl::pair<atom, std::string>,
        boost::mpl::pair<list, std::vector<value>>,
        boost::mpl::pair<dotted_list, boost::fusion::vector<std::vector<value>, value>>,
        boost::mpl::pair<number, int>,
        boost::mpl::pair<string, std::string>,
        boost::mpl::pair<bool_, bool>>;

public:
    template <class Type>
    using rep = typename boost::mpl::at<reps, Type>::type;

    value()
      : value(boost::fusion::make_pair<list>(rep<list>{}))
    {}

    template <class Type>
    static value make(rep<Type> const &r)
    {
        return value(boost::fusion::make_pair<Type>(r));
    }

    template <class Type>
    bool is() const
    {
        return m_impl.type() == typeid(boost::fusion::pair<Type, rep<Type>>);
    }

    template <class Type>
    rep<Type> &get()
    {
        BOOST_ASSERT(is<Type>());
        return boost::get<boost::fusion::pair<Type, rep<Type>>>(m_impl).second;
    }

    template <class Type>
    rep<Type> const &get() const
    {
        BOOST_ASSERT(is<Type>());
        return boost::get<boost::fusion::pair<Type, rep<Type>>>(m_impl).second;
    }

private:
    using impl = boost::variant<
        boost::fusion::pair<atom, rep<atom>>,
        boost::recursive_wrapper<boost::fusion::pair<list, rep<list>>>,
        boost::recursive_wrapper<boost::fusion::pair<dotted_list, rep<dotted_list>>>,
        boost::fusion::pair<number, rep<number>>,
        boost::fusion::pair<string, rep<string>>,
        boost::fusion::pair<bool_, rep<bool_>>>;

    template <class Type>
    value(boost::fusion::pair<Type, rep<Type>> const &p)
      : m_impl(p)
    {}

    impl m_impl;
};
}

#endif
