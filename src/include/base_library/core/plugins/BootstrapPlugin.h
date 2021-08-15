#ifndef CPP_BASE_LIBRARY_BOOTSTRAPPLUGIN_H
#define CPP_BASE_LIBRARY_BOOTSTRAPPLUGIN_H

#include "base_library/core/models/BootstrapSequence.h"

class BootstrapPlugin {
 public:
  BootstrapPlugin() = default;
  virtual ~BootstrapPlugin() = default;
  virtual void onStart() = 0;
  virtual BootstrapSequence getPriority() = 0;
};

#endif  // CPP_BASE_LIBRARY_BOOTSTRAPPLUGIN_H
