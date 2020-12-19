#ifndef LOGGING_FILEPROPERTYREPOSITORY_H
#define LOGGING_FILEPROPERTYREPOSITORY_H

#include "PropertyRepository.h"
#include "base_library/services/ExecutorService.h"
#include "base_library/services/FileService.h"
#include "base_library/strategies/ConfigSerializationStrategy.h"

/**
 * implementation of file repository for properties
 * This implementation works only for one process, bc. we have to make sure
 * that only one thread access to the file to prevent race conditions.
 * Otherwise we would have to use semaphores to protect the access.
 * we
 */
class FilePropertyRepository : public PropertyRepository {
 public:

  explicit FilePropertyRepository(std::shared_ptr<ConfigSerializationStrategy> configSerializationStrategy);

  /**
   * returns priority of repository
   */
  PropertyRepositoryType getType() override;

  /**
   * returns if save operations are supported or not
   */
  bool isMutable() override;

  /**
   * save property to repository
   * - saves property in file
   * - waits until awake was called
   * - after awake PropertyService will check which properties doesn't have
   *   a configuration and inform the PoropertyRepository
   * - alternative is that the Property is changed via REST Controller
   *   In this case fswatch will notify that the file was changed and the
   *   PropertyService will be infomred about the changed data
   * - in case new property is inserted. The new property will added to the
   *   end of the file.
   */
  void save(std::shared_ptr<PropertyBase> property) override;

  /**
   * save properties to repository
   * @param properties
   */
  void save(
      const std::vector<std::shared_ptr<PropertyBase>> &properties) override;

  /**
   * load all properties from repository and return result
   */
  std::vector<std::shared_ptr<PropertyBase>> awake() override;

 private:
  // members of class
  std::string content = "";
  const std::filesystem::path configurationPath;
  std::shared_ptr<ConfigSerializationStrategy> configSerializationStrategy;
};

#endif  // LOGGING_FILEPROPERTYREPOSITORY_H
