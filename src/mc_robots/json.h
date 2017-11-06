#pragma once

#include <mc_rbdyn/RobotModule.h>
#include <mc_rbdyn_urdf/urdf.h>

#include "api.h"

extern "C"
{
  ROBOT_MODULE_API std::vector<std::string> MC_RTC_ROBOT_MODULE() { return {"json"}; }
  ROBOT_MODULE_API void destroy(mc_rbdyn::RobotModule * ptr) { delete ptr; }
  ROBOT_MODULE_API mc_rbdyn::RobotModule * create(const std::string&,
                                                  const std::string & path);
}