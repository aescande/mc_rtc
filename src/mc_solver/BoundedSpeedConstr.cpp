#include <mc_solver/BoundedSpeedConstr.h>

#include <mc_rtc/logging.h>

namespace mc_solver
{

BoundedSpeedConstr::BoundedSpeedConstr(const mc_rbdyn::Robots & robots, unsigned int robotIndex, double dt)
: constr(new tasks::qp::BoundedSpeedConstr(robots.mbs(), robotIndex, dt)),
  robotIndex(robotIndex)
{
}

void BoundedSpeedConstr::addToSolver(const std::vector<rbd::MultiBody> & mbs, tasks::qp::QPSolver & solver) const
{
  constr->addToSolver(mbs, solver);
}

void BoundedSpeedConstr::removeFromSolver(tasks::qp::QPSolver & solver) const
{
  constr->removeFromSolver(solver);
}

void BoundedSpeedConstr::addBoundedSpeed(QPSolver & solver, const std::string & bodyName, const Eigen::Vector3d & bodyPoint, const Eigen::MatrixXd & dof, const Eigen::VectorXd & speed)
{
  addBoundedSpeed(solver, bodyName, bodyPoint, dof, speed, speed);
}

void BoundedSpeedConstr::addBoundedSpeed(QPSolver & solver, const std::string & bodyName, const Eigen::Vector3d & bodyPoint, const Eigen::MatrixXd & dof, const Eigen::VectorXd & lowerSpeed, const Eigen::VectorXd & upperSpeed)
{
  int id = solver.robots().robot(robotIndex).bodyIdByName(bodyName);
  if(id >= 0)
  {
    constr->addBoundedSpeed(solver.robots().mbs(), id, bodyPoint, dof, lowerSpeed, upperSpeed);
    constr->updateBoundedSpeeds();
    solver.updateConstrSize();
  }
  else
  {
    LOG_ERROR("Could not add bounded speed constraint for body " << bodyName << " since it does not exist in robot " << solver.robots().robot(robotIndex).name())
  }
}

void BoundedSpeedConstr::removeBoundedSpeed(QPSolver & solver, const std::string & bodyName)
{
  int id = solver.robots().robot(robotIndex).bodyIdByName(bodyName);
  if(id >= 0)
  {
    constr->removeBoundedSpeed(id);
    constr->updateBoundedSpeeds();
    solver.updateConstrSize();
  }
  else
  {
    LOG_ERROR("Could not remove bounded speed constraint for body " << bodyName << " since it does not exist in robot " << solver.robots().robot(robotIndex).name())
  }
}

void BoundedSpeedConstr::reset(QPSolver & solver)
{
  constr->resetBoundedSpeeds();
  constr->updateBoundedSpeeds();
  solver.updateConstrSize();
}

}
