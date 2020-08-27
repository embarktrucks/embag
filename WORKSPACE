load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "com_github_nelhage_rules_boost",
    commit = "9f9fb8b2f0213989247c9d5c0e814a8451d18d7f",
    remote = "https://github.com/nelhage/rules_boost",
    shallow_since = "1570056263 -0700",
)

load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")

boost_deps()

new_local_repository(
    name = "system_libs",
    build_file_content = """
cc_library(
	name = "liblz4",
	srcs = ["liblz4.so"],
	visibility = ["//visibility:public"],
)
""",
    path = "/usr/lib/x86_64-linux-gnu",
)

# .deb building bits
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_pkg",
    sha256 = "4ba8f4ab0ff85f2484287ab06c0d871dcb31cc54d439457d28fd4ae14b18450a",
    url = "https://github.com/bazelbuild/rules_pkg/releases/download/0.2.4/rules_pkg-0.2.4.tar.gz",
)

load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")

rules_pkg_dependencies()

# Pybind11
http_archive(
    name = "pybind11_bazel",
    strip_prefix = "pybind11_bazel-203508e14aab7309892a1c5f7dd05debda22d9a5",
    urls = ["https://github.com/pybind/pybind11_bazel/archive/203508e.zip"],
)

http_archive(
    name = "pybind11",
    build_file = "@pybind11_bazel//:pybind11.BUILD",
    strip_prefix = "pybind11-2.5.0",
    urls = ["https://github.com/pybind/pybind11/archive/v2.5.0.tar.gz"],
)

load("@pybind11_bazel//:python_configure.bzl", "python_configure")

python_configure(
    name = "local_config_python",
    # Change this to "2" when compiling for Python 2.  I'm unclear if it's possible to have both targets at the same time...
    python_version = "3",
)

# Experimental python rules
git_repository(
    name = "rules_python",
    commit = "cd552725122fdfe06a72864e21a92b5093a1857d",
    remote = "https://github.com/bazelbuild/rules_python.git",
    shallow_since = "1593046824 -0700",
)

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()

# Only needed if using the packaging rules.
load("@rules_python//python:pip.bzl", "pip_repositories")

pip_repositories()

# GTest
git_repository(
    name = "gtest",
    commit = "6a7ed316a5cdc07b6d26362c90770787513822d4",
    remote = "https://github.com/google/googletest",
    shallow_since = "1583246693 -0500",
)
