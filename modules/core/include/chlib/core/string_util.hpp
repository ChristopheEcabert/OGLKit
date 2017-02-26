/**
 *  @file   string_util.hpp
 *  @brief  Utility function for string handling
 *
 *  @author Christophe Ecabert
 *  @date   26/08/16
 *  Copyright © 2016 Christophe Ecabert. All rights reserved.
 */

#ifndef __CHLIB_STRING_UTIL__
#define __CHLIB_STRING_UTIL__

#include <string>
#include <vector>

#include "chlib/core/library_export.hpp"

/**
 *  @namespace  CHLib
 *  @brief      Chris dev space
 */
namespace CHLib {
  
/**
 *  @class  StringUtil
 *  @brief  Utility function for string handling
 *  @author Christophe Ecabert
 *  @date   26/08/16
 */
class CHLIB_EXPORTS StringUtil {
 public:
  
  /**
   *  @name Split
   *  @fn static void Split(const std::string& string,
                           const std::string delimiter,
                           std::vector<std::string>* parts);
   *  @brief  Split a given \p string into parts for a specific delimiter
   *  @param[in]  string    String to split
   *  @param[in]  delimiter Delimiter
   *  @param[out] parts     Splitted parts
   */
  static void Split(const std::string& string,
                    const std::string delimiter,
                    std::vector<std::string>* parts);
  
  /**
   *  @name ExtractDirectory
   *  @fn
   *  @brief  Split path into directory + extension
   *  @param[in]  path  Path where to extract data
   *  @param[out] dir   Extracted directory
   *  @param[out] file  Extracted filename
   *  @param[out] ext   Extracted extension
   */
  static void ExtractDirectory(const std::string& path,
                               std::string* dir,
                               std::string* file,
                               std::string* ext);
};
  
}  // namespace CHLib


#endif /* __CHLIB_STRING_UTIL__ */