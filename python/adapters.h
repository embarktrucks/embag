#pragma once

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include "utils.h"

namespace py = pybind11;

#if __cplusplus < 201402L
struct EnumClassHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};
typedef std::unordered_set<Embag::RosValue::Type, EnumClassHash> RosValueTypeSet;
#else
typedef std::unordered_set<Embag::RosValue::Type> RosValueTypeSet;
#endif

py::dict rosValueToDict(
  const Embag::RosValue::Pointer &ros_value,
  const RosValueTypeSet &types_to_unpack,
  bool packed_types_as_memoryview,
  const py::object& ros_time_py_type);
py::list rosValueToList(
  const Embag::RosValue::Pointer &ros_value,
  const RosValueTypeSet &types_to_unpack,
  bool packed_types_as_memoryview,
  const py::object& ros_time_py_type);
py::object castValue(const Embag::RosValue::Pointer &value, const py::object& ros_time_py_type);

// By default, it doesn't make sense to return a memoryview for python non-primitive types
const RosValueTypeSet default_types_to_unpack = {
  Embag::RosValue::Type::ros_duration,
  Embag::RosValue::Type::ros_time
};

py::object primitiveArrayToPyObject(
    const Embag::RosValue::Pointer &primitive_array,
    const RosValueTypeSet &types_to_unpack=default_types_to_unpack,
    bool packed_types_as_memoryview=false,
    const py::object& ros_time_py_type=py::none()) {
  const Embag::RosValue::Type item_type = primitive_array->getElementType();

  if (types_to_unpack.find(item_type) != types_to_unpack.end()) {
    return rosValueToList(primitive_array, default_types_to_unpack, packed_types_as_memoryview, ros_time_py_type);
  } else if (packed_types_as_memoryview) {
    #if PY_VERSION_HEX >= 0x03030000
      // In python 3.3 and above, memoryview provides good support for converting to a list via tolist
      return py::memoryview(py::cast(primitive_array));
    #else
      // In other versions, we need to rely on numpy arrays to provide a powerful tolist functionality
      return py::array(py::cast(primitive_array));
    #endif
  } else {
    return py::bytes(
      static_cast<const char *>(primitive_array->getPrimitiveArrayRosValueBuffer()),
      primitive_array->getPrimitiveArrayRosValueBufferSize());
  }
}

py::list rosValueToList(
    const Embag::RosValue::Pointer &ros_value,
    const RosValueTypeSet &types_to_unpack=default_types_to_unpack,
    bool packed_types_as_memoryview=false,
    const py::object& ros_time_py_type=py::none()) {
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
        list.append(castValue(value, ros_time_py_type));
        break;
      }
      case Type::object: {
        list.append(rosValueToDict(value, types_to_unpack, packed_types_as_memoryview, ros_time_py_type));
        break;
      }
      case Type::primitive_array: {
        list.append(primitiveArrayToPyObject(value, types_to_unpack, packed_types_as_memoryview, ros_time_py_type));
        break;
      }
      case Type::array:
      {
        list.append(rosValueToList(value, types_to_unpack, packed_types_as_memoryview, ros_time_py_type));
        break;
      }
      default: {
        throw std::runtime_error("Unhandled type");
      }
    }
  }

  return list;
}

py::dict rosValueToDict(
    const Embag::RosValue::Pointer &ros_value,
    const RosValueTypeSet &types_to_unpack=default_types_to_unpack,
    bool packed_types_as_memoryview=false,
    const py::object& ros_time_py_type=py::none()) {
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
        dict[key] = castValue(value, ros_time_py_type);
        break;
      }
      case Type::object: {
        dict[key] = rosValueToDict(value, types_to_unpack, packed_types_as_memoryview, ros_time_py_type);
        break;
      }
      case Type::primitive_array: {
        dict[key] = primitiveArrayToPyObject(value, types_to_unpack, packed_types_as_memoryview, ros_time_py_type);
        break;
      }
      case Type::array: 
      {
        dict[key] = rosValueToList(value, types_to_unpack, packed_types_as_memoryview, ros_time_py_type);
        break;
      }
      default: {
        throw std::runtime_error("Unhandled type");
      }
    }
  }

  return dict;
}

template<typename RosTimeType>
py::object castRosTime(const Embag::RosValue::Pointer& ros_value, const py::object& py_type=py::none()) {
  const RosTimeType time_value = ros_value->as<RosTimeType>();
  if (py_type.is_none()) {
    // Keep as a RosTime or RosDuration
    return py::cast(time_value);
  } else if (!py::isinstance<py::type>(py_type)) {
    throw py::type_error("Provided python type for casting a ROS time is not a type!");
  } else {
    PyObject* native_py_type = py_type.ptr();
    if (
      native_py_type == (PyObject*) &PyLong_Type
      // In python2, we also need to support the PyInt_Type
      // (In python3, all `int`s are `long`s under the hood)
      #if PY_VERSION_HEX < 0x03000000
      || native_py_type == (PyObject*) &PyInt_Type
      #endif
    ) {
      return py::cast(time_value.to_nsec());
    } else if (native_py_type == (PyObject*) &PyFloat_Type) {
      return py::cast(time_value.to_sec());
    } else {
      throw py::value_error("Can only cast ROS times and durations to int or float!");
    }
  }
}

py::object castValue(const Embag::RosValue::Pointer& value, const py::object& ros_time_py_type=py::none()) {
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
    case Embag::RosValue::Type::ros_time:
      return castRosTime<Embag::RosValue::ros_time_t>(value, ros_time_py_type);
    case Embag::RosValue::Type::ros_duration:
      return castRosTime<Embag::RosValue::ros_duration_t>(value, ros_time_py_type);
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

template<>
const py::str RosValue::const_iterator<py::str, std::unordered_map<std::string, size_t>::const_iterator>::operator*() const {
  return index_->first;
}

template<>
const py::tuple RosValue::const_iterator<py::tuple, std::unordered_map<std::string, size_t>::const_iterator>::operator*() const {
  return py::make_tuple(index_->first, castValue(value_.object_info_.children.at(index_->second)));
}
}
