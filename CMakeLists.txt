cmake_minimum_required(VERSION 3.2)

project(libperfhalide)

add_library(perfhalide SHARED perf.c)

#install(TARGETS libperfhalide
#  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
