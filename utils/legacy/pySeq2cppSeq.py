#!/usr/bin/env python

# This script converts a python seq file generated by the PG into the JSON format read by the C++ part

from mc_rbdyn import GripperSurface
from mc_rbdyn.robot import loadRobot, Robots
from mc_rbdyn.stance import loadStances, IdentityContactAction, AddContactAction, RemoveContactAction
import spacevecalg as sva
import json
import os
import sys

import rospkg

from stability import StabilityPolygon, Mode
from shapes import PolygonInterpolator
from mc_ros_utils.polygons import contact_points_normals, stab_contacts
from eigen3 import toNumpy
import numpy as np
import rbdyn as rbd

def usage():
  print "{} [.seq file]".format(sys.argv[0])

def svapt2dict(pt):
  pt_out = {}
  pt_out["rotation"] = []
  rot = pt.rotation()
  for i in xrange(3):
    for j in xrange(3):
      pt_out["rotation"].append(rot.coeff(i,j))
  t = pt.translation()
  pt_out["translation"] = []
  for i in xrange(3):
    pt_out["translation"].append(t[i])
  return pt_out

def surface2dict(s):
  s_out = {}
  s_out["name"] = s.name
  s_out["bodyName"] = s.bodyName;
  s_out["X_b_s"] = svapt2dict(s.X_b_s)
  s_out["materialName"] = s.materialName
  if hasattr(s, "planarPoints"):
    s_out["planarPoints"] = []
    for px,py in s.planarPoints:
      p = {'x': px, 'y': py}
      s_out["planarPoints"].append(p)
  if hasattr(s, "radius"):
    s_out["radius"] = s.radius
    s_out["width"] = s.width
  if hasattr(s, "pointsFromOrigin"):
    s_out["pointsFromOrigin"] = []
    for p in s.pointsFromOrigin:
      s_out["pointsFromOrigin"].append(svapt2dict(p))
    s_out["X_b_motor"] = svapt2dict(s.X_b_motor)
    s_out["motorMaxTorque"] = s.motorMaxTorque
  return s_out

def contact2dict(c):
  c_out = {}
  c_out["robotSurface"] = surface2dict(c.robotSurface)
  c_out["envSurface"] = surface2dict(c.envSurface)
  c_out["is_fixed"] = c.isFixed()
  if c.isFixed():
    c_out["X_es_rs"] = svapt2dict(c.X_es_rs)
  else:
    c_out["X_es_rs"] = svapt2dict(sva.PTransform.identity())
  return c_out

def action2dict(a):
  a_out = {}
  if isinstance(a, IdentityContactAction):
    a_out["type"] = "Identity"
  if isinstance(a, AddContactAction):
    a_out["type"] = "Add"
    a_out["contact"] = contact2dict(a.contact)
  if isinstance(a, RemoveContactAction):
    a_out["type"] = "Remove"
    a_out["contact"] = contact2dict(a.contact)
  return a_out

def stance2dict(s):
  s_out = {}
  s_out["q"] = s.q
  s_out["geomContacts"] = []
  for c in s.geomContacts:
    s_out["geomContacts"].append(contact2dict(c))
  s_out["stabContacts"] = []
  for c in s.stabContacts:
    s_out["stabContacts"].append(contact2dict(c))
  return s_out

def tuplePair2dict(p1, p2):
  p_out = { "p1" : [0,0], "p2" : [0,0] }
  p_out["p1"][0] = p1[0]
  p_out["p1"][1] = p1[1]
  p_out["p2"][0] = p2[0]
  p_out["p2"][1] = p2[1]
  return p_out

def polygonInterpolator2dict(p):
  p_out = {}
  p_out["tuple_pairs"] = []
  for p1, p2 in p.tuple_pairs:
    p_out["tuple_pairs"].append(tuplePair2dict(p1, p2))
  return p_out

if __name__ == "__main__":
  if len(sys.argv) < 2:
    usage()
    sys.exit(1)

  seq_in = sys.argv[1]
  robot = loadRobot('hrp2_drc', rospkg.RosPack().get_path('hrp2_drc_description') + '/rsdf/hrp2_drc')
  robot_mass = sum([l.inertia().mass() for l in robot.mb.bodies()])
  stab_poly = StabilityPolygon(robot_mass, dimension = 2)


  seq_out = os.getcwd() + '/' + os.path.basename(seq_in.replace('.seq', '.json'))
  data_out = {"stances" : [], "actions": [], "polygon_interpolators": []}

  stances, actions = loadStances(seq_in)
  for s in stances:
    data_out["stances"].append(stance2dict(s))
  for a in actions:
    data_out["actions"].append(action2dict(a))
  polygons = []
  polygon_interpolators = []
  for i,(s,a) in enumerate(zip(stances,actions)):
    print "Compute polygon {0} of {1}".format(i+1, len(stances))
    stab_poly.reset()
    robot.mbc.zero(robot.mb)
    robot.mbc.q = s.q
    rbd.forwardKinematics(robot.mb, robot.mbc)
    rbd.forwardVelocity(robot.mb, robot.mbc)
    if isinstance(a, AddContactAction):
        contacts = s.stabContacts
    else:
        contacts = s.contacts
    #contacts = s.stabContacts
    for c in contacts:
        if c.robotSurface.name == "LFrontSole":
            c.robotSurface = robot.surfaces["LFullSole"]
        if c.robotSurface.name == "RFrontSole":
            c.robotSurface = robot.surfaces["RFullSole"]
    offset, s_c = stab_contacts(contacts, 0.5, robot)
    if i == 1:
      for c in s_c:
          c.r = c.r*0.8
    stab_poly.contacts = s_c
    gripIndexes = []
    planarContacts = []
    begin = 0
    for c in contacts:
      nrPoints = len(c.points())
      if isinstance(c.robotSurface, GripperSurface):
        bodyIndex = robot.bodyIndexByName(c.robotSurface.bodyName)
        gripIndexes.append((begin,  begin + nrPoints, bodyIndex))
      else:
        planarContacts.append(c)
      begin += nrPoints

    for b, e, bodyIndex in gripIndexes:
      pos = toNumpy(list(robot.mbc.bodyPosW)[bodyIndex].translation()) - offset
      stab_poly.addTorqueConstraint(s_c[b:e],
                                    pos,
                                    5*np.ones((3,1)))
    stab_poly.compute(Mode.best, epsilon=1e-2, maxIter=100, solver='plain', record_anim=False, plot_step=False, plot_final = False)
    full_poly = stab_poly.polygon(offset)
    if len(polygons) == 0:
        polygons.append(full_poly)

    stab_poly.reset()
    print planarContacts
    offset, s_c = stab_contacts(planarContacts, 0.5, robot)
    stab_poly.contacts = s_c
    try:
      stab_poly.compute(Mode.best, epsilon=1e-2, maxIter=100, solver='plain', record_anim=False, plot_step=False, plot_final = False)
      s_poly = stab_poly.polygon(offset)
    except RuntimeError:
      pos = [c.r + offset for c in s_c]
      s_poly = geom.Polygon([(p.item(0), p.item(1)) for p in pos]).convex_hull
    interp = PolygonInterpolator(full_poly, s_poly)
    # FIXME Ok on this branch...
    #if i == 25:
    #    intermediate_poly = interp.interpolate(0.5)
    #elif i < 8:
    #    intermediate_poly = interp.interpolate(0.9)
    #else:
    #    intermediate_poly = interp.interpolate(0.7)
    intermediate_poly = interp.interpolate(0.9)
    polygon_interpolators.append(PolygonInterpolator(polygons[-1], intermediate_poly))
    polygons.append(intermediate_poly)

  for pi in polygon_interpolators:
    data_out["polygon_interpolators"].append(polygonInterpolator2dict(pi))

  with open(seq_out, 'w') as f_out:
    json.dump(data_out, f_out)

  print "Saved {} stances and {} actions into json file ({})".format(len(stances), len(actions), seq_out)