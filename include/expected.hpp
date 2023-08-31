#ifndef INCLUDE_EXPECTED_HPP_
#define INCLUDE_EXPECTED_HPP_

namespace std {

template<typename Err>
struct unexpected {
  constexpr unexpected(const unexpected&) = default;
  constexpr unexpected(unexpected&&) = default;

  template<typename E = Err>
  constexpr explicit unexpected(E&& err) noexcept
  : _error{std::forward<E>(err)}
  { }

  constexpr unexpected& operator=(const unexpected&) = default;
  constexpr unexpected& operator=(unexpected&&) = default;

  constexpr auto error() noexcept -> Err& {
    return _error;
  }

  constexpr auto error() const noexcept -> const Err& {
    return _error;
  }
 private:
  Err _error;
};

template<typename E>
unexpected(E) -> unexpected<E>;

struct unexpect_t {
  explicit unexpect_t() = default;
};

inline constexpr unexpect_t unexpect{};

// todo(marrony): replace with std::expected on c++23
// doesn't support voids
template<typename Tp, typename Err>
struct expected {
  constexpr expected() noexcept
  : _value{}
  , _has_value{true}
  { }

  //expected(const expected&) = default;
  constexpr expected(const expected& other) noexcept
  : _has_value{other._has_value}
  {
    if (_has_value)
      std::construct_at(&_value, other._value);
    else
      std::construct_at(&_error, other._error);
  }

  template<typename U, typename G>
  constexpr expected(const expected<U, G>& other) noexcept
  : _has_value{other._has_value}
  {
    if (_has_value)
      std::construct_at(&_value, other._value);
    else
      std::construct_at(&_error, other._error);
  }

  //expected(expected&&) = default;
  constexpr expected(expected&& other) noexcept
  : _has_value{other._has_value}
  {
    if (_has_value)
      std::construct_at(&_value, std::move(other)._value);
    else
      std::construct_at(&_error, std::move(other)._error);
  }

  template<typename U, typename G>
  constexpr /*explicit*/ expected(expected<U, G>&& other) noexcept
  : _has_value{other._has_value}
  {
    if (_has_value)
      std::construct_at(&_value, std::move(other)._value);
    else
      std::construct_at(&_error, std::move(other)._error);
  }

  template<typename U = Tp>
  constexpr /*explicit*/ expected(U&& value) noexcept
  : _value{std::forward<U>(value)}
  , _has_value{true}
  { }

  template<typename G = Err>
  constexpr /*explicit*/ expected(unexpected<G>&& other) noexcept
  : _error{std::move(other).error()}
  , _has_value{false}
  { }

  template<typename... Args>
  constexpr explicit expected(unexpect_t, Args&&... args) noexcept
  : _error{std::forward<Args>(args)...}
  , _has_value(false)
  { }

#if 0
  template<typename G = Err>
  constexpr /*explicit*/ expected(G&& error) noexcept
  : _error{std::move(error)}
  , _has_value{false}
  { }
#endif

  //template<typename... _Args>
  //constexpr explicit expected(std::in_place_t, _Args&&... __args) noexcept
	//{ }

  //template<typename _Up, typename... _Args>
	//constexpr explicit expected(std::in_place_t, std::initializer_list<_Up> __il, _Args&&... __args) noexcept
  //{ }

  constexpr ~expected() {
    if (_has_value)
      std::destroy_at(&_value);
    else
      std::destroy_at(&_error);
  }

  //expected& operator=(const expected&) = delete;

#if 0
  constexpr auto operator=(const expected& value) noexcept -> expected& {
    return *this;
  }

  constexpr auto operator=(expected&& value) noexcept -> expected& {
    return *this;
  }

  template<typename U = Tp>
  constexpr auto operator=(U&&) noexcept -> expected& {
    return *this;
  }
#endif

  using value_type = Tp;
  using error_type = Err;
  using unexpected_type = unexpected<Err>;

  template<typename F>
  [[nodiscard]]
  constexpr auto and_then( F&& f ) {
    using X = decltype(f(_value));
    using U = typename X::value_type;

    if (_has_value)
      return std::invoke(std::forward<F>(f), _value);

    return std::expected<U, Err>(unexpect, _error);
  }

  [[nodiscard]]
  constexpr explicit operator bool() const noexcept {
    return _has_value;
  }

  [[nodiscard]]
  constexpr auto has_value() const noexcept -> bool {
    return _has_value;
  }

  [[nodiscard]]
  constexpr auto value() -> Tp& {
    return _value;
  }

  [[nodiscard]]
  constexpr auto value() const -> const Tp& {
    return _value;
  }

  [[nodiscard]]
  constexpr auto error() -> Err& {
    return _error;
  }

  [[nodiscard]]
  constexpr auto error() const -> const Err& {
    return _error;
  }

  template<typename F>
  constexpr auto transform(F&& f) const {
    if (_has_value) {
      return expected{f(_value)};
    } else {
      return expected{unexpected{std::move(_error)}};
    }
  }

  template<typename F>
  constexpr auto transform_error(F&& f) const {
    if (_has_value) {
      return _value;
    } else {
      return unexpected(f(_error));
    }
  }

 private:
  union {
    Tp  _value;
    Err _error;
  };

  bool _has_value;
};

}

#endif  // INCLUDE_EXPECTED_HPP_
