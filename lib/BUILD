load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "embag",
    srcs = [
        "bag_view.cc",
        "embag.cc",
        "message_parser.cc",
        "ros_value.cc",
    ],
    hdrs = [
        "bag_vew.h",
        "embag.h",
        "message_parser.h",
        "ros_bag_types.h",
        "ros_message.h",
        "ros_value.h",
        "util.h",
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

cc_binary(
    name = "embag.so",
    linkshared = True,
    deps = [
        "//lib:embag",
    ],
)
