#include <functional>
#include <iostream>
#include <boost/sml.hpp>

class StateMachine
{
    friend boost::sml::sm<StateMachine>;

    // prevent anyone (except subclasses) from creating an instance of this class
protected:
    StateMachine() = default;

public:
    // rule of 5
    StateMachine(const StateMachine&) = default;
    StateMachine(StateMachine&&) = default;
    StateMachine& operator=(const StateMachine&) = default;
    StateMachine& operator=(StateMachine&&) = default;
    virtual ~StateMachine() = default;

protected:
    struct e1
    {
    };

    struct e2
    {
    };

    struct e3
    {
    };

private:
    void internalFunction() const
    {
        std::cout << "internalFunction, internalState = " << internalState << '\n';
    }

    bool positiveGuard() // NOLINT(*-convert-member-functions-to-static)
    {
        return true;
    }

    bool negativeGuard() // NOLINT(*-convert-member-functions-to-static)
    {
        return false;
    }

    void onPositiveGuard() const
    {
        std::cout << "onPositiveGuard, internalState = " << internalState << '\n';
    }

    void onNegativeGuard() const
    {
        std::cout << "onNegativeGuard, internalState = " << internalState << '\n';
    }

    void onCustomGuard() const
    {
        std::cout << "onCustomGuard, internalState = " << internalState << '\n';
    }

public:
    auto operator()() noexcept
    {
        // get class name, required to specify for member function pointers
        using self = std::remove_pointer_t<decltype(this)>;

        auto member_function = [](auto&& memberFunctionPtr)
        {
            // create a lambda that calls a member function with no arguments
            return
                [memberFunctionPtr] // capture the member function
            (self& thisInstance)
                // require boost::sml to pass the instance on invocation. The instance is set by passing a reference to self to the constructor of boost::sml::sm
            {
                return std::invoke(memberFunctionPtr, thisInstance);
            };
        };

        // creates a wrapper that when invoked with a instance to self returns the value of the member variable
        // this can be used to create a guard for any type that is implicitly convertible to bool
        auto member_variable = [](
            auto self::* member_variable_ptr // pointer to a member variable of any type
        )
        {
            return [member_variable_ptr](self& thisInstance)
            {
                // return value of member variable
                return thisInstance.*member_variable_ptr;
            };
        };

        namespace sml = boost::sml;
        using namespace sml;

        const auto idle = state<class idle>;
        // clang-format off
        return make_transition_table(
            *idle + sml::on_entry<_> / member_function(&self::internalFunction)
            , idle + event<e1>[member_variable(&self::falseGuard)] / member_function(&self::onNegativeGuard) = "s1"_s
            , idle + event<e1>[member_variable(&self::customGuard)] / member_function(&self::onCustomGuard) = "s1"_s

            , idle + event<e1>[member_function(&self::positiveGuard)] / member_function(&self::onPositiveGuard) = "s2"_s
            , "s1"_s + sml::on_exit<_> / [] { std::cout << "s1 on exit" << std::endl; }
            , "s1"_s + event<e2> = "s2"_s
            , "s2"_s + event<e3> = X
        );
        // clang-format on
    }

protected:
    struct ConvertibleToBool
    {
        operator bool() const
        {
            return internalState;
        }

        bool internalState{false};
    };

    bool falseGuard = false;

    ConvertibleToBool customGuard{};

    int internalState{42};
};

class ClassWithStateMachine final : StateMachine
{
    boost::sml::sm<StateMachine> sm{
        static_cast<StateMachine&>(*this)
    };

public:
    ClassWithStateMachine() = default;

    // copies or moves must not be made as the state machine contains a reference to this instance
    // this reference would be invalid once the instance is copied or moved
    ClassWithStateMachine(const ClassWithStateMachine&) = delete;
    ClassWithStateMachine(ClassWithStateMachine&&) = delete;
    ClassWithStateMachine& operator=(const ClassWithStateMachine&) = delete;
    ClassWithStateMachine& operator=(ClassWithStateMachine&&) = delete;
    // specify destructor because of the rule of 5
    ~ClassWithStateMachine() override = default;

public:
    void onEventE1()
    {
        internalState = 1337;
        sm.process_event(e1{});
    }
};

int main()
{
    std::cout << "Hello, World!" << std::endl;

    ClassWithStateMachine classWithStateMachine{};
    classWithStateMachine.onEventE1();

    return 0;
}
