#pragma once

#include "ros_msg_types.h"

namespace Embag {

std::shared_ptr<RosMsgTypes::ros_msg_def> parseMsgDef(const std::string &def);

}