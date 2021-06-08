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
  Instrumentation/ReadMetadata.cpp
  Instrumentation/AddMetadata.cpp
  Instrumentation/OutputDependencyMap.cpp
  )

target_link_libraries (clam-prov PRIVATE
  ${LLVM_SEAHORN_LIBS}
  ${SEA_DSA_BS}
  ${CLAM_LIBS}
  )

llvm_config (clam-prov ${LLVM_LINK_COMPONENTS})
install(TARGETS clam-prov RUNTIME DESTINATION bin)

