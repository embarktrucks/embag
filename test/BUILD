cc_test(
    name = "embag_test",
    srcs = ["embag_test.cc"],
    copts = ["-Iexternal/gtest/include"],
    data = ["test.bag", "test_2.bag"],
    deps = [
        "//lib:embag",
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

load("@test_python_requirements//:requirements.bzl", "requirement")

py_test(
    name = "embag_test_python3",
    main = "embag_test_python.py",
    srcs = ["embag_test_python.py"],
    data = [
        "test.bag",
        "//python:libembag.so",
    ],
    deps = [
        requirement("numpy"),
    ],
    python_version = "PY3",
)

py_test(
    name = "embag_test_python2",
    main = "embag_test_python.py",
    srcs = ["embag_test_python.py"],
    data = [
        "test.bag",
        "//python:libembag.so",
    ],
    deps = [
        requirement("numpy"),
    ],
    python_version = "PY2",
)
