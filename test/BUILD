cc_test(
    name = "embag-test",
    srcs = ["embag-test.cc"],
    copts = ["-Iexternal/gtest/include"],
    data = [
        "test.bag",
    ],
    deps = [
        "//lib:embag",
        "@gtest",
        "@gtest//:gtest_main",  # Only if hello_test.cc has no main()
    ],
)
