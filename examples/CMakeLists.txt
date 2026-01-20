add_executable(sim_logger_example_cpp
  example_cpp.cpp
)
target_link_libraries(sim_logger_example_cpp
  PRIVATE
    sim_logger::core
)

add_executable(sim_logger_example_async_cpp
  example_async_cpp.cpp
)
target_link_libraries(sim_logger_example_async_cpp
  PRIVATE
    sim_logger::core
)

add_executable(sim_logger_example_c
  example_c.c
)
target_link_libraries(sim_logger_example_c
  PRIVATE
    sim_logger::c_api
)
