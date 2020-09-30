load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")
load("@rules_python//experimental/python:wheel.bzl", "py_wheel")
load("//lib:version.bzl", "EMBAG_VERSION")

pybind_extension(
    name = "libembag",
    srcs = [
        "adapters.h",
        "embag.cc",
        "ros_compat.h",
        "schema_builder.cc",
        "schema_builder.h",
        "utils.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//lib:embag",
    ],
)

py_wheel(
    name = "wheel",
    author = "Jason Snell",
    author_email = "jason@embarktrucks.com",
    description_file = "README.md",
    distribution = "embag",
    license = "MIT",
    # Only pure python wheels are officially supported for now...
    #platform = "linux_x86_64",
    strip_path_prefixes = [
        "python",
    ],
    version = EMBAG_VERSION,
    deps = [
        ":libembag.so",
    ],
)
