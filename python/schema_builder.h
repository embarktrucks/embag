#pragma once

#include <pybind11/pybind11.h>

#include "lib/view.h"

namespace py = pybind11;

class SchemaBuilder {
 public:
  explicit SchemaBuilder(std::shared_ptr<Embag::Bag> &bag) : bag_(bag) {};

  py::object generateSchema(const std::string &topic);

 private:
  py::dict schemaForField(const std::string &scope, Embag::RosMsgTypes::ros_msg_field &field) const;

  std::shared_ptr<Embag::Bag> bag_;
  std::shared_ptr<Embag::RosMsgTypes::ros_msg_def> msg_def_;
  const py::object ordered_dict_ = py::module::import("collections").attr("OrderedDict");
};
