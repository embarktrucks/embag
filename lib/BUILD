load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "embag",
    srcs = ["embag.cc"],
    hdrs = ["embag.h"],
    visibility = ["//embag_echo:__pkg__"],
    deps = [
        "@boost//:iostreams",
        "@boost//:spirit",
        "@boost//:fusion",
        "@boost//:variant",
        "@system_libs//:liblz4",
    ],
)

