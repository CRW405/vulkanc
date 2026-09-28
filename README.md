
# VulkanC

- Learning Vulkan in C because I hate C++.
- The ultimate goal of this learning project is to create a simple graphical engine.

## Current TODOs:

- Push constants and transformation matrices.
- Text rendering.
- 2D scenes
- 3D scenes

## Notes

### (Simplified) Vulkan Setup

Vulkan is a low-level API, so the application explicitly creates the objects
needed to connect a program to a display and submit work to the GPU. The setup
in `src/main.c` and `src/vEngine/` follows this order:

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
- When the window is resized, or when image acquisition/presentation reports
  that the swapchain is out of date, the device is idled and the swapchain,
  image views, framebuffers, and swapchain-dependent semaphores are recreated.

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
- Two frames can be in flight at the same time. Each frame has its own command
  buffer, image-available semaphore, and fence.
- Each swapchain image tracks the fence of the frame currently using it, so an
  image is not reused while the GPU is still rendering it.
- Synchronization prevents the CPU or GPU from reusing resources while they
  are still in use.

#### Render Loop

1. Wait for the current frame's fence.
2. Acquire an available swapchain image.
3. Wait for the fence currently associated with that image, if any.
4. Record commands into the current frame's command buffer.
5. Submit the commands to the graphics queue and signal the frame fence.
6. Present the completed image to the surface.
7. Advance to the next frame slot.

#### Cleanup

- Wait for the device to become idle.
- Destroy Vulkan objects in reverse order of creation.
- Free host-side allocations, destroy the surface and instance, then destroy
  the GLFW window.

### (Simplified) Graphics Pipeline

- Shaders are programs that run on the GPU. They are written in GLSL or HLSL,
  C-like languages, compiled into SPIR-V, and then loaded into Vulkan shader
  modules.
- The non-shader parts of the pipeline are handled by the GPU driver and are
  fixed-function stages that can be configured.

#### Vertex Buffer // Input

- Vertex buffers contain input data such as positions, colors, normals, and
  texture coordinates.
- This project currently uploads a small array of colored triangle vertices
  into a device-local vertex buffer.
- Think OBJ files.

#### Input Assembler

- Takes the input vertices and assembles them into primitives, usually
  triangles.

#### Vertex Shader

- Runs once per vertex.
- Performs transformations on vertices, such as 3D-to-2D projection.
- Can operate on multiple spaces such as model, world, view, and clip space.
- Can pass data such as colors or texture coordinates to later stages.

#### Rasterization

- Turns primitives into fragments.
- A fragment is a candidate for a pixel; later tests determine whether and how
  it contributes to the framebuffer.
- This is similar to coloring in a triangle one fragment at a time.

#### Fragment Shader

- Runs once per fragment.
- Uses inputs such as colors, lighting, and textures to determine fragment
  output.
- This is where things become pretty.

#### Color Blending

- Combines fragment output with the existing framebuffer color.
- Blending can be configured for effects such as transparency.

#### Frame Buffer / Image

- In Vulkan, a framebuffer and an image are separate objects.
- The swapchain image is the color attachment that receives the final
  fragments and is eventually presented to the window.
