constexpr auto check_result_void() {
  struct error_t {};
  struct error2_t {};

  // map
  auto map0 = (result<void, error_t> {}).map([]() {});
  static_assert(
    std::is_same_v<decltype(map0), result<void, error_t>>,
    "result<void, ...>.map()"
  );

  auto map1 = (result<void, error_t> {}).map([]() {
    return 1;
  });
  static_assert(
    std::is_same_v<decltype(map1), result<int, error_t>>,
    "result<void, ...>.map()"
  );

  auto map2 = (result<void, error_t> {}).map([]() {
    return result<int, error_t>{1};
  });
  static_assert(
    std::is_same_v<decltype(map2), result<result<int, error_t>, error_t>>,
    "result<void, ...>.map()"
  );

  // flat_map
  auto flat_map0 = (result<void, error_t> {}).flat_map([]() {
    return result<void, error_t>{};
  });
  static_assert(
    std::is_same_v<decltype(flat_map0), result<void, error_t>>,
    "result<void, ...>.flat_map()"
  );

  auto flat_map1 = (result<void, error_t> {}).flat_map([]() {
    return result<int, error_t>{1};
  });
  static_assert(
    std::is_same_v<decltype(flat_map1), result<int, error_t>>,
    "result<void, ...>.flat_map()"
  );

  // match
  auto match0 = (result<void, error_t> {}).match(
    []() { return 1; },
    [](const error_t&) { return 1; }
  );
  static_assert(
    std::is_same_v<decltype(match0), int>,
    "result<void, ...>.match()"
  );

  auto match1 = (result<void, error_t> {}).match(
    []() { return result<int, error2_t>{1}; },
    [](const error_t&) { return result<int, error2_t>{1}; }
  );
  static_assert(
    std::is_same_v<decltype(match1), result<int, error2_t>>,
    "result<void, ...>.match()"
  );

  static_assert(
    std::is_void_v<decltype(
      (result<void, error_t> {}).match(
        []() {},
        [](const error_t&) {}
      )
    )>,
    "result<void, ...>.match()"
  );

  // visit
  auto visit0 = visit(
    []() {},
    result<void, error_t> {},
    result<void, error_t> {}
  );
  static_assert(
    std::is_same_v<decltype(visit0), result<void, error_t>>,
    "visit() doesn't change return type"
  );

  auto visit1 = visit(
    []() { return 1; },
    result<void, error_t> {},
    result<void, error_t> {}
  );
  static_assert(
    std::is_same_v<decltype(visit1), result<int, error_t>>,
    "visit() change return type"
  );
}

constexpr auto check_result_typed() {
  struct error_t {};
  struct error2_t {};

  // map
  auto map0 = (result<int, error_t> {1}).map([](int) {
    return 1;
  });
  static_assert(
    std::is_same_v<decltype(map0),
    result<int, error_t>>,
    "result<int, ...>.map()"
  );

  auto map1 = (result<int, error_t> {1}).map([](int) {
  });
  static_assert(
    std::is_same_v<decltype(map1),
    result<void, error_t>>,
    "result<int, ...>.map()"
  );

  auto map2 = (result<int, error_t> {1}).map([](int) {
    return result<int, error_t>{1};
  });
  static_assert(
    std::is_same_v<decltype(map2),
    result<result<int, error_t>, error_t>>,
    "result<int, ...>.map()"
  );

  // flat_map
  auto flat_map0 = (result<int, error_t> {1}).flat_map([](int) {
    return result<int, error_t>{1};
  });
  static_assert(
    std::is_same_v<decltype(flat_map0), result<int, error_t>>,
    "result<int, ...>.flat_map()"
  );

  auto flat_map1 = (result<int, error_t> {1}).flat_map([](int) {
    return result<void, error_t>{};
  });
  static_assert(
    std::is_same_v<decltype(flat_map1),
    result<void, error_t>>,
    "result<int, ...>.flat_map()"
  );

  // match
  auto match0 = (result<int, error_t> {1}).match(
    [](int) { return 1; },
    [](const error_t&) { return 1; }
  );
  static_assert(
    std::is_same_v<decltype(match0), int>,
    "result<int, ...>.match()"
  );

  auto match1 = (result<int, error_t> {1}).match(
    [](int) { return result<int, error2_t>{1}; },
    [](const error_t&) { return result<int, error2_t>{1}; }
  );
  static_assert(
    std::is_same_v<decltype(match1), result<int, error2_t>>,
    "result<int, ...>.match()"
  );

  static_assert(
    std::is_void_v<decltype(
      (result<int, error_t> {1}).match(
        [](int) {},
        [](const error_t&) {}
      )
    )>,
    "result<int, ...>.match()"
  );

  // visit
  auto visit0 = visit(
    [](int, int) { return 1; },
    result<int, error_t> {1},
    result<int, error_t> {1}
  );
  static_assert(
    std::is_same_v<decltype(visit0), result<int, error_t>>,
    "visit() doesn't change return type"
  );

  auto visit1 = visit(
    [](int, int) {},
    result<int, error_t> {1},
    result<int, error_t> {1}
  );
  static_assert(
    std::is_same_v<decltype(visit1), result<void, error_t>>,
    "visit() change return type"
  );
}

