#include <iostream>
#include <optional>
#include <expected>
#include <functional>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <memory>
#include <chrono>
#include <signal.h>

using namespace std::literals;

namespace monad {
    
template<class T> 
inline std::optional<T> make_optional(bool condition, T const& v) {
    if(condition) return v;
    return std::nullopt;
}

template<class F> 
inline std::optional<std::invoke_result_t<F>> make_optional(bool condition, F&& f) {
    if(condition) return f();
    return std::nullopt;
}

template<class F> 
inline std::optional<std::invoke_result_t<F>> try_make_optional(F&& f) {
    try { return f(); }
    catch(...) { return std::nullopt; }
}

template<class T, class E> 
inline std::expected<T,E> make_expected(bool condition, T const& v, E e) {
    if(condition) return v;
    return std::unexpected(e);
}

template<class F, class E> 
inline std::expected<std::invoke_result_t<F>,E> make_expected(bool condition, F&& f, E e) {
    if(condition) return f();
    return std::unexpected(e);
}

template<class F, class E> 
inline std::expected<std::invoke_result_t<F>,E> try_make_expected(F&& f, E e) {
    try { return f(); }
    catch(...) { return std::unexpected(e); }
}

template<class T>
struct log_t {
    std::optional<T> value;
    std::string msg;
};

template<class T>
struct exceptional_t {
    exceptional_t() = default;
    exceptional_t(T value, std::string e = "") 
        : value(value), e(e) {}
    exceptional_t(std::optional<T> value, std::string e = "") 
        : value(std::move(value)), e(e) {}
    std::optional<T> value;
    std::string e;
};

auto add1 = [](int a) -> std::optional<int> 
{ return a + 1; };

auto div2 = [](int a) -> std::optional<int> 
{ return a / 2; };

auto div0 = [](int a) -> std::optional<int> 
{ return std::nullopt; };

auto tadd1 = [](int a) -> std::expected<int, std::string> 
{ return a + 1; };

auto tdiv2 = [](int a) -> std::expected<int, std::string> 
{ return a / 2; };

auto tdiv0 = [](int a) -> std::expected<int, std::string> 
{ return std::unexpected("Can't divide by zero."); };

/////////////////////

template<class M>
struct div_;

template<class T>
struct div_<std::optional<T>> {
    T b;
    std::optional<T> operator()(T a) { 
        if(b == T{}) return std::nullopt;
        return a / b;
    }
};

template<class T, class E>
struct div_<std::expected<T,E>> {
    T b;
    std::expected<T,E> operator()(T a) { 
        if(b == T{}) return std::unexpected("Can't divide by zero.");
        return a / b;
    }
};

template<class T>
struct div_<log_t<T>> {
    T b;
    log_t<T> operator()(T a) { 
        std::string msg = "* Dividing " + std::to_string(a) + " by " + std::to_string(b) + ".\n"; 
        if(b == T{}) return {std::nullopt, msg + "!! Can't divide by zero.\n\n"};
        if(b < T{}) msg += "W Negative denominator.\n";
        return {a / b, std::move(msg)};
    }
};

template<class T>
struct div_<exceptional_t<T>> {
    T b;
    exceptional_t<T> operator()(T a) { 
        if(b == T{}) throw std::runtime_error("Division by zero");
        return {a / b};
    }
};

template<class T>
struct divM_
{ 
    T b;
    
    template<class M>
    M operator<<=(M m) {
        return bnd(div_<M>{b}, m); 
    }
};

template<class Container, class M>
struct at_;

template<class Container>
struct at_<Container, std::optional<typename Container::value_type>> {
    Container& c;
    std::optional<typename Container::value_type> operator()(auto i) 
    {
        if(i>=c.size()) return std::nullopt;
        return {c.at(i)}; 
    }
};

template<class Container, class E>
struct at_<Container, std::expected<typename Container::value_type, E>> {
    Container& c;
    std::expected<typename Container::value_type, E> operator()(auto i) 
    { 
        if(i>=c.size()) return std::unexpected("Invalid index " + std::to_string(i));
        return {c.at(i)}; 
    }
};

template<class Container>
struct at_<Container, log_t<typename Container::value_type>> {
    Container& c;
    log_t<typename Container::value_type> operator()(auto i) 
    {
        if(i>=c.size()) return {std::nullopt, "!! Invalid index " + std::to_string(i) + ".\n\n"};
        return {c.at(i)};
    }
};

template<class Container>
struct at_<Container, exceptional_t<typename Container::value_type>> {
    Container& c;
    exceptional_t<typename Container::value_type> operator()(auto i)
    { return {c.at(i)}; }
};

template<class Container>
struct atM_
{ 
    Container& c;
    
    template<class M>
    auto operator<<=(M m) {
        return bnd(at_<Container,M>{c}, m);
    }
};


//////////////////////

template<class M, class T>
M rtrn(T a)
{ return M(a); }


template<class M>
struct zero_t;

template<class T>
struct zero_t<std::optional<T>>
{
    std::optional<T> operator()()
    { return std::nullopt; }    
};

template<class T, class E>
struct zero_t<std::expected<T,E>>
{
    std::expected<T,E> operator()()
    { return std::unexpected(E{}); } 
    
    std::expected<T,E> operator()(E e)
    { return std::unexpected(std::move(e)); } 
};

template<class T>
struct zero_t<log_t<T>>
{
    log_t<T> operator()()
    { return {std::nullopt, "No LOG\n\n"}; }    
};

template<class T>
struct zero_t<exceptional_t<T>>
{
    exceptional_t<T> operator()()
    { return exceptional_t<T>(std::nullopt, std::string()); }    
};

template<class M, class... Ts>
M zero(Ts... ts)
{ return zero_t<M>{}(ts...); }

template<class T, class F>
auto bnd(F&& f, std::optional<T> m)
{ return m.and_then(std::forward<F>(f)); }

template<class T, class E, class F>
auto bnd(F&& f, std::expected<T,E> m)
{ return m.and_then(std::forward<F>(f)); }

template<class T, class F>
auto bnd(F&& f, log_t<T> m)
{
    std::invoke_result_t<F, T> fm;
    
    if(m.value) {
        fm = f(*m.value);
        fm.msg = m.msg +  fm.msg;//(fm.msg!="" ? ('\n' + fm.msg) : "");
        return fm;
    }
    return fm; 
}

template<class T>
struct is_throwable : std::false_type {};

template<class T>
struct is_throwable<exceptional_t<T>> : std::true_type {};

template<class T, class F>
auto bnd(F&& f, exceptional_t<T> m) requires (!std::is_void_v<std::invoke_result_t<F, T>>)
{
    using result_t = 
        std::conditional_t<
            is_throwable<std::invoke_result_t<F, T>>::value,
            std::invoke_result_t<F, T>,
            exceptional_t<std::invoke_result_t<F, T>>>;
        
    // if constexpr(std::is_void_v<std::invoke_result_t<F, T>>)
    result_t r;
    
    if(!m.value) {
        r.e=m.e;
        return r;
    }
        
    // if constexpr(std::is_nothrow_invocable_v<F,T>) {
    //     if constexpr(is_throwable<std::invoke_result_t<F, T>>::value)
    //         return f(*m.value);
    //     else 
    //         return exceptional_t(f(*m.value));
    // }
        
    try {
        if constexpr(is_throwable<std::invoke_result_t<F, T>>::value)
            return f(*m.value);
        else { 
            std::cout << "YO " << *m.value << std::endl;
            return exceptional_t(f(*m.value));
        }
    } 
    catch(const std::out_of_range& e) { r.e = "std::out_of_range "s + e.what(); }
    catch(const std::invalid_argument& e) { r.e = "std::invalid_argument "s + e.what(); }
    catch(const std::logic_error& e) { std::cout << "YOtoto" << std::endl; r.e = "std::logic_error "s + e.what(); }
    catch(const std::runtime_error& e) { r.e = "std::runtime_error "s + e.what(); }
    catch(const std::bad_optional_access& e) { r.e = "std::bad_optional_access "s + e.what(); }
    catch(const std::exception& e) { r.e = "std::exception "s + e.what(); }
    catch(...) { r.e = "exception"; }
    
    
    return r;
}

template<class T>
void io_print(std::optional<T> m) {
    if(!m) std::cout << "Nothing" << std::endl;
    else std::cout << std::to_string(*m) << std::endl;
}

template<class T, class E>
void io_print(std::expected<T,E> m) {
    if(!m.has_value()) std::cout << m.error() << std::endl;
    else std::cout << m.value() << std::endl;
} 

template<class T>
void io_print(log_t<T> m) {
    std::cout << "[LOG]: " << std::endl;
    std::cout << m.msg;// << std::endl;
    if(m.value)
        std::cout << "= " << std::to_string(*m.value) << std::endl << std::endl;
}

template<class T>
void io_print(exceptional_t<T> m) {
    if(m.value)
        std::cout << std::to_string(*m.value) << std::endl;
    else
        std::cout << m.e << std::endl;
}

void io_print(std::string m) 
{ std::cout << m << std::endl; }   


template<class F>
struct M_
{ 
    F f;
    
    template<class M>
    auto operator<<=(M m)
    { return bnd(f, m); }
};

}