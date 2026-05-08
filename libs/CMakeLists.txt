add_library(libs INTERFACE)

#flecs

add_subdirectory(flecs)
target_link_libraries(libs INTERFACE flecs::flecs_static)


#box2d
set(BOX2D_SAMPLES OFF CACHE BOOL "Disable box2d samples")
set(BOX2D_AVX2 ON CACHE BOOL "Enable AVX2 for box2d")
add_subdirectory(box2d)
target_link_libraries(libs INTERFACE box2d)

#box2d
add_subdirectory(json)
target_link_libraries(libs INTERFACE nlohmann_json::nlohmann_json)

#glm
#add_subdirectory(glm)
#target_link_libraries(libs INTERFACE glm)

#bixxy-engine

set(ADD_IMGUI ${ADD_IMGUI} CACHE BOOL "Enable ImGui support" FORCE)

add_subdirectory(luminoveau)
target_link_libraries(libs INTERFACE luminoveau)
target_include_directories(libs INTERFACE luminoveau)