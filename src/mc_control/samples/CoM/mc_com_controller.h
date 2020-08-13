/*
 * Copyright 2015-2020 CNRS-UM LIRMM, CNRS-AIST JRL
 */

#pragma once

#include <mc_control/api.h>
#include <mc_control/mc_controller.h>

#include <mc_tasks/CoMTask.h>

namespace mc_control
{

struct MC_CONTROL_DLLAPI MCCoMController : public MCController
{
  MCCoMController(std::shared_ptr<mc_rbdyn::RobotModule> robot, double dt);

  bool run() override;

  void reset(const ControllerResetData & reset_data) override;

private:
  mc_tasks::CoMTaskPtr comTask_;
  std::string leftFootSurface_, rightFootSurface_;
};

} // namespace mc_control

SIMPLE_CONTROLLER_CONSTRUCTOR("CoM", mc_control::MCCoMController)
