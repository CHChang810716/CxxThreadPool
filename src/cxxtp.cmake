include(cmake/scheme/lib.cmake)
target_sources(${AKT_TARGET} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/cxxtp/Worker.cpp
  ${CMAKE_CURRENT_LIST_DIR}/cxxtp/Scheduler.cpp
  ${CMAKE_CURRENT_LIST_DIR}/cxxtp/ts_queue/CircularQueue.cpp
)
# target_compile_options(${AKT_TARGET} INTERFACE -fcoroutines)