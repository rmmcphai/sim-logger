add_library(sim_logger_c_api
  src/c_api.cpp
)

add_library(sim_logger::c_api ALIAS sim_logger_c_api)

target_include_directories(sim_logger_c_api
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sim_logger_c_api
  PUBLIC
    sim_logger::core
)

target_compile_features(sim_logger_c_api PUBLIC cxx_std_17)

# Export macro for Windows (harmless elsewhere)
target_compile_definitions(sim_logger_c_api PRIVATE SIM_LOGGER_C_API_BUILD=1)
