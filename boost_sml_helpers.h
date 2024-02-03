#ifndef BOOST_SML_HELPERS_H
#define BOOST_SML_HELPERS_H

namespace sml_helpers {

    // boost::sml helper that makes global functions usable as guards
    template<typename TReturn>
    decltype(auto) guard(TReturn (*pointerToFunction)()) {
        return [pointerToFunction]() {
            // invoke the member function using the instance passed in as a parameter
            return std::invoke(pointerToFunction);
        };
    }

    // boost::sml helper that makes global and member variables usable as guards
    template<typename TReturn>
    decltype(auto) guard(TReturn &variable) {
        return [&variable]() { return variable; };
    }

    // boost::sml helper that makes lambdas usable as guards
    template<typename TGuard,
             // only enable this overload if TGuard is a invokeable
             typename = std::enable_if_t<std::is_invocable_v<TGuard>>>
    decltype(auto) guard(TGuard &&guardLambda) {
        return std::forward<TGuard>(guardLambda);
    }

    // boost::sml helper that makes member functions and variables with syntax &Class::member usable as guards
    template<typename TClass, typename TReturn>
    decltype(auto) guard(TReturn TClass::*pointerToMember) {
        static constexpr bool is_member_function_pointer = std::is_member_function_pointer_v<decltype(pointerToMember)>;
        static constexpr bool is_member_object_pointer = std::is_member_object_pointer_v<decltype(pointerToMember)>;

        static_assert(is_member_function_pointer || is_member_object_pointer,
                      "pointerToMember must be a pointer to a member function or a pointer to a member variable");

        if constexpr (is_member_function_pointer) {
            // Create a lambda that calls a member function with no arguments.
            // Require boost::sml to pass the instance TClass &instance on invocation.
            // This is done by passing a reference to self to the constructor of boost::sml::sm
            return [pointerToMember](TClass &instance) {
                // invoke the member function using the instance passed in as a parameter
                return std::invoke(pointerToMember, instance);
            };
        }
        else {
            // Create a lambda that returns the value of a member variable.
            // Require boost::sml to pass the instance TClass &instance on invocation.
            // This is done by passing a reference to self to the constructor of boost::sml::sm
            return [pointerToMember](TClass &instance) {
                // return value of member variable
                return instance.*pointerToMember;
            };
        }
    }

} // namespace sml_helpers

#endif // BOOST_SML_HELPERS_H
