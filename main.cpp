#include <boost/sml.hpp>
#include <functional>
#include <iostream>

// support function for boost sml
// create a lambda that calls a member function with no arguments
// require boost::sml to pass the instance on invocation. The instance is set by passing a reference to self to the
// constructor of boost::sml::sm
template<typename T>
decltype(auto) class_member(auto T::*pointerToMember) {
    static constexpr bool is_member_function_pointer = std::is_member_function_pointer_v<decltype(pointerToMember)>;
    static constexpr bool is_member_object_pointer = std::is_member_object_pointer_v<decltype(pointerToMember)>;

    static_assert(is_member_function_pointer || is_member_object_pointer,
                  "pointerToMember must be a pointer to a member function or a pointer to a member variable");

    if constexpr (is_member_function_pointer) {
        return [pointerToMember](T& instance) {
            // invoke the member function using the instance passed in as a parameter
            return std::invoke(pointerToMember, instance);
        };
    }
    else {
        return [pointerToMember](T& instance) {
            // return value of member variable
            return instance.*pointerToMember;
        };
    }
}

// this class is supposed to be used as a base class.
// this way, the state machine transition table can be defined within the class itself, accessing its member variables
// and functions, without the need to pass them as parameters
class StateMachine {
protected:
    struct event1 {};

public:
    auto operator()() noexcept {
        namespace sml = boost::sml;
        using namespace sml;

        // get class name, required to specify for member function pointers
        using self = std::remove_pointer_t<decltype(this)>;
        // using self = StateMachine;

        // clang-format off
        return make_transition_table(

            // use private member function as action
            *"idle"_s + sml::on_entry<_>
                / class_member(&self::memberFunctionAction)

            // use private member variable as guard
            , "idle"_s + event<event1> [class_member(&self::memberVariableGuardBool) || // type bool
                                        class_member(&self::memberVariableGuard)] // arbitrary type (must be convertible to bool)

                // warning: do not capture "this" in a lambda definded here, for some reason "this", when captured, is not the instance of this class
                // / [this]() {
                    // std::cout << "member variable guard was true " << '\n';
                    // std::cout << "  someNumber = " << someNumber << '\n';
                    // std::cout << "  memberVariableGuardBool = " << memberVariableGuardBool << '\n';
                    // std::cout << "  memberVariableGuard = " << static_cast<bool>(memberVariableGuard) << '\n';
                // }
                = "s1"_s

            // use private member function as guard
            , "idle"_s + event<event1> [class_member(&self::memberFunctionGuard)]

                // warning: do not capture "this" in a lambda definded here, for some reason "this", when captured, is not the instance of this class
                // / [this] (){
                    // std::cout << "member function guard was true " << '\n';
                    // std::cout << "  someNumber = " << someNumber << '\n';
                // }
                = "s2"_s

            , "s1"_s = X
            , "s2"_s = X
        );
        // clang-format on
    }

private:
    void memberFunctionAction() const { std::cout << "internalFunction, someNumber = " << someNumber << '\n'; }

    bool memberFunctionGuard() // NOLINT(*-convert-member-functions-to-static)
    {
        return true;
    }

    void onGuardFunctionWasTrue() const { std::cout << "onPositiveGuard, someNumber = " << someNumber << '\n'; }

    void onActionMemberVariableGuard() const { std::cout << "onCustomGuard, someNumber = " << someNumber << '\n'; }

protected:
    // prevent anyone (except subclasses) from creating an instance of this class
    // this is especially important for using boost::sml, because if a dependency is missing (like e.g. a reference to
    // an object) boost sml will internally just create a new instance of the class
    StateMachine() = default;

public:
    // as we use this class as a base class, we need to make the destructor virtual
    virtual ~StateMachine() = default;

    // rule of 5, we need to specify the copy and move constructors and assignment operators because we defined a
    // destructor
    StateMachine(const StateMachine &) = default;
    StateMachine(StateMachine &&) = default;
    StateMachine &operator=(const StateMachine &) = default;
    StateMachine &operator=(StateMachine &&) = default;

protected:
    struct ConvertibleToBool {
        explicit operator bool() const { return internalState; }

        bool internalState{false};
    };

    ConvertibleToBool memberVariableGuard{};

    bool memberVariableGuardBool{false};

    int someNumber{42};
};

class ClassWithStateMachine final : StateMachine {
    boost::sml::sm<StateMachine> sm{
            // the sml::sm state machine has a dependency on a reference to a StateMachine instance
            // this is because we want to call member functions and access member variables of the StateMachine instance
            //
            // this makes the class non-copyable and non-movable, because if we would copy / move the class and the
            // sml::sm,
            // the reference to this instance would be invalid
            //
            // as we inherit from StateMachine, we can just pass a reference to this instance
            static_cast<StateMachine&>(*this)
    };

public:
    ClassWithStateMachine() = default;

    // copies or moves must not be made as the state machine contains a reference to this instance
    // this reference would be invalid once the instance is copied or moved
    ClassWithStateMachine(const ClassWithStateMachine &) = delete;
    ClassWithStateMachine(ClassWithStateMachine &&) = delete;
    ClassWithStateMachine &operator=(const ClassWithStateMachine &) = delete;
    ClassWithStateMachine &operator=(ClassWithStateMachine &&) = delete;
    // specify destructor because of the rule of 5
    ~ClassWithStateMachine() override = default;

public:
    void onEventE1() {
        // modify the internal state of the state machine base class
        someNumber = 1337;

        // try out that custom variables work as a guard by uncomment the following line
        memberVariableGuard.internalState = true;

        sm.process_event(event1{});
    }
};

int main() {
    std::cout << "Hello, World!" << std::endl;

    ClassWithStateMachine classWithStateMachine{};
    classWithStateMachine.onEventE1();

    return 0;
}
