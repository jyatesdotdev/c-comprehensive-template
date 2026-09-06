# cmake/Install.cmake — install/export static libraries for find_package.
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(_install_targets core cli systems simulation rendering_sw math ml containers)
if(ENABLE_HPC AND TARGET hpc)
    list(APPEND _install_targets hpc)
endif()
if(ENABLE_NETWORKING AND TARGET networking)
    list(APPEND _install_targets networking)
endif()
if(ENABLE_RENDERING AND TARGET rendering)
    list(APPEND _install_targets rendering)
endif()

install(TARGETS ${_install_targets}
    EXPORT c_comprehensive_templateTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT c_comprehensive_templateTargets
    FILE c_comprehensive_templateTargets.cmake
    NAMESPACE c_comprehensive_template::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/c_comprehensive_template
)

set(_config "${CMAKE_CURRENT_BINARY_DIR}/c_comprehensive_templateConfig.cmake")
file(WRITE "${_config}"
"include(CMakeFindDependencyMacro)\n"
)
if(ENABLE_HPC AND TARGET hpc)
    file(APPEND "${_config}" "find_dependency(Threads)\n")
endif()
file(APPEND "${_config}"
"include(\"\${CMAKE_CURRENT_LIST_DIR}/c_comprehensive_templateTargets.cmake\")\n"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/c_comprehensive_templateConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${_config}"
    "${CMAKE_CURRENT_BINARY_DIR}/c_comprehensive_templateConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/c_comprehensive_template
)
