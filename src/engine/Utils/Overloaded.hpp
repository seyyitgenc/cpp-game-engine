#pragma once

namespace GNC {

// One visitor out of a lambda per alternative, for std::visit. Unlike a
// generic lambda, a variant alternative with no matching lambda fails to
// compile.
template<typename... Lambdas>
struct Overloaded : Lambdas... {
    using Lambdas::operator()...;
};

} // namespace GNC
