#include "lib/view.h"

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "schema_builder.h"
#include "adapters.h"
#include "ros_compat.h"

namespace py = pybind11;

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
      }, py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */, py::arg("topics"))
      .def("getSchema", [](std::shared_ptr<Embag::Bag> &bag, const std::string &topic) {
        auto builder = SchemaBuilder{bag};
        return builder.generateSchema(topic);
      })
      .def("close", &Embag::Bag::close);

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
        return std::to_string(v.to_nsec());
      });

  py::class_<Embag::RosValue::ros_duration_t>(m, "RosDuration")
      .def_readonly("secs", &Embag::RosValue::ros_duration_t::secs)
      .def_readonly("nsecs", &Embag::RosValue::ros_duration_t::nsecs)
      .def("__str__", [](Embag::RosValue::ros_duration_t &v) {
        return std::to_string(v.to_nsec());
      });
}
