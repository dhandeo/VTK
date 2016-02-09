# OpenSlide required
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(OpenSlide REQUIRED)
find_package(TIFF 4.0 REQUIRED)

mark_as_advanced(OPENSLIDE_INCLUDE_DIR OPENSLIDE_LIBRARY)

set(OPENSLIDE_SRCS vtkOpenSlideReader.cxx)
set(PTIF_SRCS vtkPTIFWriter.cxx)

include_directories(${OPENSLIDE_INCLUDE_DIRS})
set_source_files_properties(vtkOPENSLIDE WRAP_EXCLUDE)

vtk_module_library(vtkDomainsMicroscopy ${OPENSLIDE_SRCS} ${PTIF_SRCS})

vtk_module_link_libraries(vtkDomainsMicroscopy LINK_PRIVATE ${OPENSLIDE_LIBRARY})

find_package(TIFF 4.0 REQUIRED)
