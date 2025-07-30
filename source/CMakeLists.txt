add_library(flux STATIC flux.cc)
target_include_directories(flux PUBLIC ${FLUX_ROOT}/include)
target_link_libraries(flux PUBLIC fmt::fmt)
