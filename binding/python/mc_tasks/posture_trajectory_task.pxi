cdef class _PostureTrajectoryTask(MetaTask):
  def __cinit__(self):
    pass
  def stiffness(self, stiff = None):
    assert(self.ttg_base.get())
    if stiff is None:
      return deref(self.ttg_base).stiffness()
    else:
      deref(self.ttg_base).stiffness(stiff)
  def setGains(self, stiffness, damping):
    assert(self.ttg_base.get())
    deref(self.ttg_base).setGains(stiffness, damping)
  def damping(self):
    assert(self.ttg_base.get())
    return deref(self.ttg_base).damping()
  def weight(self, w = None):
    assert(self.ttg_base.get())
    if w is None:
      return deref(self.ttg_base).weight()
    else:
      deref(self.ttg_base).weight(w)
