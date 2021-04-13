#ifndef CPP_SYSTEM_LIBRARY_SHMVERSION_H
#define CPP_SYSTEM_LIBRARY_SHMVERSION_H

#include <ostream>
struct ShmVersion {
  int major = -1;
  int minor = -1;
  int patch = -1;

  bool operator<(const ShmVersion &rhs) const {
    if (major < rhs.major)
      return true;
    if (rhs.major < major)
      return false;
    if (minor < rhs.minor)
      return true;
    if (rhs.minor < minor)
      return false;
    return patch < rhs.patch;
  }
  bool operator>(const ShmVersion &rhs) const { return rhs < *this; }
  bool operator<=(const ShmVersion &rhs) const { return !(rhs < *this); }
  bool operator>=(const ShmVersion &rhs) const { return !(*this < rhs); }

  bool operator==(const ShmVersion &rhs) const {
    return major == rhs.major && minor == rhs.minor && patch == rhs.patch;
  }
  bool operator!=(const ShmVersion &rhs) const { return !(rhs == *this); }

  friend std::ostream &operator<<(std::ostream &os, const ShmVersion &version) {
    os << "major: " << version.major << " minor: " << version.minor
       << " patch: " << version.patch;
    return os;
  }
};

#endif // CPP_SYSTEM_LIBRARY_SHMVERSION_H
