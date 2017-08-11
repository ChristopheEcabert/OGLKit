/**
 *  @file   image_loader.cpp
 *  @brief  Helper class to load images into memory
 *
 *  @author Christophe Ecabert
 *  @date   11.03.17
 *  Copyright © 2017 Christophe Ecabert. All rights reserved.
 */

#include "oglkit/io/image_factory.hpp"

/**
 *  @namespace  OGLKit
 *  @brief      Chris dev space
 */
namespace OGLKit {
  
#pragma mark
#pragma mark Initialization
  
/*
 *  @name Get
 *  @fn static ImageFactory& Get(void)
 *  @brief  Provide access to single instance of ImageFactory
 */
ImageFactory& ImageFactory::Get(void) {
  static ImageFactory factory;
  return factory;
}
  
/*
 *  @name   ImageFactory
 *  @fn     ImageFactory(void)
 *  @brief  Constructor
 */
ImageFactory::ImageFactory(void) {}
  
/*
 *  @name   ~ImageFactory
 *  @fn     ~ImageFactory(void)
 *  @brief  Destructor
 */
ImageFactory::~ImageFactory(void) {
}
  
#pragma mark
#pragma mark Usage
  
/*
 *  @name CreateByExtension
 *  @fn Image* CreateByExtension(const std::string& extension)
 *  @brief  Create an image based on the extension type
 *  @param[in]  extension  Image extension (type)
 *  @return Pointer to an instance of the given type, nullptr if type is
 *  unknown
 */
Image* ImageFactory::CreateByExtension(const std::string& extension) {
  Image* img = nullptr;
  for (const auto* proxy : proxies_) {
    if (proxy->Extension() == extension) {
      img = proxy->Create();
      break;
    }
  }
  return img;
}

/*
 *  @name Register
 *  @fn void Register(const ImageProxy* object)
 *  @brief  Register a type of image with a given proxy.
 *  @param[in]  object  Object to register
 */
void ImageFactory::Register(const ImageProxy* object) {
  // Alreay registered ?
  auto it = proxies_.find(object);
  if (it == proxies_.end()) {
    proxies_.insert(object);
  }
}
  
}  // namespace OGLKit
