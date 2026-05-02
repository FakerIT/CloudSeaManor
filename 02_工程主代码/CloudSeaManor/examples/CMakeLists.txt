# NVIDIA Image Generation Examples
# This file contains example code for using the NvidiaImageGenerator

add_executable(NvidiaImageExamples
    examples/NvidiaImageIntegrationExamples.cpp
)

target_include_directories(NvidiaImageExamples
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(NvidiaImageExamples
    PRIVATE
        CloudSeamanor_core
        ${CURL_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        SFML::Graphics
        SFML::Window
        SFML::System
)

set_target_properties(NvidiaImageExamples PROPERTIES FOLDER "Examples/NvidiaImage")

# ============================================================================
# Game Integration Examples
# ============================================================================
add_executable(GameImageIntegration
    examples/GameImageGenerationIntegration.cpp
)

target_include_directories(GameImageIntegration
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(GameImageIntegration
    PRIVATE
        CloudSeamanor_core
        ${CURL_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        SFML::Graphics
        SFML::Window
        SFML::System
)

set_target_properties(GameImageIntegration PROPERTIES FOLDER "Examples/GameIntegration")