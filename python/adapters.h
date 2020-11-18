#pragma once

#include <pybind11/pybind11.h>

#include "utils.h"

namespace py = pybind11;

py::dict rosValueToDict(const Embag::RosValue &ros_value);
py::list rosValueToList(const Embag::RosValue &ros_value);

py::list unpackBlob(const Embag::RosValue::blob_t &blob) {
  using Type = Embag::RosValue::Type;

  py::list list{};
  for (size_t i = 0; i < blob.size; i++) {
    switch (blob.type) {
      case Type::ros_bool: {
        list.append(*(reinterpret_cast<const bool *>(blob.data.data()) + i));
        break;
      }
      case Type::int8: {
        list.append(*(reinterpret_cast<const int8_t *>(blob.data.data()) + i));
        break;
      }
      case Type::int16: {
        list.append(*(reinterpret_cast<const int16_t *>(blob.data.data()) + i));
        break;
      }
      case Type::uint16: {
        list.append(*(reinterpret_cast<const uint16_t *>(blob.data.data()) + i));
        break;
      }
      case Type::int32: {
        list.append(*(reinterpret_cast<const int32_t *>(blob.data.data()) + i));
        break;
      }
      case Type::uint32: {
        list.append(*(reinterpret_cast<const uint32_t *>(blob.data.data()) + i));
        break;
      }
      case Type::int64: {
        list.append(*(reinterpret_cast<const int64_t *>(blob.data.data()) + i));
        break;
      }
      case Type::uint64: {
        list.append(*(reinterpret_cast<const uint64_t *>(blob.data.data()) + i));
        break;
      }
      case Type::float32: {
        list.append(*(reinterpret_cast<const float *>(blob.data.data()) + i));
        break;
      }
      case Type::float64: {
        list.append(*(reinterpret_cast<const double *>(blob.data.data()) + i));
        break;
      }
      default: {
        throw std::runtime_error("Unable to deserialize blob");
      }
    }
  }

  return list;
}

py::list rosValueToList(const Embag::RosValue &ros_value) {
  using Type = Embag::RosValue::Type;

  py::list list{};

  for (const auto &value : ros_value.getValues()) {
    switch (value->getType()) {
      case Type::ros_bool: {
        list.append(value->as<bool>());
        break;
      }
      case Type::int8: {
        list.append(value->as<int8_t>());
        break;
      }
      case Type::uint8: {
        list.append(value->as<uint8_t>());
        break;
      }
      case Type::int16: {
        list.append(value->as<int16_t>());
        break;
      }
      case Type::uint16: {
        list.append(value->as<uint16_t>());
        break;
      }
      case Type::int32: {
        list.append(value->as<int32_t>());
        break;
      }
      case Type::uint32: {
        list.append(value->as<uint32_t>());
        break;
      }
      case Type::int64: {
        list.append(value->as<int64_t>());
        break;
      }
      case Type::uint64: {
        list.append(value->as<uint64_t>());
        break;
      }
      case Type::float32: {
        list.append(value->as<float>());
        break;
      }
      case Type::float64: {
        list.append(value->as<double>());
        break;
      }
      case Type::string: {
        list.append(encodeStrLatin1(value->as<std::string>()));
        break;
      }
      case Type::ros_time: {
        list.append(value->as<Embag::RosValue::ros_time_t>());
        break;
      }
      case Type::ros_duration: {
        list.append(value->as<Embag::RosValue::ros_duration_t>());
        break;
      }
      case Type::object: {
        list.append(rosValueToDict(*value));
        break;
      }
      case Type::array: {
        list.append(rosValueToList(*value));
        break;
      }
      case Type::blob: {
        const auto &blob = value->getBlob();
        // Binary data is usually stored as arrays of uint8s
        if (blob.type == Type::uint8) {
          list.append(py::bytes(blob.data.c_str(), blob.byte_size));
        } else {
          list.append(unpackBlob(blob));
        }
        break;
      }
      default: {
        throw std::runtime_error("Unhandled type");
      }
    }
  }

  return list;
}

py::dict rosValueToDict(const Embag::RosValue &ros_value) {
  using Type = Embag::RosValue::Type;

  py::dict dict{};

  for (const auto &element : ros_value.getObjects()) {
    const auto &key = element.first.c_str();
    const auto &value = element.second;

    switch (element.second->getType()) {
      case Type::ros_bool: {
        dict[key] = value->as<bool>();
        break;
      }
      case Type::int8: {
        dict[key] = value->as<int8_t>();
        break;
      }
      case Type::uint8: {
        dict[key] = value->as<uint8_t>();
        break;
      }
      case Type::int16: {
        dict[key] = value->as<int16_t>();
        break;
      }
      case Type::uint16: {
        dict[key] = value->as<uint16_t>();
        break;
      }
      case Type::int32: {
        dict[key] = value->as<int32_t>();
        break;
      }
      case Type::uint32: {
        dict[key] = value->as<uint32_t>();
        break;
      }
      case Type::int64: {
        dict[key] = value->as<int64_t>();
        break;
      }
      case Type::uint64: {
        dict[key] = value->as<uint64_t>();
        break;
      }
      case Type::float32: {
        dict[key] = value->as<float>();
        break;
      }
      case Type::float64: {
        dict[key] = value->as<double>();
        break;
      }
      case Type::string: {
        dict[key] = encodeStrLatin1(value->as<std::string>());
        break;
      }
      case Type::ros_time: {
        dict[key] = value->as<Embag::RosValue::ros_time_t>().to_sec();
        break;
      }
      case Type::ros_duration: {
        dict[key] = value->as<Embag::RosValue::ros_duration_t>().to_sec();
        break;
      }
      case Type::object: {
        dict[key] = rosValueToDict(*value);
        break;
      }
      case Type::array: {
        dict[key] = rosValueToList(*value);
        break;
      }
      case Type::blob: {
        const auto &blob = value->getBlob();
        // Binary data is usually stored as arrays of uint8s
        if (blob.type == Type::uint8) {
          dict[key] = py::bytes(blob.data.c_str(), blob.byte_size);
        } else {
          dict[key] = unpackBlob(blob);
        }
        break;
      }
      default: {
        throw std::runtime_error("Unhandled type");
      }
    }
  }

  return dict;
}
