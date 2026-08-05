module;

#include "base_library/StdIncludes.h"

#include "base_library/core/exceptions/ConfigShmSegmentNotFound.h"
#include "base_library/core/exceptions/FileServiceFileExists.h"
#include "base_library/core/exceptions/FileServiceIsNotFileException.h"
#include "base_library/core/exceptions/HttpBadRequestException.h"
#include "base_library/core/exceptions/LoggerServiceNotInitialized.h"
#include "base_library/core/exceptions/SQLException.h"
#include "base_library/core/exceptions/ShmSegmentNotFound.h"

export module base_library.core.exceptions;

export using ::ConfigShmSegmentNotFound;
export using ::FileServiceFileExists;
export using ::FileServiceIsNotFileException;
export using ::HttpBadRequestException;
export using ::LoggerServiceNotInitialized;
export using ::ShmSegmentNotFound;

export namespace db {
using ::db::SQLException;
}
