#pragma once

#include <mc_solver/api.h>
#include <mc_solver/qpsolver.h>

#include <mc_rbdyn/polygon_utils.h>

#include <Tasks/QPConstr.h>

namespace mc_solver
{

/** \class CoMIncPlaneConstr
 * \brief Wrapper around tasks::qp::CoMIncPlaneConstr
 */
struct MC_SOLVER_DLLAPI CoMIncPlaneConstr : public ConstraintSet
{
public:
  CoMIncPlaneConstr(const mc_rbdyn::Robots & robots, unsigned int robotIndex, double dt);

  virtual void addToSolver(const std::vector<rbd::MultiBody> & mbs, tasks::qp::QPSolver & solver) const override;

  virtual void removeFromSolver(tasks::qp::QPSolver & solver) const override;

  void set_planes(QPSolver & solver, const std::vector<mc_rbdyn::Plane> & planes, const std::vector<Eigen::Vector3d> & speeds = {}, const std::vector<Eigen::Vector3d> & normalsDots = {});
private:
  std::shared_ptr<tasks::qp::CoMIncPlaneConstr> constr;
};

}
