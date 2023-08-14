
constexpr auto check_result_void() {
  struct error_t {};
  struct error2_t {};

  // map
  static_assert(
    std::is_same_v<decltype(
      (result<void, error_t> {}).map([]() {})
    ),
    result<void, error_t>>,
    "result<void, ...>.map()"
  );

  static_assert(
    std::is_same_v<decltype(
      (result<void, error_t> {}).map([]() { return 1; })
    ),
    result<int, error_t>>,
    "result<void, ...>.map()"
  );

  static_assert(
    std::is_same_v<decltype(
      (result<void, error_t> {}).map([]() { return result<int, error_t>{1}; })
    ),
    result<result<int, error_t>, error_t>>,
    "result<void, ...>.map()"
  );

  // flat_map
  static_assert(
    std::is_same_v<decltype(
      (result<void, error_t> {}).flat_map([]() { return result<void, error_t>{}; })
    ),
    result<void, error_t>>,
    "result<void, ...>.flat_map()"
  );

  static_assert(
    std::is_same_v<decltype(
      (result<void, error_t> {}).flat_map([]() { return result<int, error_t>{1}; })
    ),
    result<int, error_t>>,
    "result<void, ...>.flat_map()"
  );

  // match
  static_assert(
    std::is_same_v<decltype(
      (result<void, error_t> {}).match(
        []() {},
        [](const error_t&) {}
      )
    ),
    void>,
    "result<void, ...>.match()"
  );

  static_assert(
    std::is_same_v<decltype(
      (result<void, error_t> {}).match(
        []() { return 1; },
        [](const error_t&) { return 1; }
      )
    ),
    int>,
    "result<void, ...>.match()"
  );

  static_assert(
    std::is_same_v<decltype(
      (result<void, error_t> {}).match(
        []() { return result<int, error2_t>{1}; },
        [](const error_t&) { return result<int, error2_t>{1}; }
      )
    ),
    result<int, error2_t>>,
    "result<void, ...>.match()"
  );

  // visit
  static_assert(
    std::is_same_v<decltype(
      visit(
        []() {},
        result<void, error_t> {},
        result<void, error_t> {}
      )
    ),
    result<void, error_t>>,
    "visit() doesn't change return type"
  );

  static_assert(
    std::is_same_v<decltype(
      visit(
        []() { return 1; },
        result<void, error_t> {},
        result<void, error_t> {}
      )
    ),
    result<int, error_t>>,
    "visit() change return type"
  );
}

constexpr auto check_result_typed() {
  struct error_t {};
  struct error2_t {};

  // map
  static_assert(
    std::is_same_v<decltype(
      (result<int, error_t> {1}).map([](int) { return 1; })
    ),
    result<int, error_t>>,
    "result<int, ...>.map()"
  );

  static_assert(
    std::is_same_v<decltype(
      (result<int, error_t> {1}).map([](int) {})
    ),
    result<void, error_t>>,
    "result<int, ...>.map()"
  );

  static_assert(
    std::is_same_v<decltype(
      (result<int, error_t> {1}).map([](int) { return result<int, error_t>{1}; })
    ),
    result<result<int, error_t>, error_t>>,
    "result<int, ...>.map()"
  );

  // flat_map
  static_assert(
    std::is_same_v<decltype(
      (result<int, error_t> {1}).flat_map([](int) { return result<int, error_t>{1}; })
    ),
    result<int, error_t>>,
    "result<int, ...>.flat_map()"
  );

  static_assert(
    std::is_same_v<decltype(
      (result<int, error_t> {1}).flat_map([](int) { return result<void, error_t>{}; })
    ),
    result<void, error_t>>,
    "result<int, ...>.flat_map()"
  );

  // match
  static_assert(
    std::is_same_v<decltype(
      (result<int, error_t> {1}).match(
        [](int) { return 1; },
        [](const error_t&) { return 1; }
      )
    ),
    int>,
    "result<int, ...>.match()"
  );

  static_assert(
    std::is_same_v<decltype(
      (result<int, error_t> {1}).match(
        [](int) {},
        [](const error_t&) {}
      )
    ),
    void>,
    "result<int, ...>.match()"
  );

  static_assert(
    std::is_same_v<decltype(
      (result<int, error_t> {1}).match(
        [](int) { return result<int, error2_t>{1}; },
        [](const error_t&) { return result<int, error2_t>{1}; }
      )
    ),
    result<int, error2_t>>,
    "result<int, ...>.match()"
  );

  // visit
  static_assert(
    std::is_same_v<decltype(
      visit(
        [](int, int) { return 1; },
        result<int, error_t> {1},
        result<int, error_t> {1}
      )
    ),
    result<int, error_t>>,
    "visit() doesn't change return type"
  );

  static_assert(
    std::is_same_v<decltype(
      visit(
        [](int, int) {},
        result<int, error_t> {1},
        result<int, error_t> {1}
      )
    ),
    result<void, error_t>>,
    "visit() change return type"
  );
}
