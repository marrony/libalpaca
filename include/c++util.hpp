#ifndef CPP_UTIL
#define CPP_UTIL

/*
Move semantics

auto fn(const T& x) { // const lvalue reference
}

auto fn(T& x) { // non-const lvalue reference
}

auto fn(Type&& x) {
  // rvalue reference, only support move semmantics
  // in the body of function x is treated as lvalue reference

  // perfectly forward
  g(std::move(x));
}

template<T>
auto fn(T&& x) {
  // universal (forward) reference

  // perfectly forward
  g(std::forward<T>(x));
}

template<T>
auto fn(T&&... x) {
  // universal (forward) reference
  // x can be const, non-const, rvalue & lvalue

  // perfectly forward
  g(std::forward<T>(x)...);
}

auto&& = universal reference (not template)
* can refer to lvalue and rvalues.
* keep constness

struct S {
  auto f() &  { std::cout << "lvalue\n"; }
  auto f() && { std::cout << "rvalue\n"; }
};

S s;
s.f();            // prints "lvalue"
std::move(s).f(); // prints "rvalue"
S().f();          // prints "rvalue"

*/

template<class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template<typename Tp, typename Err>
class result;

namespace __detail {
  struct __no_type {};

  template<typename T>
  struct result_traits {
    using type = __no_type;
    using error = __no_type;

    constexpr static bool is_result = false;
  };

  template<typename Tp, typename Err>
  struct result_traits<result<Tp, Err>> {
    using type = Tp;
    using error = Err;

    constexpr static bool is_result = true;
  };

  template<typename T>
  using type_t = typename result_traits<std::remove_cvref_t<T>>::type;

  template<typename T>
  using error_t = typename result_traits<std::remove_cvref_t<T>>::error;

  template<typename T>
  constexpr bool is_result_v = result_traits<std::remove_cvref_t<T>>::is_result;

  template<typename T, typename U>
  using type_or_else = std::conditional_t<is_result_v<T>, type_t<T>, U>;

  template<typename T, typename U>
  using error_or_else = std::conditional_t<is_result_v<T>, error_t<T>, U>;

  template<typename Fn, typename... Ts>
  using fn_result = std::remove_cvref_t<std::invoke_result_t<Fn&&, Ts...>>;

  template<typename Head, typename... Tail>
  struct _head_t {
    using type = Head;
  };

  template<typename... Ts>
  using head_t = typename _head_t<std::remove_cvref_t<Ts>...>::type;

  template<typename... Ts>
  concept all_results = (is_result_v<Ts> || ...);

  template<typename... Ts>
  concept all_voids = std::conjunction_v<
    std::is_void<type_t<Ts>>...
  >;

  template<typename Err, typename Head, typename... Tail>
  constexpr auto first_error(Head&& head, Tail&&... tail) -> Err {
    if constexpr (sizeof...(Tail) == 0)
      return std::forward<Head>(head).error();
    else {
      if (head.is_error())
        return std::forward<Head>(head).error();

      return first_error<Err>(
        std::forward<Tail>(tail)...
      );
    }
  }
}

template<typename Tp, typename Err>
class result {

  union {
    Tp  _value;
    Err _error;
  };

  bool _has_value;

  template<typename, typename>
  friend class result;
public:
  constexpr result() noexcept = delete;

  constexpr result(const result& other) noexcept
  : _has_value{other._has_value}
  {
    if (_has_value)
      std::construct_at(&_value, other._value);
    else
      std::construct_at(&_error, other._error);
  }

  template<typename V>
  requires std::convertible_to<V, Tp>
  constexpr result(result<V, Err>&& other) noexcept
  : _has_value(other._has_value)
  {
    if (_has_value)
      std::construct_at(&_value, std::move(other)._value);
    else
      std::construct_at(&_error, std::move(other)._error);
  }

  template<typename V>
  requires std::convertible_to<V, Tp>
  constexpr result(const V& v) noexcept
  : _value{v}
  , _has_value{true}
  { }

  template<typename V>
  requires std::convertible_to<V, Tp>
  constexpr result(V&& v) noexcept
  : _value{std::forward<V>(v)}
  , _has_value{true}
  { }

  constexpr result(const Err& v) noexcept
  : _error{v}
  , _has_value{false}
  {
    //std::construct_at(&_error, v);
  }

  constexpr result(Err&& v) noexcept
  : _error{std::forward<Err>(v)}
  , _has_value{false}
  {
    //std::construct_at(&_error, std::forward<Err>(v));
  }

  constexpr ~result() {
    if (_has_value)
      std::destroy_at(&_value);
    else
      std::destroy_at(&_error);
  }

  template<typename... Fn>
  constexpr auto match(Fn&& ...fn) const {
    auto ov = overloaded{std::forward<Fn>(fn)...};

    if (_has_value)
      return ov(_value);

    return ov(_error);
  }

  template<typename Fn>
  constexpr auto map(Fn&& fn) const {
    using U = __detail::fn_result<Fn, Tp>;
    using Ret = result<U, Err>;

    if (_has_value) {
      if constexpr (std::is_void_v<U>) {
        std::invoke(
          std::forward<Fn>(fn),
          _value
        );

        return Ret{};
      } else {
        return Ret{
          std::invoke(
            std::forward<Fn>(fn),
            _value
          )
        };
      }
    } else {
      return Ret{_error};
    }
  }

  template<typename Fn>
  constexpr auto flat_map(Fn&& fn) const {
    using Ret = __detail::fn_result<Fn, Tp>;

    static_assert(__detail::is_result_v<Ret>, "callable must return a result<Tp, Err>");
    static_assert(std::is_same_v<Err, __detail::error_t<Ret>>, "error types must match");

    if (_has_value) {
      return std::invoke(
        std::forward<Fn>(fn),
        _value
      );
    } else {
      return Ret{_error};
    }
  }

  constexpr Tp& get() & {
    return _value;
  }

  constexpr const Tp& get() const & {
    return _value;
  }

  constexpr Tp&& get() && {
    return std::move(_value);
  }

  constexpr const Tp&& get() const && {
    return std::move(_value);
  }

  constexpr Err& error() & {
    return _error;
  }

  constexpr const Err& error() const & {
    return _error;
  }

  constexpr Err&& error() && {
    return std::move(_error);
  }

  constexpr const Err&& error() const && {
    return std::move(_error);
  }

  constexpr auto is_error() const {
    return !_has_value;
  }
};

template<typename Err>
class result<void, Err> {
  std::optional<Err> _error;
public:
  constexpr result(result&& v) noexcept
  : _error{std::move(v._error)}
  { }

  constexpr result(const result& v) noexcept
  : _error{v._error}
  { }

  constexpr result() noexcept
  : _error{}
  { }

  constexpr result(Err&& err) noexcept
  : _error{std::forward<Err>(err)}
  { }

  constexpr result(const Err& err) noexcept
  : _error{err}
  { }

  template<typename... Fn>
  constexpr auto match(Fn&& ...fn) const {
    auto ov = overloaded{std::forward<Fn>(fn)...};

    if (_error)
      return std::invoke(ov, *_error);

    return std::invoke(ov);
  }

  template<typename Fn>
  constexpr auto map(Fn&& fn) const {
    using U = __detail::fn_result<Fn>;
    using Ret = result<U, Err>;

    if (!_error) {
      if constexpr (std::is_void_v<U>) {
        std::invoke(std::forward<Fn>(fn));
        return Ret{};
      } else {
        return Ret{
          std::invoke(std::forward<Fn>(fn))
        };
      }
    } else {
      return Ret{std::move(_error.value())};
    }
  }

  template<typename Fn>
  constexpr auto flat_map(Fn&& fn) const{
    using Ret = __detail::fn_result<Fn>;

    static_assert(__detail::is_result_v<Ret>, "callable must return a result<Tp, Err>");
    static_assert(std::is_same_v<Err, __detail::error_t<Ret>>, "error types must match");

    if (!_error) {
      return std::invoke(std::forward<Fn>(fn));
    } else {
      return Ret{*_error};
    }
  }

  constexpr void get() const { }

  constexpr Err& error() & {
    return *_error;
  }

  constexpr Err&& error() && {
    return std::move(*_error);
  }

  constexpr auto is_error() const {
    return _error.has_value();
  }
};

template<typename Fn, typename... Ts>
requires __detail::all_results<Ts...> && (__detail::all_voids<Ts...>)
constexpr auto visit(Fn&& fn, Ts&&... ts) {
  using U = __detail::fn_result<Fn>;
  using Tp = __detail::type_or_else<U, U>;
  using Err = __detail::error_or_else<U, __detail::error_t<__detail::head_t<Ts...>>>;

  using Ret = result<Tp, Err>;

  if ((ts.is_error() || ...)) {
    return Ret{
      __detail::first_error<Err>(std::forward<Ts>(ts)...)
    };
  }

  if constexpr (std::is_void_v<Tp>) {
    std::invoke(std::forward<Fn>(fn));
    return Ret{};
  } else {
    return Ret{
      std::invoke(std::forward<Fn>(fn))
    };
  }
}

template<typename Fn, typename... Ts>
requires __detail::all_results<Ts...> && (!__detail::all_voids<Ts...>)
constexpr auto visit(Fn&& fn, Ts&&... ts) {
  using U = __detail::fn_result<Fn, __detail::type_t<Ts>...>;
  using Tp = __detail::type_or_else<U, U>;
  using Err = __detail::error_or_else<U, __detail::error_t<__detail::head_t<Ts...>>>;

  using Ret = result<Tp, Err>;

  if ((ts.is_error() || ...)) {
    return Ret{
      __detail::first_error<Err>(std::forward<Ts>(ts)...)
    };
  }

  if constexpr (std::is_void_v<Tp>) {
    std::invoke(
      std::forward<Fn>(fn),
      std::forward<__detail::type_t<Ts>>(ts.get())...
    );
    return Ret{};
  } else {
    return Ret{
      std::invoke(
        std::forward<Fn>(fn),
        std::forward<__detail::type_t<Ts>>(ts.get())...
      )
    };
  }
}

template <typename C, typename T>
struct rebind {
  static_assert(not std::is_same_v<C, C>, "C must match C<T, ...>");
};

template<
  template<typename, typename...> typename C,
  typename T,
  typename U
>
struct rebind<C<T>, U> {
  using type = C<U>;
};

template<typename C, typename T>
using rebind_t = typename rebind<C, T>::type;

template<typename C, typename Fn>
constexpr auto flatten(C&& container, Fn&& fn) {
  using Cont = std::remove_cvref_t<C>;
  using T = typename Cont::value_type;

  using FnRet = __detail::fn_result<Fn, T>;
  static_assert(__detail::is_result_v<FnRet>, "Callable must return result<Tp, Err>");

  using Tp = __detail::type_t<FnRet>;
  using Err = __detail::error_t<FnRet>;
  using OutC = rebind_t<Cont, Tp>;
  using Ret = result<OutC, Err>;

  OutC ret{};
  auto inserter = std::back_inserter(ret);

  for (auto&& v : container) {
    auto&& fn_ret = std::invoke(std::move(fn), v);

    if (fn_ret.is_error()) {
      return Ret{ std::move(fn_ret).error() };
    }

    ++inserter = std::move(fn_ret).get();
  }

  return Ret{std::move(ret)};
}

//C<result<Tp, Err>> => result<C<Tp>, Err>
template<typename C>
constexpr auto flatten(C&& container) {
  using Cont = std::remove_cvref_t<C>;
  using T = typename Cont::value_type;

  static_assert(__detail::is_result_v<T>, "container");

  using Tp = __detail::type_t<T>;
  using Err = __detail::error_t<T>;
  using OutC = rebind_t<Cont, Tp>;
  using Ret = result<OutC, Err>;

  OutC ret{};
  auto inserter = std::back_inserter(ret);

  for (auto&& v : container) {
    if (v.is_error()) {
      return Ret{ std::move(v).error() };
    }

    ++inserter = std::move(v).get();
  }

  return Ret{ ret };
}

#include "c++util-asserts.hpp"

#endif  // CPP_UTIL
