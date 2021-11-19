#pragma once

#include <pybind11/pybind11.h>

#include "utils.h"

namespace py = pybind11;

py::dict rosValueToDict(const Embag::RosValue::RosValuePointer &ros_value);
py::list rosValueToList(const Embag::RosValue::RosValuePointer &ros_value);

py::list rosValueToList(const Embag::RosValue::RosValuePointer &ros_value) {
  using Type = Embag::RosValue::Type;

  if (ros_value->getType() != Type::array) {
    throw std::runtime_error("Provided RosValue is not an array");
  }

  py::list list{};

  // TODO: Rather than creating a vector, RosValue should provide an iterator interface
  for (const auto &value : ros_value->getValues()) {
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
        list.append(rosValueToDict(value));
        break;
      }
      case Type::array: {
        list.append(rosValueToList(value));
        break;
      }
      default: {
        throw std::runtime_error("Unhandled type");
      }
    }
  }

  return list;
}

py::dict rosValueToDict(const Embag::RosValue::RosValuePointer &ros_value) {
  using Type = Embag::RosValue::Type;

  if (ros_value->getType() != Type::object) {
    throw std::runtime_error("Provided RosValue is not an object");
  }

  py::dict dict{};

  // TODO: Rather than creating an unordered_map, RosValue should provide an iterator interface
  for (const auto &element : ros_value->getObjects()) {
    const auto &key = element.first.c_str();
    const auto &value = element.second;

    switch (value->getType()) {
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
      // TODO: Don't return floats here - the raw ros time has more precision
      case Type::ros_time: {
        dict[key] = value->as<Embag::RosValue::ros_time_t>().to_sec();
        break;
      }
      case Type::ros_duration: {
        dict[key] = value->as<Embag::RosValue::ros_duration_t>().to_sec();
        break;
      }
      case Type::object: {
        dict[key] = rosValueToDict(value);
        break;
      }
      case Type::array: {
        dict[key] = rosValueToList(value);
        break;
      }
      default: {
        throw std::runtime_error("Unhandled type");
      }
    }
  }

  return dict;
}

py::object castValue(const Embag::RosValue::RosValuePointer& value) {
  switch (value->getType()) {
    case Embag::RosValue::Type::object:
    case Embag::RosValue::Type::array:
      return py::cast(value);
    case Embag::RosValue::Type::ros_bool:
      return py::cast(value->as<bool>());
    case Embag::RosValue::Type::int8:
      return py::cast(value->as<int8_t>());
    case Embag::RosValue::Type::uint8:
      return py::cast(value->as<uint8_t>());
    case Embag::RosValue::Type::int16:
      return py::cast(value->as<int16_t>());
    case Embag::RosValue::Type::uint16:
      return py::cast(value->as<uint16_t>());
    case Embag::RosValue::Type::int32:
      return py::cast(value->as<int32_t>());
    case Embag::RosValue::Type::uint32:
      return py::cast(value->as<uint32_t>());
    case Embag::RosValue::Type::int64:
      return py::cast(value->as<int64_t>());
    case Embag::RosValue::Type::uint64:
      return py::cast(value->as<uint64_t>());
    case Embag::RosValue::Type::float32:
      return py::cast(value->as<float>());
    case Embag::RosValue::Type::float64:
      return py::cast(value->as<double>());
    case Embag::RosValue::Type::string:
      return encodeStrLatin1(value->as<std::string>());
    // TODO: Don't return floats here - the raw ros time has more precision
    case Embag::RosValue::Type::ros_time:
      return py::cast(value->as<Embag::RosValue::ros_time_t>().to_sec());
    case Embag::RosValue::Type::ros_duration:
      return py::cast(value->as<Embag::RosValue::ros_duration_t>().to_sec());
    default:
      throw std::runtime_error("Unhandled type");
  }
}

py::object getField(Embag::RosValue::RosValuePointer &v, const std::string field_name) {
  if (v->getType() != Embag::RosValue::Type::object) {
    throw std::runtime_error("Can only getField on an object");
  }

  return castValue(v->get(field_name));
}

py::object getIndex(Embag::RosValue::RosValuePointer &v, const size_t index) {
  if (v->getType() != Embag::RosValue::Type::array) {
    throw std::runtime_error("Can only getIndex on an array");
  }

  return castValue(v->at(index));
}

namespace Embag {
template<>
const py::object RosValue::const_iterator<py::object, size_t>::operator*() const {
  return castValue(value_.getChildren().at(index_));
}
}
