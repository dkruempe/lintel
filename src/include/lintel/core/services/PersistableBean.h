#ifndef LINTEL_PERSISTABLEBEAN_H
#define LINTEL_PERSISTABLEBEAN_H

namespace lintel {
/** Interface for beans that are notified when persistence data is loaded. */
class PersistableBean
{
public:
  /** Called when the bean should reload or refresh its state from persistent storage. */
  virtual void onAwake() = 0;
};

}// namespace lintel
#endif// LINTEL_PERSISTABLEBEAN_H
