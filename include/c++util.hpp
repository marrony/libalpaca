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

template<typename Tp, typename Err>
class result;

namespace __detail {
  template<typename T>
  struct _error_t;

  template<typename Tp, typename Err>
  struct _error_t<result<Tp, Err>> {
    using type = Err;
  };

  template<typename T>
  using error_t = typename _error_t<T>::type;

  template<typename T>
  constexpr bool is_result = false;

  template<typename Tp, typename Err>
  constexpr bool is_result<result<Tp, Err>> = true;

  template<typename Head, typename... Tail>
  struct _first_t {
    using type = Head;
  };

  template<typename... Ts>
  using first_t = typename _first_t<std::remove_reference_t<Ts>...>::type;

  template<typename Err, typename Head, typename... Tail>
  constexpr auto first_error(Head&& head, Tail&&... tail) -> Err {
    if constexpr (sizeof...(Tail) == 0)
      return head.error();
    else {
      if (head.is_error())
        return head.error();

      return first_error<Err>(
        std::forward<Tail>(tail)...
      );
    }
  }

  template<typename V, typename Tp, typename Err>
  constexpr auto move_result(result<V, Err>&& v) -> std::variant<Tp, Err> {
    if (!v.is_error())
      return std::get<V>(v.value);
    else
      return std::get<Err>(v.value);
  }
}

template<typename Tp, typename Err>
class result {
  using type = std::variant<Tp, Err>;

  type value;

  template<typename, typename>
  friend class result;

  template<typename V, typename Tp1, typename Err1>
  friend constexpr auto __detail::move_result(result<V, Err1>&& v) -> std::variant<Tp1, Err1>;
public:
   constexpr result(type&& v) noexcept
  : value{std::move(v)}
  { }

  constexpr result(result&& v) noexcept
  : value{std::move(v.value)}
  { }

  constexpr result(const result& v) noexcept
  : value{v.value}
  { }

  template<typename V>
  requires std::convertible_to<V, Tp>
  constexpr result(result<V, Err>&& v) noexcept
  : value{__detail::move_result<V, Tp, Err>(std::forward<result<V, Err>>(v))}
  { }

#if 1
  template<typename V>
  requires std::convertible_to<V, Tp> || std::convertible_to<V, Err>
  constexpr result(V&& v) noexcept
  : value{std::forward<V>(v)}
  { }
#else
  template<typename V>
  requires (std::disjunction_v<std::is_same<V, Tp>, std::is_same<V, Err>>)
  constexpr result(V&& v) noexcept
  : value{std::forward<V>(v)}
  { }
#endif

  template<typename V>
  requires (std::disjunction_v<std::is_same<V, Tp>, std::is_same<V, Err>>)
  constexpr result(const V& v) noexcept
  : value{v}
  { }

  template<typename... Fn>
  constexpr auto match(Fn&& ...fn) {
    return std::visit(overloaded{fn...}, value);
  }

  template<typename Fn>
  constexpr auto map(Fn&& fn) {
    using U = decltype(fn(std::declval<Tp>()));
    using Ret = result<U, Err>;

    if (value.index() == 0) {
      if constexpr (std::is_void_v<U>) {
        std::invoke(
          std::forward<Fn>(fn),
          std::forward<Tp>(std::get<Tp>(value))
        );

        return Ret{};
      } else {
        return Ret{
          std::invoke(
            std::forward<Fn>(fn),
            std::forward<Tp>(std::get<Tp>(value))
          )
        };
      }
    } else {
      return Ret{std::get<Err>(value)};
    }
  }

  template<typename Fn>
  constexpr auto flat_map(Fn&& fn) -> decltype(fn(std::declval<Tp>())) {
    using Ret = decltype(fn(std::declval<Tp>()));

    static_assert(__detail::is_result<Ret>, "callable must return a result<Tp, Err>");
    static_assert(std::is_same_v<Err, __detail::error_t<Ret>>, "error types must match");

    if (value.index() == 0) {
      return std::invoke(
        std::forward<Fn>(fn),
        std::forward<Tp>(std::get<Tp>(value))
      );
    } else {
      return Ret{std::get<Err>(value)};
    }
  }

  constexpr Tp& get() & {
    return std::get<0>(value);
  }

  constexpr const Tp& get() const & {
    return std::get<0>(value);
  }

  constexpr Tp&& get() && {
    return std::move(std::get<0>(value));
  }

  constexpr const Tp&& get() const && {
    return std::move(std::get<0>(value));
  }

  constexpr Err& error() & {
    return std::get<1>(value);
  }

  constexpr const Err& error() const & {
    return std::get<1>(value);
  }

  constexpr Err&& error() && {
    return std::move(std::get<1>(value));
  }

  constexpr const Err&& error() const && {
    return std::move(std::get<1>(value));
  }

  constexpr auto is_error() const {
    return value.index() > 0;
  }
};

template<typename Err>
class result<void, Err> {
  std::optional<Err> _error;
public:
#if 0
   constexpr result(type&& v) noexcept
  : value{std::move(v)}
  { }
#endif

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
  : _error{std::move(err)}
  { }

  constexpr result(const Err& err) noexcept
  : _error{err}
  { }

  template<typename... Fn>
  constexpr auto match(Fn&& ...fn) {
    auto ov = overloaded{fn...};

    if (_error)
      return std::invoke(ov, *_error);

    return std::invoke(ov);
  }

  template<typename Fn>
  constexpr auto map(Fn&& fn) {
    using U = decltype(fn());
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
      return Ret{*_error};
    }
  }

  template<typename Fn>
  constexpr auto flat_map(Fn&& fn) -> decltype(fn()) {
    using Ret = decltype(fn());

    static_assert(__detail::is_result<Ret>, "callable must return a result<Tp, Err>");
    static_assert(std::is_same_v<Err, __detail::error_t<Ret>>, "error types must match");

    if (!_error) {
      return std::invoke(std::forward<Fn>(fn));
    } else {
      return Ret{*_error};
    }
  }

  constexpr void get() {
  }

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
constexpr auto visit(Fn&& fn, Ts&&... ts) {
  using Tp = decltype(fn(ts.get()...));
  using Err = __detail::error_t<__detail::first_t<Ts...>>;

  if ((ts.is_error() || ...)) {
    return result<Tp, Err>{
      __detail::first_error<Err>(std::forward<Ts>(ts)...)
    };
  }

  if constexpr (std::is_void_v<Tp>) {
    std::invoke(std::forward<Fn>(fn), ts.get()...);
    return result<Tp, Err>{};
  } else {
    return result<Tp, Err>{
      std::invoke(std::forward<Fn>(fn), ts.get()...)
    };
  }
}

#endif  // CPP_UTIL
