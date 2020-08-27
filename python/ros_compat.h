#pragma once

#include <pybind11/pybind11.h>
#include "lib/view.h"

namespace py = pybind11;

py::object msgCompat(const Embag::RosValue &value) {
  using Type = Embag::RosValue::Type;

  switch(value.getType()) {
    case Type::object: {
      py::object py_msg = py::cast(value);

      for (const auto &object: value.getObjects()) {
        const auto &name = object.first.c_str();
        const std::shared_ptr<Embag::RosValue> &ros_value = object.second;

        switch (ros_value->getType()) {
          case Type::ros_bool: {
            py_msg.attr(name) = ros_value->as<bool>();
            break;
          }
          case Type::int8: {
            py_msg.attr(name) = ros_value->as<int8_t>();
            break;
          }
          case Type::uint8: {
            py_msg.attr(name) = ros_value->as<uint8_t>();
            break;
          }
          case Type::int16: {
            py_msg.attr(name) = ros_value->as<int16_t>();
            break;
          }
          case Type::uint16: {
            py_msg.attr(name) = ros_value->as<uint16_t>();
            break;
          }
          case Type::int32: {
            py_msg.attr(name) = ros_value->as<int32_t>();
            break;
          }
          case Type::uint32: {
            py_msg.attr(name) = ros_value->as<uint32_t>();
            break;
          }
          case Type::int64: {
            py_msg.attr(name) = ros_value->as<int64_t>();
            break;
          }
          case Type::uint64: {
            py_msg.attr(name) = ros_value->as<uint64_t>();
            break;
          }
          case Type::float32: {
            py_msg.attr(name) = ros_value->as<float>();
            break;
          }
          case Type::float64: {
            py_msg.attr(name) = ros_value->as<double>();
            break;
          }
          case Type::string: {
            py_msg.attr(name) = ros_value->as<std::string>();
            break;
          }
          case Type::ros_time: {
            py_msg.attr(name) = ros_value->as<Embag::RosValue::ros_time_t>();
            break;
          }
          case Type::ros_duration: {
            py_msg.attr(name) = ros_value->as<Embag::RosValue::ros_duration_t>();
            break;
          }
          case Type::object:
          case Type::array: {
            py_msg.attr(name) = msgCompat(*ros_value);
            break;
          }
          case Type::blob: {
            const auto &blob = ros_value->getBlob();
            py_msg.attr(name) = py::bytes(blob.data.c_str(), blob.byte_size);
            break;
          }
          default: {
            throw std::runtime_error("Unhandled type");
          }
        }
      }

      return py_msg;
    } // end object case
    case Type::array: {
      py::list list{};

      for (const auto &ros_value : value.getValues()) {
        switch (ros_value->getType()) {
          case Type::ros_bool: {
            list.append(ros_value->as<bool>());
            break;
          }
          case Type::int8: {
            list.append(ros_value->as<int8_t>());
            break;
          }
          case Type::uint8: {
            list.append(ros_value->as<uint8_t>());
            break;
          }
          case Type::int16: {
            list.append(ros_value->as<int16_t>());
            break;
          }
          case Type::uint16: {
            list.append(ros_value->as<uint16_t>());
            break;
          }
          case Type::int32: {
            list.append(ros_value->as<int32_t>());
            break;
          }
          case Type::uint32: {
            list.append(ros_value->as<uint32_t>());
            break;
          }
          case Type::int64: {
            list.append(ros_value->as<int64_t>());
            break;
          }
          case Type::uint64: {
            list.append(ros_value->as<uint64_t>());
            break;
          }
          case Type::float32: {
            list.append(ros_value->as<float>());
            break;
          }
          case Type::float64: {
            list.append(ros_value->as<double>());
            break;
          }
          case Type::string: {
            list.append(ros_value->as<std::string>());
            break;
          }
          case Type::ros_time: {
            list.append(ros_value->as<Embag::RosValue::ros_time_t>());
            break;
          }
          case Type::ros_duration: {
            list.append(ros_value->as<Embag::RosValue::ros_duration_t>());
            break;
          }
          case Type::object:
          case Type::array: {
            list.append(msgCompat(*ros_value));
            break;
          }
          case Type::blob: {
            const auto &blob = ros_value->getBlob();
            list.append(py::bytes(blob.data.c_str(), blob.size));
            break;
          }
          default: {
            throw std::runtime_error("Unhandled type");
          }
        }
      } // end array case

      return std::move(list);
    }
    default: {
      throw std::runtime_error("Unhandled type at top level");
    }
  }
}

struct IteratorCompat {
  explicit IteratorCompat(const Embag::View::iterator& i) : iterator_(i) {}

  py::tuple operator*() const {
    const auto msg = *iterator_;
    return py::make_tuple(msg->topic, msgCompat(msg->data()), msg->timestamp);
  }

  bool operator==(const IteratorCompat &other) const {
    return iterator_ == other.iterator_;
  }

  Embag::View::iterator& operator++() {
    return ++iterator_;
  }

  Embag::View::iterator iterator_;
};
