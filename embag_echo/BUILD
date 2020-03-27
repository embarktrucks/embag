load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "embag_echo",
    srcs = ["embag_echo.cc"],
    deps = [
        "//lib:embag",
        "@boost//:program_options",
    ],
)
