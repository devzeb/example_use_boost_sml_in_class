# Example on how to use boost::sml inside a class

Boost sml is a brilliant library for state machines.
The downside is that it is not very easy to use, as it is heavily templated.

This example demonstrates:

- how to instantiate boost::sml inside a class, allowing the state machine to access private functions
  and variables
- how to use private member functions as actions or guards
- how to use member variables of type bool as guards
- how to use member variables of any type that is implicitly convertible to bool as guards

# Was this example helpful?

If you like this example and want to support me, you can do so by buying me a coffee, pizza or other developer essentials by clicking this link: [Support me with PayPal](https://www.paypal.com/donate/?hosted_button_id=TGDGATFR63N3G)

# Getting stated with this example

- clone this repository
- checkout the submodules by running \
`git submodule update --init --recursive`
- use your ide to configure and build the cmake project, or configure and build it manually by running \
`mkdir build && cd build && cmake .. && cmake --build .`