#pragma once

#include <pybind11/pybind11.h>

#include "lib/view.h"

namespace py = pybind11;

class SchemaBuilder {
 public:
  SchemaBuilder(Embag::View &view, const std::string &topic);
  SchemaBuilder(Embag::Bag &bag, const std::string &topic);
  SchemaBuilder(const std::string &message_type,
                const std::string &message_definition);

  py::object generateSchema();

private:
  py::dict schemaForField(const std::string &scope, const Embag::RosMsgTypes::ros_msg_field &field) const;

  std::string outer_scope_;
  std::shared_ptr<Embag::RosMsgTypes::ros_msg_def> msg_def_;
  const py::object ordered_dict_ = py::module::import("collections").attr("OrderedDict");
};
