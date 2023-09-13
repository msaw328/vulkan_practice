add_executable(vulkan-playground
            main.c
            xcb_wrap.c
            vk_wrap.c
            )

target_link_libraries(vulkan-playground
            ${Vulkan_LIBRARIES}
            ${XCB_LIBRARIES}
            )

target_include_directories(vulkan-playground
            PRIVATE ${Vulkan_INCLUDE_DIRS}
            PRIVATE ${XCB_INCLUDE_DIRS}
            )
