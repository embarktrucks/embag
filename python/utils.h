#pragma once

#include <pybind11/pybind11.h>
#include <Python.h>


pybind11::str encodeStrLatin1(const std::string& str) {
  return pybind11::reinterpret_steal<pybind11::str>(PyUnicode_DecodeLatin1(str.data(), str.length(), nullptr));
}
