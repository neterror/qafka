cmake_minimum_required(VERSION 3.20)
project(qafka VERSION 1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_MAKEFILE OFF)
set(CMAKE_BUILD_TYPE Debug)

find_package(Qt6 CONFIG REQUIRED Core)
qt_standard_project_setup()

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

add_library(qafka
  src/producer.cpp
  src/consumer.cpp
  src/kafka_client.cpp

  include/consumer.h
  include/producer.h
  include/kafka_client.h
  include/thread.h
)

target_include_directories(qafka PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include ${KAFKA_INCLUDE_DIRS})

include(GenerateExportHeader)
generate_export_header(qafka)

target_sources(qafka
  PUBLIC
  FILE_SET HEADERS
  BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}
  FILES ${CMAKE_CURRENT_BINARY_DIR}/qafka_export.h
)



find_package(PkgConfig REQUIRED)
pkg_check_modules(KAFKA REQUIRED rdkafka++)


target_link_libraries(qafka PUBLIC Qt6::Core ${KAFKA_LIBRARIES})

install(TARGETS qafka  LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include/qafka)
install(FILES ${CMAKE_BINARY_DIR}/qafka_export.h DESTINATION include/qafka)
