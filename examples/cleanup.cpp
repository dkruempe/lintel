#include <boost/interprocess/sync/named_upgradable_mutex.hpp>

int main(int argc, char *argv[]) {
    // shm_property_mutex_upgradable
    for (int i = 1; i < argc; i++) {
        char *temp = argv[i];
        boost::interprocess::named_upgradable_mutex::remove(temp);
    }
    return 0;
}