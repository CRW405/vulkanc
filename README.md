
# VulkanC

- Learning Vulkan in C because I hate C++.
- The ultimate goal of this learning project is to create a simple graphical engine.

## Notes

### (Simplified) Vulkan Setup

Vulkan is a low-level API, so the application explicitly creates the objects
needed to connect a program to a display and submit work to the GPU. The setup
in `attempt3/main.c` follows this order:

#### Window

- GLFW creates the window.
- The OpenGL client API is disabled because Vulkan will render into the window.

#### Validation Layers

- Validation layers inspect Vulkan API usage and report incorrect or unsafe
  behavior while developing.
- They are development tools and are not part of the normal rendering process.

#### Instance

- The Vulkan instance is the application's connection to the Vulkan loader.
- It contains application information and enables the extensions required by
  GLFW to connect Vulkan to the window system.

#### Surface

- A surface represents the window as a Vulkan presentation target.
- Vulkan uses it to determine whether a physical device and queue family can
  display images in that window.

#### Physical Device / GPU

- Vulkan enumerates the available physical devices.
- The application selects a GPU with queue families that support both graphics
  work and presentation.
- A queue family is a group of queues with a particular set of capabilities.

#### Logical Device / Queues

- The logical device is the application's interface to the selected GPU.
- Device extensions, such as `VK_KHR_swapchain`, are enabled here.
- Graphics and presentation queues are retrieved from the logical device and are
  used to submit rendering work and display completed images.

#### Swapchain

- The swapchain is a collection of images that can be rendered to and
  presented to the window.
- The application chooses the image format, presentation mode, extent, and
  number of images based on what the surface supports.
- Rendering can happen in one image while another image is being displayed.

#### Image Views

- A swapchain image is a raw image resource.
- An image view describes how Vulkan should interpret and access that image,
  such as its format and color aspect.

#### Render Pass / Framebuffers

- A render pass describes the attachments used during rendering and how they
  are loaded, stored, and transitioned between layouts.
- A framebuffer connects the render pass to a particular swapchain image view.
- One framebuffer is created for each swapchain image.

#### Command Pool / Command Buffer

- A command pool allocates command buffers for a queue family.
- A command buffer stores rendering commands, such as beginning a render pass,
  clearing an image, and ending the render pass.
- The recorded command buffer is submitted to the graphics queue.

#### Synchronization

- Semaphores coordinate work between the image acquisition, graphics
  submission, and presentation operations.
- A fence lets the CPU wait until the submitted frame has finished.
- Synchronization prevents the CPU or GPU from reusing resources while they
  are still in use.

#### Render Loop

1. Wait for the previous frame's fence.
2. Acquire an available swapchain image.
3. Record commands for that image.
4. Submit the commands to the graphics queue.
5. Present the completed image to the surface.

#### Cleanup

- Wait for the device to become idle.
- Destroy Vulkan objects in reverse order of creation.
- Free host-side allocations, destroy the surface and instance, then destroy
  the GLFW window.

### (Simplified) Graphics Pipeline

- Shaders are programs that run on the GPU. They are written in GLSL or HLSL, C like languages. They are compiled into SPIR-V and then run on the GPU.
- The non shader parts of the pipeline are handled by the GPU driver and are not programmable. They are fixed function but can be configured.

#### Vertex / Index Buffer // Input

- Contain input data such point positons, colors, normals, texture data, etc.
- Basic example could be a list of points. (x_1, y_1), (x_2, y_2), ...
- Think OBJ files.

#### Input Assembler

- Takes our input's vertices and assembles them into primitives (usually a triangle).

#### Vertex Shader

- Runs once per vertex.
- Performs transformations on our vertices. Ex: 3d to 2d projection.
- Can operate on multiple spaces such as world space, model space, view space, clip space.
- Can pass other data such as color or texture data.

#### Rasterization

- Turns primitives into fragments.
- A fragment is used to represent and create a pixel. Doesnt necessarily mean it will be a pixel, but it is a candidate for one.
- Other operations can take place in order to figure out what frasgments turn into what pixels.
- Like coloring in a triangle.

#### Fragment Shader

- Runs once per fragment.
- Takes things like lighting, textures, etc and figures out what color those pixels should be.
- This is where things become pretty.

#### Color Blending

- Self explanatory.
- Think transparency.

#### Frame Buffer / Image

- In actual Vulkan, a frame buffer and image are two seperate things.
- Turn our final fragments into pixels and (possibly) display them.
