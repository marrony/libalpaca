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

template<T> auto fn(T&& x) {
  // universal (forward) reference

  // perfectly forward
  g(std::forward<T>(x));
}

template<T> auto fn(T&&... x) {
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

template<typename... Err>
using err_t = std::variant<Err...>;

template<typename Err>
auto unexpected(Err err) -> Err {
  return err;
}

template<typename Tp, typename Err>
using ret_t = std::variant<Tp, Err>;

template<typename... Ts>
struct flatten_t;

namespace __detail {
  template<typename T>
  constexpr bool is_ret_t = false;

  template<typename Tp, typename Err>
  constexpr bool is_ret_t<ret_t<Tp, Err>> = true;

  template<typename T>
  struct ret_t_helper;

  template<typename Tp, typename Err>
  struct ret_t_helper<ret_t<Tp, Err>> {
    using value_type = Tp;
    using error_type = Err;
  };
}

template<typename Tp, typename Err, typename... Ts>
struct flatten_t<ret_t<Tp, Err>, Ts...> {
  using tail_type = flatten_t<Ts...>;
  using return_type = typename tail_type::return_type;
  using head_type = ret_t<Tp, Err>;

  constexpr static auto flatten(head_type&& head, Ts&&... tail) -> return_type {
    if (head.index() == 0) {
      return tail_type::flatten(std::forward<Ts>(tail)...);
    } else {
      return std::get<Err>(std::move(head));
    }
  }
};

template<typename F, typename... Ts>
requires std::invocable<F>
struct flatten_t<F, Ts...> {
  using func_return_type = decltype(std::declval<F>()());
  static_assert(__detail::is_ret_t<func_return_type>,
    "F must return a ret_t type.");

  using Tp  = typename __detail::ret_t_helper<func_return_type>::value_type;
  using Err = typename __detail::ret_t_helper<func_return_type>::error_type;

  using tail_type = flatten_t<Ts...>;
  using return_type = typename tail_type::return_type;

  constexpr static auto flatten(F head, Ts&&... tail) -> return_type {
    auto&& h = std::invoke(head);

    if (h.index() == 0) {
      return tail_type::flatten(std::forward<Ts>(tail)...);
    } else {
      return std::get<Err>(std::move(h));
    }
  }
};

template<typename Tp, typename Err>
struct flatten_t<ret_t<Tp, Err>> {
  using return_type = ret_t<Tp, Err>;
  using head_type = ret_t<Tp, Err>;

  constexpr static auto flatten(head_type&& head) -> return_type {
    return std::move(head);
  }
};

template<typename F>
requires std::invocable<F>
struct flatten_t<F> {
  using func_return_type = decltype(std::declval<F>()());
  static_assert(__detail::is_ret_t<func_return_type>,
    "F must return a ret_t type.");

  using return_type = func_return_type;
  using Tp  = typename __detail::ret_t_helper<func_return_type>::value_type;
  using Err = typename __detail::ret_t_helper<func_return_type>::error_type;

  constexpr static auto flatten(F head) -> return_type {
    auto&& h = std::invoke(head);

    if (h.index() == 0) {
      return std::get<Tp>(std::move(h));
    } else {
      return std::get<Err>(std::move(h));
    }
  }
};

template<typename... Ts>
constexpr auto flatten(Ts&&... ts) -> typename flatten_t<Ts...>::return_type {
  return flatten_t<Ts...>::flatten(std::forward<Ts>(ts)...);
}

#if 0
template<typename T>
struct function_traits : function_traits<decltype(&std::remove_reference_t<T>::operator())> {
};

template<typename R, typename... Args>
struct function_traits<R(*)(Args...)> {
  using return_type = R;
};

template<typename T, typename R, typename... Args>
struct function_traits<R(T::*)(Args...) const> {
  using return_type = R;
};
#endif

#endif  // CPP_UTIL
