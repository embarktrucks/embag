#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "utils.h"

namespace py = pybind11;

py::dict rosValueToDict(const Embag::RosValue::Pointer &ros_value, bool always_list_primitive_array);
py::list rosValueToList(const Embag::RosValue::Pointer &ros_value, bool always_list_primitive_array);
py::object castValue(const Embag::RosValue::Pointer &value);

py::object primitiveArrayToPyObject(const Embag::RosValue::Pointer &primitive_array, bool always_list) {
  const Embag::RosValue::Type item_type = primitive_array->getElementType();
  if (always_list || item_type == Embag::RosValue::Type::ros_time || item_type == Embag::RosValue::Type::ros_duration) {
    // Don't return primitive for non python primitive types
    return rosValueToList(primitive_array, always_list);
  } else {
    #if PY_VERSION_HEX >= 0x03030000
      // In python 3.3 and above, memoryview provides good support for converting to a list via tolist
      return py::memoryview(py::cast(primitive_array));
    #else
      // In other versions, we need to rely on numpy arrays to provide a powerful tolist functionality
      return py::array(py::cast(primitive_array));
    #endif
  }
}

py::list rosValueToList(const Embag::RosValue::Pointer &ros_value, bool always_list_primitive_array) {
  using Type = Embag::RosValue::Type;

  if (ros_value->getType() != Type::array && ros_value->getType() != Type::primitive_array) {
    throw std::runtime_error("Provided RosValue is not an array");
  }

  py::list list{};

  // TODO: Rather than creating a vector, RosValue should provide an iterator interface
  for (const auto &value : ros_value->getValues()) {
    switch (value->getType()) {
      case Type::ros_bool:
      case Type::int8:
      case Type::uint8:
      case Type::int16:
      case Type::uint16:
      case Type::int32:
      case Type::uint32:
      case Type::int64:
      case Type::uint64:
      case Type::float32:
      case Type::float64:
      case Type::string:
      case Type::ros_time:
      case Type::ros_duration: {
        list.append(castValue(value));
        break;
      }
      case Type::object: {
        list.append(rosValueToDict(value, always_list_primitive_array));
        break;
      }
      case Type::primitive_array: {
        list.append(primitiveArrayToPyObject(value, always_list_primitive_array));
        break;
      }
      case Type::array:
      {
        list.append(rosValueToList(value, always_list_primitive_array));
        break;
      }
      default: {
        throw std::runtime_error("Unhandled type");
      }
    }
  }

  return list;
}

py::dict rosValueToDict(const Embag::RosValue::Pointer &ros_value, bool always_list_primitive_array) {
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
      case Type::ros_bool:
      case Type::int8:
      case Type::uint8:
      case Type::int16:
      case Type::uint16:
      case Type::int32:
      case Type::uint32:
      case Type::int64:
      case Type::uint64:
      case Type::float32:
      case Type::float64:
      case Type::string:
      case Type::ros_time:
      case Type::ros_duration: {
        dict[key] = castValue(value);
        break;
      }
      case Type::object: {
        dict[key] = rosValueToDict(value, always_list_primitive_array);
        break;
      }
      case Type::primitive_array: {
        dict[key] = primitiveArrayToPyObject(value, always_list_primitive_array);
        break;
      }
      case Type::array: 
      {
        dict[key] = rosValueToList(value, always_list_primitive_array);
        break;
      }
      default: {
        throw std::runtime_error("Unhandled type");
      }
    }
  }

  return dict;
}

py::object castValue(const Embag::RosValue::Pointer& value) {
  switch (value->getType()) {
    case Embag::RosValue::Type::object:
    case Embag::RosValue::Type::array:
    case Embag::RosValue::Type::primitive_array:
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

py::object getField(Embag::RosValue::Pointer &v, const std::string field_name) {
  if (v->getType() != Embag::RosValue::Type::object) {
    throw std::runtime_error("Can only getField on an object");
  }

  return castValue(v->get(field_name));
}

py::object getIndex(Embag::RosValue::Pointer &v, const size_t index) {
  if (v->getType() != Embag::RosValue::Type::array) {
    throw std::runtime_error("Can only getIndex on an array");
  }

  return castValue(v->at(index));
}

namespace Embag {
template<>
const py::object RosValue::const_iterator<py::object, size_t>::operator*() const {
  return castValue(value_.at(index_));
}

}
