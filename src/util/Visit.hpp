#pragma once

namespace sge::util {

template <class... Ts>
struct Visitor : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
Visitor(Ts...) -> Visitor<Ts...>;

} // namespace sge::util
