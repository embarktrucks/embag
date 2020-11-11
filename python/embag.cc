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
      .def(py::init<std::shared_ptr<Embag::Bag>>())
      .def(py::init<const std::string&>())
      .def("addBag", (Embag::View (Embag::View::*)(const std::string &)) &Embag::View::addBag)
      .def("add_bag", (Embag::View (Embag::View::*)(const std::string &)) &Embag::View::addBag)
      .def("addBag", (Embag::View (Embag::View::*)(std::shared_ptr<Embag::Bag>)) &Embag::View::addBag)
      .def("add_bag", (Embag::View (Embag::View::*)(std::shared_ptr<Embag::Bag>)) &Embag::View::addBag)
      .def("getStartTime", &Embag::View::getStartTime)
      .def("get_start_time", &Embag::View::getStartTime)
      .def("get_end_time", &Embag::View::getEndTime)
      .def("getMessages", (Embag::View (Embag::View::*)(void)) &Embag::View::getMessages)
      .def("get_messages", (Embag::View (Embag::View::*)(void)) &Embag::View::getMessages)
      .def("getMessages", (Embag::View (Embag::View::*)(const std::string &)) &Embag::View::getMessages)
      .def("get_messages", (Embag::View (Embag::View::*)(const std::string &)) &Embag::View::getMessages)
      .def("getMessages", (Embag::View (Embag::View::*)(const std::vector<std::string> &)) &Embag::View::getMessages)
      .def("get_messages", (Embag::View (Embag::View::*)(const std::vector<std::string> &)) &Embag::View::getMessages)
      .def("__iter__", [](Embag::View &v) {
        return py::make_iterator(v.begin(), v.end());
      }, py::keep_alive<0, 1>() /* Essential: keep object alive while iterator exists */ );

  py::class_<Embag::RosMessage, std::shared_ptr<Embag::RosMessage>>(m, "RosMessage", py::dynamic_attr())
      .def(py::init())
      .def("__str__", [](std::shared_ptr<Embag::RosMessage> &m) {
        return encodeStrLatin1(m->toString());
      })
      .def("data", &Embag::RosMessage::data)
      .def("get", [](std::shared_ptr<Embag::RosMessage> &m, const std::string& key) {
        return m->data().get(key);
      })
      .def("__getitem__", [](std::shared_ptr<Embag::RosMessage> &m, const std::string& key) {
        return m->data().get(key);
      })
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
      .def("as_bool", &Embag::RosValue::as<bool>)
      .def("as_uint8", &Embag::RosValue::as<uint8_t>)
      .def("as_int8", &Embag::RosValue::as<int8_t>)
      .def("as_uint16", &Embag::RosValue::as<uint16_t>)
      .def("as_int16", &Embag::RosValue::as<int16_t>)
      .def("as_uint32", &Embag::RosValue::as<uint32_t>)
      .def("as_int32", &Embag::RosValue::as<int32_t>)
      .def("as_uint64", &Embag::RosValue::as<uint64_t>)
      .def("as_int64", &Embag::RosValue::as<int64_t>)
      .def("as_float", &Embag::RosValue::as<float>)
      .def("as_double", &Embag::RosValue::as<double>)
      .def("as_string", &Embag::RosValue::as<std::string>)
      .def("as_ros_time", &Embag::RosValue::as<Embag::RosValue::ros_time_t>)
      .def("as_ros_duration", &Embag::RosValue::as<Embag::RosValue::ros_duration_t>)
      .def("as_blob", &Embag::RosValue::getBlob)
      .def("__str__", [](std::shared_ptr<Embag::RosValue> &v, const std::string &path) {
        return encodeStrLatin1(v->toString());
      }, py::arg("path") = "")
      .def("__getitem__", [](std::shared_ptr<Embag::RosValue> &v, const std::string &key) {
        if (v->getType() != Embag::RosValue::Type::object) {
          throw std::runtime_error("Element is not an object");
        }
        return v->get(key);
      });

  py::class_<Embag::RosValue::blob_t>(m, "RosBlob")
      .def_readonly("byte_size", &Embag::RosValue::blob_t::byte_size)
      .def_readonly("size", &Embag::RosValue::blob_t::size)
      .def("__len__", [](Embag::RosValue::blob_t &b) {
        return b.size;
      })
      .def("__getitem__", [](Embag::RosValue::blob_t &b, const size_t idx) {
        return 1;
      })
      .def("bytes", [](Embag::RosValue::blob_t &b){
        return py::bytes(b.data.c_str(), b.byte_size);
      })
      .def("to_list", [](Embag::RosValue::blob_t &b) {
        return rosBlobToList(b);
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
