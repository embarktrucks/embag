#pragma once

#include <pybind11/pybind11.h>

#include "lib/view.h"

namespace py = pybind11;

class SchemaBuilder {
 public:
  explicit SchemaBuilder(std::shared_ptr<Embag::Bag> &bag) : bag_(bag) {};

  py::dict generateSchema(const std::string &topic);

 private:
  py::dict schemaForField(const std::string &scope, const Embag::RosMsgTypes::ros_msg_field &field) const;

  std::shared_ptr<Embag::Bag> bag_;
  std::shared_ptr<Embag::RosMsgTypes::ros_msg_def> msg_def_;
};
