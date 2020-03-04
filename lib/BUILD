load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "embag",
    srcs = [
        "embag.cc",
        "ros_msg.cc",
    ],
    hdrs = [
        "embag.h",
        "ros_msg.h",
        "ros_value.h",
    ],
    visibility = ["//embag_echo:__pkg__"],
    deps = [
        "@boost//:fusion",
        "@boost//:iostreams",
        "@boost//:spirit",
        "@boost//:variant",
        "@system_libs//:liblz4",
    ],
)
