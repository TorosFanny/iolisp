#ifndef IOLISP_ERROR_HPP
#define IOLISP_ERROR_HPP

#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include "./show.hpp"
#include "./value.hpp"

namespace iolisp
{
class error
  : public std::runtime_error
{
public:
    explicit error(std::string const &message)
      : std::runtime_error(message)
    {}
};

class wrong_number_of_arguments
  : public error
{
public:
    template <class Range>
    wrong_number_of_arguments(int expected, Range const &found)
      : error(build_message(expected, found))
    {}

private:
    template <class Range>
    static std::string build_message(int expected, Range const &found)
    {
        std::ostringstream s;
        s << "Expected " << expected << " args; found values";
        for (auto const &v : found)
            s << ' ' << v;
        return s.str();
    }
};

class type_mismatch
  : public error
{
public:
    type_mismatch(std::string expected, value const &found)
      : error("Invalid type: expected " + std::move(expected) + ", found " + show(found))
    {}
};

class parse_error
  : public error
{
public:
    explicit parse_error(std::string message)
      : error("Parse error at " + std::move(message))
    {}
};

class bad_special_form
  : public error
{
public:
    bad_special_form(std::string message, value const &form)
      : error(std::move(message) + ": " + show(form))
    {}
};

class not_function
  : public error
{
public:
    not_function(std::string message, std::string fn)
      : error(std::move(message) + ": " + std::move(fn))
    {}
};

class unbound_variable
  : public error
{
public:
    unbound_variable(std::string message, std::string name)
      : error(message + ": " + name)
    {}
};
}

#endif
