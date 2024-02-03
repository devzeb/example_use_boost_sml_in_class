#include <boost/sml.hpp>
#include <functional>
#include <iostream>

#include "boost_sml_helpers.h"

bool globalFunctionGuardVariable{false};
bool globalFunctionGuard() { return globalFunctionGuardVariable; }

bool globalVariableGuard{false};

// this class is supposed to be used as a base class.
// this way, the state machine transition table can be defined within the class itself, accessing its member variables
// and functions, without the need to pass them as parameters
class StateMachine {
protected:
    struct eventCheckGuards {};
    struct eventCheckGuardsAnyOrAll {};

protected:
    // prevent anyone (except subclasses) from creating an instance of this class
    // this is especially important for using boost::sml without boost::sml::dont_instantiate_statemachine_class,
    // because if a dependency is missing (like e.g. a reference to an object) boost sml will internally just create a
    // new instance of the class
    StateMachine() = default;

public:
    auto operator()() noexcept {
        // get class name, required to specify for member function pointers
        using self = std::remove_pointer_t<decltype(this)>;
        namespace sml = boost::sml;
        using namespace sml;
        using sml_helpers::guard;

        // clang-format off
        return make_transition_table(
            // demonstration of helper function guard(...) that makes anything useable as a guard

            // use lambda as guard
            *"idle"_s + event<eventCheckGuards> [guard([this]{return lambdaGuardVariable;})] / []{std::cout << "lambda guard was true" << '\n';}
            // use member variable of type bool as guard
            ,"idle"_s + event<eventCheckGuards> [guard(memberVariableBool)] / []{std::cout << "memberVariableBool was true" << '\n';}
            // use member variable of type bool as guard by pointer to class member syntax
            ,"idle"_s + event<eventCheckGuards> [guard(&self::memberVariableBool)] / []{std::cout << "memberVariableBool by pointer to member was true" << '\n';}
            // use member variable of arbitrary type as guard
            ,"idle"_s + event<eventCheckGuards> [guard(memberVariableConvertibleToBool)] / []{std::cout << "memberVariableConvertibleToBool was true" << '\n';}
            // use member variable of arbitrary type as guard by pointer to class member syntax
            ,"idle"_s + event<eventCheckGuards> [guard(&self::memberVariableConvertibleToBool)] / []{std::cout << "memberVariableConvertibleToBool by pointer to member was true" << '\n';}
            // use private member function as guard by pointer to class member syntax
            ,"idle"_s + event<eventCheckGuards> [guard(&self::memberFunctionGuard)] / []{std::cout << "memberFunctionGuard was true" << '\n';}
            // use global variable as guard
            ,"idle"_s + event<eventCheckGuards> [guard(globalVariableGuard)] / []{std::cout << "globalVariableGuard was true" << '\n';}
            // use global function as guard
            ,"idle"_s + event<eventCheckGuards> [guard(globalFunctionGuard)] / []{std::cout << "globalFunctionGuard was true" << '\n';}
            // default case if no guard was true
            ,"idle"_s + event<eventCheckGuards>  / []{ std::cout << "no guard was true" << '\n';}

            // use all of the guards above in a single transition
            ,"idle"_s + event<eventCheckGuardsAnyOrAll> [
                guard([this]{return lambdaGuardVariable;}) &&
                guard(memberVariableBool) &&
                guard(&self::memberVariableBool) &&
                guard(memberVariableConvertibleToBool) &&
                guard(&self::memberVariableConvertibleToBool) &&
                guard(&self::memberFunctionGuard) &&
                guard(globalVariableGuard) &&
                guard(globalFunctionGuard)
            ] / []{std::cout << "all guards were true" << '\n';}

            // use any of the guards above in a single transition
            ,"idle"_s + event<eventCheckGuardsAnyOrAll> [
                guard([this]{return lambdaGuardVariable;}) ||
                guard(memberVariableBool) ||
                guard(&self::memberVariableBool) ||
                guard(memberVariableConvertibleToBool) ||
                guard(&self::memberVariableConvertibleToBool) ||
                guard(&self::memberFunctionGuard) ||
                guard(globalVariableGuard) ||
                guard(globalFunctionGuard)
            ] / []{std::cout << "eventCheckAnyGuard: any guard was true" << '\n';}

            // default case if neither all or any of the guards were true
            ,"idle"_s + event<eventCheckGuardsAnyOrAll> / []{std::cout << "eventCheckAnyGuard: no guard was true" << '\n';}

            // use private member function as action
            ,"idle"_s + sml::on_entry<_> / &self::memberFunctionAction

            , "s1"_s = X
        );
        // clang-format on
    }

private:
    void memberFunctionAction() const { std::cout << "internalFunction, someNumber = " << someNumber << '\n'; }

    bool memberFunctionGuard() // NOLINT(*-convert-member-functions-to-static)
    {
        return memberFunctionGuardVariable;
    }

protected:
    struct ConvertibleToBool {
        explicit operator bool() const { return internalState; }

        bool internalState{false};
    };

    ConvertibleToBool memberVariableConvertibleToBool{};

    bool memberVariableBool{false};
    int someNumber{42};

    bool memberFunctionGuardVariable{false};
    bool lambdaGuardVariable{false};


public:
    // as we use this class as a base class, we need to make the destructor virtual
    virtual ~StateMachine() = default;

    // rule of 5, we need to specify the copy and move constructors and assignment operators because we defined a
    // destructor

    // all special member functions are = delete, because this class contains references to itself. This is because the
    // statemachine references itself by:
    // - having transitions / arguments that take a reference to an instance of `StateMachine`
    // - having lambdas in the transition table that capture `this` by reference
    // => see parameter of boost::sml::sm<...> in the subclass
    //
    // If we would copy or move the class, the reference would be invalid
    StateMachine(const StateMachine &) = delete;
    StateMachine(StateMachine &&) = delete;
    StateMachine &operator=(const StateMachine &) = delete;
    StateMachine &operator=(StateMachine &&) = delete;
};

class ClassWithStateMachine final : StateMachine {
public:
    void onEventE1() {
        std::cout << "changing internal variable someNumber to 1337" << '\n';
        // modify the internal state of the state machine base class
        someNumber = 1337;

        std::cout << '\n';

        // try out that custom variables work as a guard by uncomment one of the following lines

        globalFunctionGuardVariable = true;
        globalVariableGuard = true;
        memberVariableBool = true;
        memberFunctionGuardVariable = true;
        memberVariableConvertibleToBool.internalState = true;
        lambdaGuardVariable = true;

        // sm.process_event(eventCheckGuards{});

        sm.process_event(eventCheckGuardsAnyOrAll{});
    }

private:
    boost::sml::sm<StateMachine, boost::sml::dont_instantiate_statemachine_class> sm{
            // the sml::sm state machine has a dependency on a reference to a StateMachine instance
            // this is because we want to call member functions and access member variables of the StateMachine instance
            //
            // this makes the class non-copyable and non-movable, because if we would copy / move the class and the
            // sml::sm,
            // the reference to this instance would be invalid
            //
            // as we inherit from StateMachine, we can just pass a reference to this instance
            static_cast<StateMachine &>(*this)};
};

int main() {
    std::cout << "Hello, World!" << std::endl;

    ClassWithStateMachine classWithStateMachine{};
    classWithStateMachine.onEventE1();

    return 0;
}
