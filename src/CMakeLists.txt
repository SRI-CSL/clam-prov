add_definitions(-D__STDC_CONSTANT_MACROS)
add_definitions(-D__STDC_LIMIT_MACROS)

set(LLVM_LINK_COMPONENTS 
  irreader 
  bitwriter 
  ipo 
  scalaropts 
  instrumentation
  transformutils
  core 
  codegen 
  objcarcopts)


add_llvm_executable(clam-prov DISABLE_LLVM_LINK_LLVM_DYLIB
  clam-prov.cpp
  Instrumentation/AnnotateSources.cpp
  Instrumentation/WrapSinks.cpp  
  Instrumentation/OutputResults.cpp
  Instrumentation/ProvMetadata.cpp
  Instrumentation/AddMetadata.cpp
  Instrumentation/OutputDependencyMap.cpp
  Instrumentation/AddLogging.cpp
  Util/SourcesAndSinks.cpp
  Util/DummyMainFunction.cpp
  )

target_link_libraries (clam-prov PRIVATE
  ${LLVM_SEAHORN_LIBS}
  ${SEA_DSA_BS}
  ${CLAM_LIBS}
  )

llvm_config (clam-prov ${LLVM_LINK_COMPONENTS})
install(TARGETS clam-prov RUNTIME DESTINATION bin)
install(FILES sources-sinks.config DESTINATION config)

# if(UNIX AND NOT APPLE)
# ## Logger shared library
# add_library(clamprovlogger SHARED Logging/clam-prov-logger.c)
# set_target_properties(clamprovlogger PROPERTIES
#   VERSION 1
#   SOVERSION 1
#   PUBLIC_HEADER Logging/clam-prov-logger.h)
# target_include_directories(clamprovlogger PRIVATE Logging/clam-prov-logger.h)
# install(TARGETS clamprovlogger
#   LIBRARY DESTINATION lib
#   PUBLIC_HEADER DESTINATION include)
# endif()
