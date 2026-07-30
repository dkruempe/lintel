#ifndef CPP_BASE_LIBRARY_PERSISTABLEBEAN_H
#define CPP_BASE_LIBRARY_PERSISTABLEBEAN_H

/** Interface for beans that are notified when persistence data is loaded. */
class PersistableBean {
public:
    /** Called when the bean should reload or refresh its state from persistent storage. */
    virtual void onAwake() = 0;
};

#endif  // CPP_BASE_LIBRARY_PERSISTABLEBEAN_H
