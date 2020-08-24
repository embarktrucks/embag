load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")
load("@rules_python//experimental/python:wheel.bzl", "py_wheel")

pybind_extension(
    name = "embag",
    srcs = [
        "adapters.h",
        "embag.cc",
        "ros_compat.h",
        "schema_builder.cc",
        "schema_builder.h",
    ],
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
    version = "0.0.2",
    deps = [
        #        ":embag",
        ":embag.so",
    ],
)
