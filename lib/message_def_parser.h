#pragma once

#include "ros_msg_types.h"

namespace Embag {

std::shared_ptr<RosMsgTypes::MsgDef> parseMsgDef(const std::string &def, const std::string& name);

}
