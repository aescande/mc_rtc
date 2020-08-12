/*
 * Copyright 2015-2020 CNRS-UM LIRMM, CNRS-AIST JRL
 */

#include <mc_tvm/JointsSelectorFunction.h>

#include <mc_rbdyn/Robot.h>

namespace mc_tvm
{

std::unique_ptr<JointsSelectorFunction> JointsSelectorFunction::ActiveJoints(
    tvm::FunctionPtr f,
    const mc_rbdyn::Robot & robot,
    const std::vector<std::string> & activeJoints)
{
  auto useRobotVariable = [&](const tvm::VariablePtr & v) {
    return std::find(f->variables().begin(), f->variables().end(), v) != f->variables().end();
  };
  if(!useRobotVariable(robot.qJoints()) || !useRobotVariable(robot.qFloatingBase()))
  {
    mc_rtc::log::error_and_throw<std::runtime_error>(
        "Cannot setup joint selector on the provided function as it doesn't use variables of {}", robot.name());
  }

  const auto & mb = robot.mb();

  std::vector<std::string> joints = activeJoints;
  std::sort(joints.begin(), joints.end(), [&mb](const std::string & lhs, const std::string & rhs) {
    return mb.jointPosInDof(mb.jointIndexByName(lhs)) < mb.jointPosInDof(mb.jointIndexByName(rhs));
  });

  bool ffActive = joints.size() > 0 && joints[0] == mb.joint(0).name() && mb.joint(0).dof() == 6;
  if(ffActive)
  {
    joints.erase(joints.begin());
  }

  Eigen::DenseIndex ffSize = mb.joint(0).dof() == 6 ? 6 : 0;
  std::vector<std::pair<Eigen::DenseIndex, Eigen::DenseIndex>> activeIndex;
  Eigen::DenseIndex start = 0;
  Eigen::DenseIndex size = 0;
  for(const auto & j : joints)
  {
    auto jIndex = mb.jointIndexByName(j);
    auto jDof = mb.joint(jIndex).dof();
    Eigen::DenseIndex pos = mb.jointPosInDof(jIndex) - ffSize;
    if(pos != start + size)
    {
      if(size != 0)
      {
        activeIndex.push_back({start, size});
      }
      start = pos;
      size = jDof;
    }
    else
    {
      size += jDof;
    }
  }
  if(size != 0)
  {
    activeIndex.push_back({start, size});
  }

  return std::unique_ptr<JointsSelectorFunction>(new JointsSelectorFunction(f, robot, ffActive, activeIndex));
}

std::unique_ptr<JointsSelectorFunction> JointsSelectorFunction::InactiveJoints(
    tvm::FunctionPtr f,
    const mc_rbdyn::Robot & robot,
    const std::vector<std::string> & inactiveJoints)
{
  std::vector<std::string> activeJoints{};
  for(const auto & j : robot.mb().joints())
  {
    if(std::find_if(inactiveJoints.begin(), inactiveJoints.end(), [&j](const std::string & s) { return s == j.name(); })
       == inactiveJoints.end())
    {
      activeJoints.push_back(j.name());
    }
  }
  return ActiveJoints(f, robot, activeJoints);
}

JointsSelectorFunction::JointsSelectorFunction(
    tvm::FunctionPtr f,
    const mc_rbdyn::Robot & robot,
    bool ffActive,
    const std::vector<std::pair<Eigen::DenseIndex, Eigen::DenseIndex>> & activeIndex)
: tvm::function::abstract::Function(f->imageSpace()), f_(f), robot_(robot), fbActive_(ffActive),
  activeIndex_(activeIndex)
{
  addDirectDependency<JointsSelectorFunction>(Output::Value, *f_, Function::Output::Value);
  addDirectDependency<JointsSelectorFunction>(Output::Velocity, *f_, Function::Output::Velocity);
  addDirectDependency<JointsSelectorFunction>(Output::NormalAcceleration, *f_, Function::Output::NormalAcceleration);

  registerUpdates(Update::Jacobian, &JointsSelectorFunction::updateJacobian);
  registerUpdates(Update::JDot, &JointsSelectorFunction::updateJDot);

  addOutputDependency<JointsSelectorFunction>(Output::Jacobian, Update::Jacobian);
  addOutputDependency<JointsSelectorFunction>(Output::JDot, Update::JDot);

  addInputDependency<JointsSelectorFunction>(Update::Jacobian, *f_, Function::Output::Jacobian);
  addInputDependency<JointsSelectorFunction>(Update::JDot, *f_, Function::Output::JDot);

  if(fbActive_)
  {
    addVariable(robot_->qFloatingBase(), f_->linearIn(*robot_->qFloatingBase()));
    jacobian_.at(robot_->qFloatingBase().get()).setZero();
  }
  if(activeIndex.size())
  {
    addVariable(robot_->qJoints(), f_->linearIn(*robot_->qJoints()));
    jacobian_.at(robot_->qJoints().get()).setZero();
  }
}

void JointsSelectorFunction::updateJacobian()
{
  if(fbActive_)
  {
    jacobian_[robot_->qFloatingBase().get()] = f_->jacobian(*robot_->qFloatingBase());
  }
  if(activeIndex_.size())
  {
    const auto & jacIn = f_->jacobian(*robot_->qJoints());
    for(const auto & p : activeIndex_)
    {
      jacobian_[robot_->qJoints().get()].middleCols(p.first, p.second) =
          jacIn.block(0, p.first, jacIn.rows(), p.second);
    }
  }
}

void JointsSelectorFunction::updateJDot()
{
  if(fbActive_)
  {
    JDot_[robot_->qFloatingBase().get()] = f_->JDot(*robot_->qFloatingBase());
  }
  if(activeIndex_.size())
  {
    const auto & JDotIn = f_->JDot(*robot_->qJoints());
    for(const auto & p : activeIndex_)
    {
      JDot_[robot_->qJoints().get()].middleCols(p.first, p.second) = JDotIn.block(0, p.first, JDotIn.rows(), p.second);
    }
  }
}

} // namespace mc_tvm