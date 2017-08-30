#include "mc_text_controller.h"

#include <mc_rbdyn/configuration_io.h>
#include <mc_solver/ConstraintSetLoader.h>
#include <mc_tasks/MetaTaskLoader.h>

#include <mc_rtc/logging.h>

namespace mc_control
{

MCTextController::MCTextController(std::shared_ptr<mc_rbdyn::RobotModule> robot,
                                   double dt,
                                   const mc_rtc::Configuration & config)
: MCController(robot, dt)
{
  if(!config.has("Text"))
  {
    LOG_ERROR_AND_THROW(std::runtime_error, "No entries relative to the Text controller in the loaded configuration")
  }
  config_ = config("Text");
}

void MCTextController::reset(const mc_control::ControllerResetData & data)
{
  mc_control::MCController::reset(data);
  if(!config_.has("constraints"))
  {
    LOG_ERROR_AND_THROW(std::runtime_error, "No constraints in the provided text file")
  }
  if(!config_.has("tasks"))
  {
    LOG_ERROR_AND_THROW(std::runtime_error, "No tasks in the provided text file");
  }
  for(const auto & c : config_("constraints"))
  {
    constraints_.push_back(mc_solver::ConstraintSetLoader::load(solver(), c));
    solver().addConstraintSet(*constraints_.back());
  }
  solver().addTask(postureTask);
  for(const auto & t : config_("tasks"))
  {
    tasks_.push_back(mc_tasks::MetaTaskLoader::load(solver(), t));
    solver().addTask(tasks_.back());
  }
  std::vector<mc_rbdyn::Contact> contacts = {};
  if(config_.has("contacts"))
  {
    for(const auto & c : config_("contacts"))
    {
      contacts.emplace_back(mc_rtc::ConfigurationLoader<mc_rbdyn::Contact>::load(robots(), c));
    }
  }
  solver().setContacts(contacts);
}

}
