#ifndef IOLISP_VALUE_HPP
#define IOLISP_VALUE_HPP

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <boost/assert.hpp>
#include <boost/fusion/include/pair.hpp>
#include <boost/iterator.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/map.hpp>
#include <boost/mpl/pair.hpp>
#include <boost/optional.hpp>
#include <boost/range/any_range.hpp>
#include <boost/variant.hpp>

namespace iolisp
{
struct atom {};
struct list {};
struct dotted_list {};
struct number {};
struct string {};
struct bool_ {};
struct primitive_function {};
struct function {};

class value;

using arguments = boost::any_range<
    value,
    boost::random_access_traversal_tag,
    value,
    std::ptrdiff_t>;

using environment = std::shared_ptr<std::map<
    std::string,
    std::shared_ptr<value>>>;

class value
{
public:
    struct function_rep
    {
        std::vector<std::string> parameters;
        boost::optional<std::string> variadic_argument;
        std::vector<value> body;
        environment closure;
    };

    using reps = boost::mpl::map<
        boost::mpl::pair<atom, std::string>,
        boost::mpl::pair<list, std::vector<value>>,
        boost::mpl::pair<dotted_list, std::pair<std::vector<value>, value>>,
        boost::mpl::pair<number, int>,
        boost::mpl::pair<string, std::string>,
        boost::mpl::pair<bool_, bool>,
        boost::mpl::pair<primitive_function, std::function<value (arguments)>>,
        boost::mpl::pair<function, function_rep>>;

    template <class Type>
    using rep = typename boost::mpl::at<reps, Type>::type;

    value()
      : value(make<list>({}))
    {}

    template <class Type>
    static value make(rep<Type> const &r)
    {
        return value(boost::fusion::make_pair<Type>(r));
    }

    template <class Type>
    bool is() const
    {
        return impl_.type() == typeid(boost::fusion::pair<Type, rep<Type>>);
    }

    template <class Type>
    rep<Type> &get()
    {
        BOOST_ASSERT(is<Type>());
        return boost::get<boost::fusion::pair<Type, rep<Type>>>(impl_).second;
    }

    template <class Type>
    rep<Type> const &get() const
    {
        BOOST_ASSERT(is<Type>());
        return boost::get<boost::fusion::pair<Type, rep<Type>>>(impl_).second;
    }

private:
    using impl = boost::variant<
        boost::fusion::pair<atom, rep<atom>>,
        boost::recursive_wrapper<boost::fusion::pair<list, rep<list>>>,
        boost::recursive_wrapper<boost::fusion::pair<dotted_list, rep<dotted_list>>>,
        boost::fusion::pair<number, rep<number>>,
        boost::fusion::pair<string, rep<string>>,
        boost::fusion::pair<bool_, rep<bool_>>,
        boost::fusion::pair<primitive_function, rep<primitive_function>>,
        boost::fusion::pair<function, rep<function>>>;

    template <class Type>
    value(boost::fusion::pair<Type, rep<Type>> const &p)
      : impl_(p)
    {}

    impl impl_;
};
}

#endif
