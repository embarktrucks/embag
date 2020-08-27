#pragma once

#include <pybind11/pybind11.h>

namespace py = pybind11;

py::dict rosValueToDict(const Embag::RosValue &ros_value);
py::list rosValueToList(const Embag::RosValue &ros_value);

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
        list.append(value->as<std::string>());
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
        list.append(py::bytes(blob.data.c_str(), blob.size));
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

  for (const auto& element : ros_value.getObjects()) {
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
        dict[key] = value->as<std::string>();
        break;
      }
      case Type::ros_time: {
        dict[key] = value->as<Embag::RosValue::ros_time_t>();
        break;
      }
      case Type::ros_duration: {
        dict[key] = value->as<Embag::RosValue::ros_duration_t>();
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
        dict[key] = py::bytes(blob.data.c_str(), blob.byte_size);
        break;
      }
      default: {
        throw std::runtime_error("Unhandled type");
      }
    }
  }

  return dict;
}
