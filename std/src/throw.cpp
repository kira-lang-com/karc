// The std's throwing helpers declared in <exception>: headers that come
// before <stdexcept> throw through them.
#include <stdexcept>
#include <optional>
#include <functional>
#include <variant>
#include <any>

void karc_throw_out_of_range(const char* what) { throw std::out_of_range(what); }
void karc_throw_invalid_argument(const char* what) { throw std::invalid_argument(what); }
void karc_throw_length_error(const char* what) { throw std::length_error(what); }
void karc_throw_bad_function_call() { throw std::bad_function_call(); }
void karc_throw_bad_optional_access() { throw std::bad_optional_access(); }
void karc_throw_bad_variant_access() { throw std::bad_variant_access(); }
void karc_throw_bad_any_cast() { throw std::bad_any_cast(); }

#include <typeinfo>

// A failed dynamic_cast to a reference.
extern "C" void karc_throw_bad_cast_c() { throw std::bad_cast(); }
