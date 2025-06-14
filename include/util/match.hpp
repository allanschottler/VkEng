#pragma once

#include <variant>

template<class... Ts>
struct overloaded : Ts...
{ using Ts::operator()...; };

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename... Args, typename... Fs>
decltype(auto) match(std::variant<Args...>& v, Fs&&... fs)
{ return std::visit(overloaded{std::forward<Fs>(fs)...}, v); }

template <typename... Args, typename... Fs>
decltype(auto) match(const std::variant<Args...>& v, Fs&&... fs)
{ return std::visit(overloaded{std::forward<Fs>(fs)...}, v); }

