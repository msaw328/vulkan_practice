#define VK_USE_PLATFORM_XCB_KHR
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include <vulkan/vulkan.h>

#include "xcb_wrap.h"

#define _DIE(msg) do { puts(msg); exit(0); } while(0);

uint8_t* load_spirv_from_file(char* filename, size_t* shader_size) {
    struct stat stat_data = { 0 };
    int res = stat(filename, &stat_data);

    if(res != 0) {
        printf("ERROR: Cannot open shader file %s\n", filename);
        exit(0);
    }

    uint8_t* data_buff = malloc(stat_data.st_size);
    FILE* f = fopen(filename, "r");
    fread(data_buff, 1, stat_data.st_size, f);
    fclose(f);

    *shader_size = stat_data.st_size;

    return data_buff;
}

void vk_wrap_main(xcb_wrap_ctx_t* xcb_ctx) {
    VkResult res;
    VkInstance instance;
    const char* instance_layers[1] = { "VK_LAYER_KHRONOS_validation" };
    const char* instance_extensions[2] = { "VK_KHR_surface", "VK_KHR_xcb_surface" };
    const VkInstanceCreateInfo instance_createinfo = {
        .sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                      = NULL,
        .flags                      = 0,
        .pApplicationInfo           = NULL,
        .enabledLayerCount          = 1,
        .ppEnabledLayerNames        = instance_layers,
        .enabledExtensionCount      = 2,
        .ppEnabledExtensionNames    = instance_extensions
    };

    res = vkCreateInstance(
        &instance_createinfo,
        NULL,
        &instance
    );

    if(res != VK_SUCCESS) _DIE("Cant create instance");

    // Instance initialized, get devices

    uint32_t phydev_count = 0;
    res = vkEnumeratePhysicalDevices(
        instance,
        &phydev_count,
        NULL
    );

    if(res != VK_SUCCESS) _DIE("Cant count physical devices");

    VkPhysicalDevice phydev_list[phydev_count];

    res = vkEnumeratePhysicalDevices(
        instance,
        &phydev_count,
        phydev_list
    );

    if(res != VK_SUCCESS) _DIE("Cant list physical devices");

    // Create vulkan surface based on the xcb window
    VkSurfaceKHR vk_surface;
    const VkXcbSurfaceCreateInfoKHR surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,

        // below stuff from xcb_wrap
        .connection = xcb_ctx->conn,
        .window = xcb_ctx->window
    };

    res = vkCreateXcbSurfaceKHR(
        instance,
        &surface_create_info,
        NULL,
        &vk_surface
    );

    if(res != VK_SUCCESS) _DIE("Cant creature surface");

    puts("Created vulkan surface from XCB window");

    // Physical devices listed, now print info about each
    puts("=== Physical devices props === BEGIN");

    for(uint32_t i = 0; i < phydev_count; i++) {
        VkPhysicalDeviceProperties phydev_prop;
        vkGetPhysicalDeviceProperties(
            phydev_list[i],
            &phydev_prop
        );

        char* phydev_type_string = "VK_PHYSICAL_DEVICE_TYPE_OTHER";
        switch(phydev_prop.deviceType) {
            default:
            case VK_PHYSICAL_DEVICE_TYPE_OTHER: {
                phydev_type_string = "VK_PHYSICAL_DEVICE_TYPE_OTHER";
                break;
            }

            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: {
                phydev_type_string = "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
                break;
            }

            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: {
                phydev_type_string = "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
                break;
            }

            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: {
                phydev_type_string = "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
                break;
            }

            case VK_PHYSICAL_DEVICE_TYPE_CPU: {
                phydev_type_string = "VK_PHYSICAL_DEVICE_TYPE_CPU";
                break;
            }
        }

        printf("phydev[%2u] = { .deviceName = \"%s\", .deviceType = %s }\n",
            i,
            phydev_prop.deviceName,
            phydev_type_string
        );

        // Also list queue families
        uint32_t phydev_queue_families_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            phydev_list[i],
            &phydev_queue_families_count,
            NULL
        );

        VkQueueFamilyProperties phydev_queue_families[phydev_queue_families_count];
        vkGetPhysicalDeviceQueueFamilyProperties(
            phydev_list[i],
            &phydev_queue_families_count,
            phydev_queue_families
        );

        // Check surface capabilities for phydev
        VkSurfaceCapabilitiesKHR surface_capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            phydev_list[i],
            vk_surface,
            &surface_capabilities
        );

        printf("\tsurface_capabilities:\n\t - image count: from %u to %u\n\t - current extent: (%u, %u)\n",
            surface_capabilities.minImageCount, surface_capabilities.maxImageCount,
            surface_capabilities.currentExtent.width, surface_capabilities.currentExtent.height
        );

        // Check surface formats for phydev
        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            phydev_list[i],
            vk_surface,
            &format_count,
            NULL
        );

        VkSurfaceFormatKHR surface_formats[format_count];
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            phydev_list[i],
            vk_surface,
            &format_count,
            surface_formats
        );

        for(uint32_t format_idx = 0; format_idx < format_count; format_idx++) {
            printf("\tsurface_format[%2u] = { .format = %u, .colorSpace = %u }\n",
                format_idx,
                surface_formats[format_idx].format,
                surface_formats[format_idx].colorSpace
            );
        };

        // Check present modes
        uint32_t present_mode_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            phydev_list[i],
            vk_surface,
            &present_mode_count,
            NULL
        );

        VkPresentModeKHR present_modes[present_mode_count];
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            phydev_list[i],
            vk_surface,
            &present_mode_count,
            present_modes
        );

        for(uint32_t present_mode_idx = 0; present_mode_idx < present_mode_count; present_mode_idx++) {
            char* present_mode_str = NULL;
            switch(present_modes[present_mode_idx]) {
                case 0:
                    present_mode_str = "VK_PRESENT_MODE_IMMEDIATE_KHR";
                    break;

                case 1:
                    present_mode_str = "VK_PRESENT_MODE_MAILBOX_KHR";
                    break;

                case 2:
                    present_mode_str = "VK_PRESENT_MODE_FIFO_KHR";
                    break;

                case 3:
                    present_mode_str = "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
                    break;

                default:
                    present_mode_str = "(unknown)";
                    break;
            }

            printf("\tpresent_mode[%2u] = %s\n",
                present_mode_idx, present_mode_str
            );
        };

        for(uint32_t j = 0; j < phydev_queue_families_count; j++) {
            // check support for presenting to a surface
            VkBool32 present_support;
            vkGetPhysicalDeviceSurfaceSupportKHR(phydev_list[i], j, vk_surface, &present_support);

            VkQueueFamilyProperties* q_props = phydev_queue_families + j;
            printf("\tqueue_family[%2u] = { .queueFlags = (%s%s%s%s%s%s%s), .queueCount = %u, }\n\t - supports presenting to the surface: %s\n",
                j, 
                (q_props->queueFlags & VK_QUEUE_GRAPHICS_BIT) ? "VK_QUEUE_GRAPHICS_BIT" : "",
                (q_props->queueFlags & VK_QUEUE_GRAPHICS_BIT && q_props->queueFlags & VK_QUEUE_COMPUTE_BIT) ? " | " : "",
                (q_props->queueFlags & VK_QUEUE_COMPUTE_BIT) ? "VK_QUEUE_COMPUTE_BIT" : "",
                (q_props->queueFlags & VK_QUEUE_COMPUTE_BIT && q_props->queueFlags & VK_QUEUE_TRANSFER_BIT) ? " | " : "",
                (q_props->queueFlags & VK_QUEUE_TRANSFER_BIT) ? "VK_QUEUE_TRANSFER_BIT" : "",
                (q_props->queueFlags & VK_QUEUE_TRANSFER_BIT && q_props->queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? " | " : "",
                (q_props->queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) ? "VK_QUEUE_SPARSE_BINDING_BIT" : "",
                q_props->queueCount,
                (present_support) ? "TRUE" : "FALSE"
            );
        }
    }

    puts("=== Physical devices props === END");

    // Choose the first one, with one instance of the first queue family
    const uint32_t PHYDEV_IDX = 0;
    const uint32_t QUEUEFAM_IDX = 0;
    VkPhysicalDevice phydev = phydev_list[PHYDEV_IDX];
    printf("Chosen phydev idx: %u\n", PHYDEV_IDX);
    
    // Create a device on our physcial device, with a single queue with max prio (1.0)
    VkDevice dev;
    const float dev_queue_prio[1] = { 1.0 };
    const VkDeviceQueueCreateInfo dev_queues[1] = {
        {
            .sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext              = NULL,
            .flags              = 0,
            .queueFamilyIndex   = QUEUEFAM_IDX,
            .queueCount         = 1,
            .pQueuePriorities   = dev_queue_prio
        }
    };
    const char* dev_extensions[] = { "VK_KHR_swapchain" };
    const VkDeviceCreateInfo dev_createinfo = {
        .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                      = NULL,
        .flags                      = 0,
        .queueCreateInfoCount       = 1,
        .pQueueCreateInfos          = dev_queues,
        .enabledExtensionCount      = 1,
        .ppEnabledExtensionNames    = dev_extensions,
        .pEnabledFeatures           = NULL
    };

    res = vkCreateDevice(
        phydev,
        &dev_createinfo,
        NULL,
        &dev
    );

    if(res != VK_SUCCESS) _DIE("Cant create device");

    puts("Created device");

    const VkFormat SWAPCHAIN_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
    const VkColorSpaceKHR SWAPCHAIN_COLOR_SPACE = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    const VkExtent2D SWAPCHAIN_EXTENT = {
        .width = 640,
        .height = 480
    };

    // Create swapchain
    const VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = NULL,
        .flags = 0,
        .surface = vk_surface,
        .minImageCount = 4,
        .imageFormat = SWAPCHAIN_FORMAT,
        .imageColorSpace = SWAPCHAIN_COLOR_SPACE,
        .imageExtent = SWAPCHAIN_EXTENT,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        
        // Since we use one queue to present and render, we may use exclusive
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = NULL,

        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,

        // Pixels obstructed by other windows and whatnot may be ignored
        .clipped = VK_TRUE,

        // Its the first swapchain we created
        .oldSwapchain = VK_NULL_HANDLE
    };

    VkSwapchainKHR swapchain;
    res = vkCreateSwapchainKHR(
        dev,
        &swapchain_info,
        NULL,
        &swapchain
    );

    if(res != VK_SUCCESS) _DIE("Cant create swapchain");

    puts("Created swapchain to the surface");

    // Retrieve the image handles to the swapchain images
    uint32_t swapchain_images_count = 0;
    vkGetSwapchainImagesKHR(
        dev,
        swapchain,
        &swapchain_images_count,
        NULL
    );

    VkImage swapchain_images[swapchain_images_count];
    vkGetSwapchainImagesKHR(
        dev,
        swapchain,
        &swapchain_images_count,
        swapchain_images
    );

    printf("Retrieved %u swapchain image handles\n", swapchain_images_count);

    VkImageView swapchain_image_views[swapchain_images_count];
    for(size_t idx = 0; idx < swapchain_images_count; idx++) {
        const VkImageViewCreateInfo image_view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .image = swapchain_images[idx],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = SWAPCHAIN_FORMAT,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        
        vkCreateImageView(
            dev,
            &image_view_info,
            NULL,
            &swapchain_image_views[idx]
        );
    }

    printf("Retrieved %u swapchain image view handles\n", swapchain_images_count);

    // Load shaders
    size_t shader_vertex_size, shader_fragment_size;
    uint8_t* shader_vertex_spirv = load_spirv_from_file("../vert.spv", &shader_vertex_size);
    uint8_t* shader_fragment_spirv = load_spirv_from_file("../frag.spv", &shader_fragment_size);

    puts("Shaders loaded");

    // Create shader module object
    const VkShaderModuleCreateInfo shader_vertex_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = shader_vertex_size,
        .pCode = (uint32_t*) shader_vertex_spirv
    };

    const VkShaderModuleCreateInfo shader_fragment_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = shader_fragment_size,
        .pCode = (uint32_t*) shader_fragment_spirv
    };

    VkShaderModule shadermod_vertex, shadermod_fragment;
    res = vkCreateShaderModule(
        dev,
        &shader_vertex_module_info,
        NULL,
        &shadermod_vertex
    );

    if(res != VK_SUCCESS) _DIE("Cant create vertex shader module");

    res = vkCreateShaderModule(
        dev,
        &shader_fragment_module_info,
        NULL,
        &shadermod_fragment
    );

    if(res != VK_SUCCESS) _DIE("Cant create fragment shader module");

    puts("Created shader modules");

    // Start of creation of the pipeline
    // Create an array with two stages: vertex and fragment shaders
    const VkPipelineShaderStageCreateInfo pipeline_shaders[] = {
        // vertex
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = shadermod_vertex,
            .pName = "main",
            .pSpecializationInfo = NULL
        },

        // fragment
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = shadermod_fragment,
            .pName = "main",

            // Can set constants in specializationinfo which allow
            // parametrizing the shader between pipelines
            // this is better than configuring during render time
            // since theres still time for compiler to, say, optimize branches
            // For triangle just leave NULL
            .pSpecializationInfo = NULL
        }
    };

    // this describes the layout of vertices in the input buffer
    // since for now the vertices are hardcoded in the shader, everything is empty
    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL
    };

    VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    // Define viewport and scissor for the viewportstate
    VkViewport pipeline_viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float) SWAPCHAIN_EXTENT.width,
        .height = (float) SWAPCHAIN_EXTENT.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D pipeline_scissor = {
        .offset = { 0, 0 },
        .extent = SWAPCHAIN_EXTENT
    };

    // This can be partially filled during runtime in a command buffer, but we'll put it in now
    VkPipelineViewportStateCreateInfo pipeline_viewpot_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &pipeline_viewport,
        .scissorCount = 1,
        .pScissors = &pipeline_scissor
    };

    // The rasterization step
    VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    // Multisampling disabled as of now, since it requires a gpu feature
    VkPipelineMultisampleStateCreateInfo pipeline_multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    // there could be a VkPipelineDepthStencilStateCreateInfo here if i used depth/stencil testing

    // Color blending needs 2 structures
    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE
        // Theres lots of factor that can be set here, but since blendEnable is disabled anyways no need to
    };

    VkPipelineColorBlendStateCreateInfo pipeline_color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
        // Again, lots of ooptions here but since most functionaity is just disabled, no need to set them
    };

    // With all the shader and non shader stages of pipeline defined, one can create a pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        // Empty layout, so everything else like counts and pointers may be left to 0

    };

    // Create a renderpass (this should be done before finalizing the graphic pipeline i think?)
    VkAttachmentDescription renderpass_attachment_desc = {
        .format = SWAPCHAIN_FORMAT, // Images from our swapchain will be attached (via framebuffer) so the attachment should have the same format
        .samples = VK_SAMPLE_COUNT_1_BIT, // i think this has to do with multisampling?

        // "The loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering"
        // pretty self explanatory
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,

        // Similarly with stencil, but we do not use stencil buffer so we dont care
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,

        // "Textures and framebuffers in Vulkan are represented by VkImage objects with a certain pixel format, however the layout of the pixels in memory can change based on what you're trying to do with an image."
        // "We want the image to be ready for presentation using the swap chain after rendering, which is why we use VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as finalLayout"
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    // Defining subpasses - for triangle we only really need one subpass. Each subpass may use output of the previous.
    
    // What attachments will the subpass reference
    VkAttachmentReference subpass_attachment_reference = {
        .attachment = 0, // index into the array of attachments
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL // which layout should the attachment be in - our color ouput
    };

    // Define the actual subpass
    VkSubpassDescription subpass_definition = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, // this is the only option for now, but vulkan may support compute subpasses in the future - better be explicit now
        .colorAttachmentCount = 1,
        .pColorAttachments = &subpass_attachment_reference

        // there is some more attachment pointers in there than just color, but we do not use them
        // "The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive"
    };

    VkRenderPassCreateInfo renderpass_create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &renderpass_attachment_desc,
        .subpassCount = 1,
        .pSubpasses = &subpass_definition
    };

    VkRenderPass renderpass;

    res = vkCreateRenderPass(dev, &renderpass_create_info, NULL, &renderpass);

    if(res != VK_SUCCESS) _DIE("Cant create renderpass");

    puts("Created renderpass");

    // The actual layout object
    VkPipelineLayout pipeline_layout;

    res = vkCreatePipelineLayout(dev, &pipeline_layout_create_info, NULL, &pipeline_layout);

    if(res != VK_SUCCESS) _DIE("Cant create pipeline layout");

    puts("Created graphic pipeline structures and layout");

    // FINALLY create the actual pipeline
    VkGraphicsPipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2, // 2 shader stages
        .pStages = pipeline_shaders,

        // fixed stages
        .pVertexInputState = &pipeline_vertex_input,
        .pInputAssemblyState = &pipeline_input_assembly,
        .pViewportState = &pipeline_viewpot_state,
        .pRasterizationState = &pipeline_rasterization_state,
        .pMultisampleState = &pipeline_multisampling,
        .pDepthStencilState = NULL, // we do not do depth or stencil
        .pColorBlendState = &pipeline_color_blending,
        .pDynamicState = NULL, // we do not use dynamic state

        .layout = pipeline_layout, // use the empty layout we created

        .renderPass = renderpass,
        .subpass = 0,

        // Last two fields allow re-use of some of previous pipelines, we do not use that feature
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VkPipeline graphics_pipeline;

    // null handle is for pipeline cache which we do not use, also the function can take an array of pipelines
    res = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipeline_create_info, NULL, &graphics_pipeline);

    if(res != VK_SUCCESS) _DIE("Cant create pipeline");

    puts("Created graphic pipeline!!!");

    // Create framebuffers
    VkFramebuffer swapchain_framebuffers[swapchain_images_count];

    for(size_t idx = 0; idx < swapchain_images_count; idx++) {
        VkImageView fb_attachment = swapchain_image_views[idx];

        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderpass,
            .attachmentCount = 1,
            .pAttachments = &fb_attachment,
            .width = SWAPCHAIN_EXTENT.width,
            .height = SWAPCHAIN_EXTENT.height,
            .layers = 1
        };

        res = vkCreateFramebuffer(dev, &fb_info, NULL, &swapchain_framebuffers[idx]);

        if(res != VK_SUCCESS) _DIE("Cant create framebuffer");
    }

    printf("Created framebuffers - %u\n", swapchain_images_count);

    // Create command buffers
    VkCommandPool cmd_pool;

    VkCommandPoolCreateInfo cmd_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // allows buffers to be rerecorded individually
        .queueFamilyIndex = QUEUEFAM_IDX
    };

    res = vkCreateCommandPool(dev, &cmd_pool_create_info, NULL, &cmd_pool);

    if(res != VK_SUCCESS) _DIE("Cannot create command pool");

    puts("Created command pool");

    // allocate a command buffer from the pool
    VkCommandBuffer cmd_buffer;

    // Create 1 buffer on primary level, from the given command pool
    VkCommandBufferAllocateInfo cmd_buffer_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    res = vkAllocateCommandBuffers(dev, &cmd_buffer_alloc_info, &cmd_buffer);

    if(res != VK_SUCCESS) _DIE("Cannot create command buffer");

    puts("Created primary command buffer");

    // Record command buffer to start renderpass
    VkCommandBufferBeginInfo cmd_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0, // There are some flags, but none are useful to us
    };

    res = vkBeginCommandBuffer(cmd_buffer, &cmd_buffer_begin_info);

    if(res != VK_SUCCESS) _DIE("Cannot begin command buffer");

    puts("Began recording the command buffer");

    size_t current_framebuffer_idx = 0;

    VkClearValue clear_color = {
        .color = {
            .float32 = {
                0.0f, 0.0f, 0.0f, 1.0f
            }
        }
    };

    VkRenderPassBeginInfo renderpass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderpass,
        .framebuffer = swapchain_framebuffers[current_framebuffer_idx],
        .renderArea = {
            .offset = { 0, 0 },
            .extent = SWAPCHAIN_EXTENT
        },
        .clearValueCount = 1,
        .pClearValues = &clear_color
    };

    vkCmdBeginRenderPass(cmd_buffer, &renderpass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    // Vulkan may introduce compute pipelines in the future, but we're using graphics
    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

    // We defined 3 vertices in the shader, and there is only one instance of the triangle, offsets both 0
    vkCmdDraw(cmd_buffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmd_buffer);

    res = vkEndCommandBuffer(cmd_buffer);

    if(res != VK_SUCCESS) _DIE("Cannot end command buffer");

    puts("Finished recording the command buffer");

    // Retrieve queue handle
    VkQueue queue;
    vkGetDeviceQueue(
        dev,
        QUEUEFAM_IDX,
        0,
        &queue
    );



    // SETUP ENDS HERE
    // Run the event loop from xcb while vulkan performs rendering
    xcb_generic_event_t* ev = NULL;
    while((ev = xcb_wait_for_event(xcb_ctx->conn))) {
        switch (ev->response_type & ~0x80) {

            case XCB_EXPOSE: {
                //xcb_expose_event_t* expose_ev = (xcb_expose_event_t*) ev;
                puts("EXPOSE event handler: press ESC to exit");
                break;
            }

            case XCB_KEY_RELEASE: {
                xcb_key_release_event_t* key_ev = (xcb_key_release_event_t*) ev;
                
                if(key_ev->detail == 9) {
                    free(ev);
                    return;
                } // ESC
                
                break;
            }

            case XCB_KEY_PRESS: {
                xcb_key_press_event_t* key_ev = (xcb_key_press_event_t*) ev;
                
                if(key_ev->detail == 9) {
                    free(ev);
                    return;
                } // ESC

                break;
            }

        }

        free(ev);
    };

    // !!!! CLEANUP AFTERWARDS !!!!
    vkDestroyCommandPool(dev, cmd_pool, NULL);

    for(size_t idx = 0; idx < swapchain_images_count; idx++) {
        vkDestroyFramebuffer(dev, swapchain_framebuffers[idx], NULL);
    }

    vkDestroyPipeline(dev, graphics_pipeline, NULL);
    vkDestroyPipelineLayout(dev, pipeline_layout, NULL);
    vkDestroyRenderPass(dev, renderpass, NULL);

    vkDestroyShaderModule(dev, shadermod_vertex, NULL);
    vkDestroyShaderModule(dev, shadermod_fragment, NULL);

    free(shader_vertex_spirv);
    free(shader_fragment_spirv);

    for(size_t idx = 0; idx < swapchain_images_count; idx++) {
        vkDestroyImageView(dev, swapchain_image_views[idx], NULL);
    }

    vkDestroySwapchainKHR(
        dev,
        swapchain,
        NULL
    );

    vkDestroySurfaceKHR(
        instance,
        vk_surface,
        NULL
    );

    vkDestroyDevice(
        dev,
        NULL
    );

    vkDestroyInstance(
        instance,
        NULL
    );

    // Cleanup xcb stuff too
    xcb_wrap_destroy_ctx(xcb_ctx);
}   
