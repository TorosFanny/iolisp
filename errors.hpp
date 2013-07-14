#ifndef IOLISP_ERRORS_HPP
#define IOLISP_ERRORS_HPP

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
    explicit error(std::string const &msg)
      : std::runtime_error(msg)
    {}
};

class wrong_number_of_arguments
  : public error
{
public:
    template <class FoundArgs>
    wrong_number_of_arguments(int expected, FoundArgs const &found)
      : error(build_message(expected, found))
    {}

private:
    template <class FoundArgs>
    static std::string build_message(int expected, FoundArgs const &found)
    {
        std::ostringstream oss;
        oss << "Expected " << expected << " args; found values";
        for (auto const &v : found)
            oss << ' ' << v;
        return oss.str();
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
    explicit parse_error(std::string msg)
      : error("Parse error at " + std::move(msg))
    {}
};

class bad_special_form
  : public error
{
public:
    bad_special_form(std::string msg, value const &form)
      : error(std::move(msg) + ": " + show(form))
    {}
};

class not_function
  : public error
{
public:
    not_function(std::string msg, std::string func)
      : error(std::move(msg) + ": " + std::move(func))
    {}
};

class unbound_variable
  : public error
{
public:
    unbound_variable(std::string msg, std::string name)
      : error(msg + ": " + name)
    {}
};
}

#endif
