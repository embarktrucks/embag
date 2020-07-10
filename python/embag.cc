#include "lib/view.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

// Prototypes
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
        dict[key] = py::bytes(blob.data.c_str(), blob.size);
        break;
      }
      default: {
        throw std::runtime_error("Unhandled type");
      }
    }
  }

  return dict;
}

py::object msgCompat(const Embag::RosValue &value) {
  using Type = Embag::RosValue::Type;

  switch(value.getType()) {
    case Type::object: {
      py::object py_msg = py::cast(value);

      for (const auto &object: value.getObjects()) {
        const auto &name = object.first;
        const std::shared_ptr<Embag::RosValue> &ros_value = object.second;

        switch (ros_value->getType()) {
          case Type::ros_bool: {
            py_msg.attr(name.c_str()) = ros_value->as<bool>();
            break;
          }
          case Type::int8: {
            py_msg.attr(name.c_str()) = ros_value->as<int8_t>();
            break;
          }
          case Type::uint8: {
            py_msg.attr(name.c_str()) = ros_value->as<uint8_t>();
            break;
          }
          case Type::int16: {
            py_msg.attr(name.c_str()) = ros_value->as<int16_t>();
            break;
          }
          case Type::uint16: {
            py_msg.attr(name.c_str()) = ros_value->as<uint16_t>();
            break;
          }
          case Type::int32: {
            py_msg.attr(name.c_str()) = ros_value->as<int32_t>();
            break;
          }
          case Type::uint32: {
            py_msg.attr(name.c_str()) = ros_value->as<uint32_t>();
            break;
          }
          case Type::int64: {
            py_msg.attr(name.c_str()) = ros_value->as<int64_t>();
            break;
          }
          case Type::uint64: {
            py_msg.attr(name.c_str()) = ros_value->as<uint64_t>();
            break;
          }
          case Type::float32: {
            py_msg.attr(name.c_str()) = ros_value->as<float>();
            break;
          }
          case Type::float64: {
            py_msg.attr(name.c_str()) = ros_value->as<double>();
            break;
          }
          case Type::string: {
            py_msg.attr(name.c_str()) = ros_value->as<std::string>();
            break;
          }
          case Type::ros_time: {
            py_msg.attr(name.c_str()) = ros_value->as<Embag::RosValue::ros_time_t>();
            break;
          }
          case Type::ros_duration: {
            py_msg.attr(name.c_str()) = ros_value->as<Embag::RosValue::ros_duration_t>();
            break;
          }
          case Type::object:
          case Type::array: {
            py_msg.attr(name.c_str()) = msgCompat(*ros_value);
            break;
          }
          case Type::blob: {
            const auto &blob = value.getBlob();
            py_msg.attr(name.c_str()) = py::bytes(blob.data.c_str(), blob.size);
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


PYBIND11_MODULE(embag, m) {
  m.doc() = "Python bindings for Embag";

  py::class_<Embag::Bag, std::shared_ptr<Embag::Bag>>(m, "Bag")
      .def(py::init<std::string>())
      .def("topics", &Embag::Bag::topics)
      .def("read_messages", [](std::shared_ptr<Embag::Bag> &bag, const std::vector<std::string>& topics) {
        Embag::View view{};
        view.addBag(bag);
        view.getMessages(topics);

        return py::make_iterator(IteratorCompat{view.begin()}, IteratorCompat{view.end()});
      }, py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */, py::arg("topics"));

  py::class_<Embag::View>(m, "View")
      .def(py::init())
      .def("addBag", &Embag::View::addBag)
      .def("getStartTime", &Embag::View::getStartTime)
      .def("getEndTime", &Embag::View::getEndTime)
      .def("getMessages", (Embag::View (Embag::View::*)(void)) &Embag::View::getMessages)
      .def("getMessages", (Embag::View (Embag::View::*)(const std::string &)) &Embag::View::getMessages)
      .def("getMessages", (Embag::View (Embag::View::*)(const std::vector<std::string> &)) &Embag::View::getMessages)
      .def("__iter__", [](Embag::View &v) {
        return py::make_iterator(v.begin(), v.end());
      }, py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */ );

  py::class_<Embag::RosMessage, std::shared_ptr<Embag::RosMessage>>(m, "RosMessage", py::dynamic_attr())
      .def(py::init())
      .def("__str__", &Embag::RosMessage::toString)
      .def("data", &Embag::RosMessage::data)
      .def("dict", [](std::shared_ptr<Embag::RosMessage> &m) {
        if (m->data().getType() != Embag::RosValue::Type::object) {
          throw std::runtime_error("Element is not an object");
        }

        return rosValueToDict(m->data());
      })
      .def_readonly("topic", &Embag::RosMessage::topic)
      .def_readonly("timestamp", &Embag::RosMessage::timestamp)
      .def_readonly("md5", &Embag::RosMessage::md5);

  auto ros_value = py::class_<Embag::RosValue, std::shared_ptr<Embag::RosValue>>(m, "RosValue", py::dynamic_attr())
      .def(py::init())
      .def("get", &Embag::RosValue::get)
      .def("__str__", &Embag::RosValue::toString, py::arg("path") = "")
      .def("__getitem__", [](std::shared_ptr<Embag::RosValue> &v, const std::string &key){
        if (v->getType() != Embag::RosValue::Type::object) {
          throw std::runtime_error("Element is not an object");
        }

        return rosValueToDict(v->get(key));
      });

  py::class_<Embag::RosValue::ros_time_t>(m, "RosTime")
      .def_readonly("secs", &Embag::RosValue::ros_time_t::secs)
      .def_readonly("nsecs", &Embag::RosValue::ros_time_t::nsecs)
      .def("to_sec", &Embag::RosValue::ros_time_t::to_sec)
      .def("__str__", [](Embag::RosValue::ros_time_t &v) {
        return std::to_string(v.secs) + "s " + std::to_string(v.nsecs) + "ns";
      });

  py::class_<Embag::RosValue::ros_duration_t>(m, "RosDuration")
      .def_readonly("secs", &Embag::RosValue::ros_duration_t::secs)
      .def_readonly("nsecs", &Embag::RosValue::ros_duration_t::nsecs)
      .def("__str__", [](Embag::RosValue::ros_duration_t &v) {
        return std::to_string(v.secs) + "s " + std::to_string(v.nsecs) + "ns";
      });
}
