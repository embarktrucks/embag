cc_test(
    name = "embag_test",
    srcs = ["embag_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    data = ["test.bag"],
    deps = [
        "//lib:embag",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

py_test(
    name = "embag_test_python",
    srcs = ["embag_test_python.py"],
    data = [
        "test.bag",
        "//python:libembag.so",
    ],
)
