#include "lib/view.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "schema_builder.h"
#include "adapters.h"
#include "ros_compat.h"
#include "utils.h"

namespace py = pybind11;

PYBIND11_MODULE(libembag, m) {
  m.doc() = "Python bindings for Embag";

  py::class_<Embag::Bag, std::shared_ptr<Embag::Bag>>(m, "Bag")
      .def(py::init<const std::string>())
      .def(py::init([](const std::string &bytes, size_t length) {
        return std::make_shared<Embag::Bag>(std::make_shared<const std::string>(bytes));
      }))
      .def("topics", &Embag::Bag::topics)
      .def("read_messages", [](std::shared_ptr<Embag::Bag> &bag, const std::vector<std::string>& topics) {
        Embag::View view{};
        view.addBag(bag);
        view.getMessages(topics);

        return py::make_iterator(IteratorCompat{view.begin()}, IteratorCompat{view.end()});
      }, py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */, py::arg("topics"))
      .def("getSchema", [](std::shared_ptr<Embag::Bag> &bag, const std::string &topic) {
        auto builder = SchemaBuilder{bag};
        return builder.generateSchema(topic);
      })
      .def("connectionsByTopic", [](std::shared_ptr<Embag::Bag> &bag) {
        std::unordered_map<std::string, std::vector<Embag::RosBagTypes::connection_data_t>> connections_by_topic;
        for (const auto &item: bag->connectionsByTopicMap()) {
          auto &connections = connections_by_topic[item.first];
          for (auto *c: item.second) {
            connections.emplace_back(c->data);
          }
        }
        return connections_by_topic;
      })
      .def("close", &Embag::Bag::close);

  py::class_<Embag::View>(m, "View")
      .def(py::init())
      .def(py::init<std::shared_ptr<Embag::Bag>>())
      .def(py::init<const std::string&>())
      .def("addBag", (Embag::View (Embag::View::*)(const std::string &)) &Embag::View::addBag)
      .def("addBag", (Embag::View (Embag::View::*)(std::shared_ptr<Embag::Bag>)) &Embag::View::addBag)
      .def("getStartTime", &Embag::View::getStartTime)
      .def("getEndTime", &Embag::View::getEndTime)
      .def("getMessages", (Embag::View (Embag::View::*)(void)) &Embag::View::getMessages)
      .def("getMessages", (Embag::View (Embag::View::*)(const std::string &)) &Embag::View::getMessages)
      .def("getMessages", (Embag::View (Embag::View::*)(const std::vector<std::string> &)) &Embag::View::getMessages)
      .def("__iter__", [](Embag::View &v) {
        return py::make_iterator(v.begin(), v.end());
      }, py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */ )
      .def("topics", &Embag::View::topics)
      .def("connectionsByTopic", [](Embag::View &view) {
        std::unordered_map<std::string, std::vector<Embag::RosBagTypes::connection_data_t>> connections_by_topic;
        for (const auto &item: view.connectionsByTopicMap()) {
          auto &connections = connections_by_topic[item.first];
          for (auto *c: item.second) {
            connections.emplace_back(c->data);
          }
        }
        return connections_by_topic;
      });

  py::class_<Embag::RosMessage, std::shared_ptr<Embag::RosMessage>>(m, "RosMessage", py::dynamic_attr())
      .def(py::init())
      .def("__str__", [](std::shared_ptr<Embag::RosMessage> &m) {
        return encodeStrLatin1(m->toString());
      })
      .def("data", &Embag::RosMessage::data)
      .def("dict", [](std::shared_ptr<Embag::RosMessage> &m) {
        if (m->data().getType() != Embag::RosValue::Type::object) {
          throw std::runtime_error("Element is not an object");
        }

        return rosValueToDict(m->data());
      })
      .def_readonly("topic", &Embag::RosMessage::topic)
      .def_readonly("timestamp", &Embag::RosMessage::timestamp)
      .def_readonly("md5", &Embag::RosMessage::md5)
      .def_readonly("raw_data_len", &Embag::RosMessage::raw_data_len);

  auto ros_value = py::class_<Embag::RosValue, std::shared_ptr<Embag::RosValue>>(m, "RosValue", py::dynamic_attr())
      .def(py::init())
      .def("get", &Embag::RosValue::get)
      .def("getType", &Embag::RosValue::getType)
      .def("__str__", [](std::shared_ptr<Embag::RosValue> &v, const std::string &path) {
        return encodeStrLatin1(v->toString());
      }, py::arg("path") = "")
      .def("__getitem__", [](std::shared_ptr<Embag::RosValue> &v, const std::string &key) {
        if (v->getType() != Embag::RosValue::Type::object) {
          throw std::runtime_error("Element is not an object");
        }

        return rosValueToDict(v->get(key));
      });

  py::enum_<Embag::RosValue::Type>(m, "RosValueType")
    .value("bool", Embag::RosValue::Type::ros_bool)
    .value("uint8", Embag::RosValue::Type::uint8)
    .value("int8", Embag::RosValue::Type::int8)
    .value("uint16", Embag::RosValue::Type::uint16)
    .value("int16", Embag::RosValue::Type::int16)
    .value("uint32", Embag::RosValue::Type::uint32)
    .value("int32", Embag::RosValue::Type::int32)
    .value("uint64", Embag::RosValue::Type::uint64)
    .value("int64", Embag::RosValue::Type::int64)
    .value("float32", Embag::RosValue::Type::float32)
    .value("float64", Embag::RosValue::Type::float64)
    .value("blob", Embag::RosValue::Type::blob)
    .value("array", Embag::RosValue::Type::array)
    .value("object", Embag::RosValue::Type::object)
    .value("string", Embag::RosValue::Type::string)
    .value("time", Embag::RosValue::Type::ros_time)
    .value("duration", Embag::RosValue::Type::ros_duration)
    .export_values();

  py::class_<Embag::RosValue::ros_time_t>(m, "RosTime")
      .def_readonly("secs", &Embag::RosValue::ros_time_t::secs)
      .def_readonly("nsecs", &Embag::RosValue::ros_time_t::nsecs)
      .def("to_sec", &Embag::RosValue::ros_time_t::to_sec)
      .def("__str__", [](Embag::RosValue::ros_time_t &v) {
        return std::to_string(v.to_nsec());
      });

  py::class_<Embag::RosValue::ros_duration_t>(m, "RosDuration")
      .def_readonly("secs", &Embag::RosValue::ros_duration_t::secs)
      .def_readonly("nsecs", &Embag::RosValue::ros_duration_t::nsecs)
      .def("__str__", [](Embag::RosValue::ros_duration_t &v) {
        return std::to_string(v.to_nsec());
      });

  py::class_<Embag::RosBagTypes::connection_data_t>(m, "Connection")
      .def_readonly("topic", &Embag::RosBagTypes::connection_data_t::topic)
      .def_readonly("type", &Embag::RosBagTypes::connection_data_t::type)
      .def_readonly("scope", &Embag::RosBagTypes::connection_data_t::scope)
      .def_readonly("md5sum", &Embag::RosBagTypes::connection_data_t::md5sum)
      .def_readonly("message_definition", &Embag::RosBagTypes::connection_data_t::message_definition)
      .def_readonly("callerid", &Embag::RosBagTypes::connection_data_t::callerid)
      .def_readonly("latching", &Embag::RosBagTypes::connection_data_t::latching)
      .def("__repr__", [](const Embag::RosBagTypes::connection_data_t &c) {
        return "<embag.Connection '" + c.type + "' from '" + c.callerid + "'>";
      });
}
