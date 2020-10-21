load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")
load("//lib:version.bzl", "EMBAG_VERSION")

cc_library(
    name = "embag",
    srcs = [
        "embag.cc",
        "message_parser.cc",
        "ros_value.cc",
        "view.cc",
    ],
    hdrs = [
        "decompression.h",
        "embag.h",
        "message_parser.h",
        "ros_bag_types.h",
        "ros_message.h",
        "ros_msg_types.h",
        "ros_value.h",
        "span.hpp",
        "util.h",
        "view.h",
    ],
    # This is required to build in the manylinux image
    linkopts = [
        "-lstdc++",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "@boost//:fusion",
        "@boost//:iostreams",
        "@boost//:spirit",
        "@boost//:variant",
        "@libbz2//:bz2",
        "@liblz4//:lz4_frame",
    ],
)

load("@rules_pkg//:pkg.bzl", "pkg_deb", "pkg_tar")

pkg_tar(
    name = "embag-lib",
    srcs = ["//lib:embag"],
    mode = "0755",
    package_dir = "/usr/lib/x86_64-linux-gnu",
    strip_prefix = "/lib",
)

pkg_tar(
    name = "embag-headers",
    srcs = [
        "embag.h",
        "message_parser.h",
        "ros_bag_types.h",
        "ros_message.h",
        "ros_msg_types.h",
        "ros_value.h",
        "span.hpp",
        "util.h",
        "view.h",
    ],
    mode = "0644",
    package_dir = "/usr/include/embag",
    strip_prefix = "/lib",
)

pkg_tar(
    name = "embag-pkgconfig",
    srcs = ["debian/libembag.pc"],
    mode = "0644",
    package_dir = "/usr/lib/x86_64-linux-gnu/pkgconfig",
    strip_prefix = "/lib/debian",
)

pkg_tar(
    name = "debian-data",
    extension = "tar.gz",
    deps = [
        ":embag-headers",
        ":embag-lib",
        ":embag-pkgconfig",
    ],
)

pkg_deb(
    name = "embag-debian",
    architecture = "amd64",
    built_using = "unzip (6.0.1)",
    data = ":debian-data",
    depends = [
        "zlib1g-dev",
        "unzip",
    ],
    description_file = "debian/description",
    homepage = "https://github.com/embarktrucks/embag",
    maintainer = "Jason Snell <jason@embarktrucks.com>",
    package = "embag",
    version = EMBAG_VERSION,
)
