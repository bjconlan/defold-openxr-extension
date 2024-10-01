#define LIB_NAME "OpenXR"
#define MODULE_NAME "openxr"

#include <GL/glx.h>
#include <X11/Xlib.h>
#include <dmsdk/sdk.h>

//#if defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_WINDOWS) || defined(DM_PLATFORM_LINUX)

#include <vector>

#define XR_USE_PLATFORM_XLIB
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr_platform.h>

inline const char* GetXRErrorString(XrInstance instance, XrResult result) {
  static char string[XR_MAX_RESULT_STRING_SIZE];
  xrResultToString(instance, result, string);
  return string;
}

static XrInstance instance = XR_NULL_HANDLE;
static XrSystemId systemId = XR_NULL_SYSTEM_ID;
static XrSession session = XR_NULL_HANDLE;
static XrSpace space = XR_NULL_HANDLE;
static PFN_xrGetOpenGLGraphicsRequirementsKHR pfnGetOpenGLGraphicsRequirementsKHR = nullptr;
// lifecycle
static XrPosef identityPose = {
  .orientation = {.x = 0, .y = 0, .z = 0, .w = 1.0},
  .position = {.x = 0, .y = 0, .z = 0}
};


static auto views = std::vector<XrView>(2);
static auto swapchains = std::vector<XrSwapchain>(2);
static auto swapchainLengths = std::vector<uint32_t>(2);
// static auto images = std::vector<XrSwapchainImageOpenGLKHR*>(3);

// initialization inputs to be resolved before init:
// renderer: opengl|opengles|vulkan (automatically resolved)
// apiLayerProperties & extensionProperties (user supplied/partially automatically resolved from renderer) 

// configurable options
XrFormFactor formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
XrViewConfigurationType viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
XrReferenceSpaceType referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;

static int Init(lua_State* L) {
  //DM_LUA_STACK_CHECK(L, 2);

  if (!instance) {
    uint32_t apiLayerCount = 0;
    XrResult result = xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr);
    std::vector<XrApiLayerProperties> apiLayerProperties = {apiLayerCount, {XR_TYPE_API_LAYER_PROPERTIES}};
    result = xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, apiLayerProperties.data());
    dmLogDebug("API layers (%d):", static_cast<uint32_t>(apiLayerProperties.size()));
    for (auto apiLayerProperty : apiLayerProperties) {
      dmLogDebug("\t%s v%d: %s", apiLayerProperty.layerName, apiLayerProperty.layerVersion, apiLayerProperty.description);
    }

    uint32_t extensionCount = 0;
    result = xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr);
    std::vector<XrExtensionProperties> extensionProperties = {extensionCount, {XR_TYPE_EXTENSION_PROPERTIES}};
    result = xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.data());
    dmLogDebug("Runtime supports %d extensions:", static_cast<uint32_t>(extensionProperties.size()));
    for (auto extensionProperty : extensionProperties) {
      dmLogDebug("\t%s v%d", extensionProperty.extensionName, extensionProperty.extensionVersion);
    }
      
    std::vector<const char*> enabledApiLayerNames = {};
    std::vector<const char*> enabledExtensionNames = {
      XR_EXT_DEBUG_UTILS_EXTENSION_NAME,
      XR_KHR_OPENGL_ENABLE_EXTENSION_NAME
    };

    XrInstanceCreateInfo createInfo{
      .type = XR_TYPE_INSTANCE_CREATE_INFO,
      .next = nullptr,
      .createFlags = 0,
      .applicationInfo = {
        .applicationName = "[todo: use project name]",
        .applicationVersion = 1,
        .engineName = "Defold",
        .engineVersion = 0,
        .apiVersion = XR_CURRENT_API_VERSION,
      },
      .enabledApiLayerCount = static_cast<uint32_t>(enabledApiLayerNames.size()),
      .enabledApiLayerNames = enabledApiLayerNames.data(),
      .enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size()),
      .enabledExtensionNames = enabledExtensionNames.data(),
    };

    if (auto result = xrCreateInstance(&createInfo, &instance); XR_FAILED(result)) {
      dmLogFatal("%d (%s) Failed to init instance", int(result), (instance ? GetXRErrorString(instance, result) : ""));
      return result;
    }

    XrInstanceProperties instanceProperties = {
      .type = XR_TYPE_INSTANCE_PROPERTIES,
      .next = nullptr
    };
    result = xrGetInstanceProperties(instance, &instanceProperties);
    dmLogInfo("Runtime Name: %s", instanceProperties.runtimeName);
    dmLogInfo("Runtime Version: %d.%d.%d", 
      (uint32_t)(((uint64_t)instanceProperties.runtimeVersion >> 48) & 0xffffULL),
      (uint32_t)(((uint64_t)instanceProperties.runtimeVersion >> 32) & 0xffffULL), 
      (uint32_t)(((uint64_t)instanceProperties.runtimeVersion) & 0xffffffffULL));

    XrSystemGetInfo systemGetInfo = {
      .type = XR_TYPE_SYSTEM_GET_INFO,
      .next = nullptr,
      .formFactor = formFactor
    };
    if (auto result = xrGetSystem(instance, &systemGetInfo, &systemId); XR_FAILED(result)) {
      dmLogFatal("%d (%s) Failed to get system for HMD form factor", int(result), GetXRErrorString(instance, result));
      return result;
    }
    dmLogInfo("Successfully got XrSystem with id %lu for HMD form factor", systemId);

    XrSystemProperties systemProperties = {
      .type = XR_TYPE_SYSTEM_PROPERTIES,
      .next = nullptr
    };
    if (auto result = xrGetSystemProperties(instance, systemId, &systemProperties); XR_FAILED(result)) {
      dmLogFatal("Failed to get system properties");
      return result;
    }
    dmLogInfo("System properties for system %lu: \"%s\", vendor ID %d",
      systemProperties.systemId, systemProperties.systemName, systemProperties.vendorId);
    dmLogInfo("\tMax layers          : %d", systemProperties.graphicsProperties.maxLayerCount);
    dmLogInfo("\tMax swapchain height: %d", systemProperties.graphicsProperties.maxSwapchainImageHeight);
    dmLogInfo("\tMax swapchain width : %d", systemProperties.graphicsProperties.maxSwapchainImageWidth);
    dmLogInfo("\tOrientation Tracking: %d", systemProperties.trackingProperties.orientationTracking);
    dmLogInfo("\tPosition Tracking   : %d", systemProperties.trackingProperties.positionTracking);

    uint32_t viewConfigurationCount = 0;    
    if (auto result = xrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, 0, &viewConfigurationCount, NULL); XR_FAILED(result)) {
      dmLogFatal("Failed to get view configuration count");
      return result;
    }
    std::vector<XrViewConfigurationView> viewConfigurations = {viewConfigurationCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW}};
    if (auto result = xrEnumerateViewConfigurationViews(instance, systemId, viewConfigurationType, viewConfigurationCount, &viewConfigurationCount, viewConfigurations.data()); XR_FAILED(result)) {
      dmLogFatal("Failed to enumerate view configuration views");
      return result;
    }
    for (auto viewConfiguration : viewConfigurations) {
      dmLogInfo("View Configuration View:");
      dmLogInfo("\tResolution       : Recommended %dx%d, Max: %dx%d",
        viewConfiguration.recommendedImageRectWidth,
        viewConfiguration.recommendedImageRectHeight,
        viewConfiguration.maxImageRectWidth,
        viewConfiguration.maxImageRectHeight);
      dmLogInfo("\tSwapchain Samples: Recommended: %d, Max: %d)",
        viewConfiguration.recommendedSwapchainSampleCount,
        viewConfiguration.maxSwapchainSampleCount);
    }

    if (auto result = xrGetInstanceProcAddr(instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&pfnGetOpenGLGraphicsRequirementsKHR); XR_FAILED(result)) {
      dmLogFatal("Failed to get OpenGL graphics requirements function!");
      return result;
    }
    
    XrGraphicsRequirementsOpenGLKHR openglReqs = {
      .type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR,
      .next = NULL
    };
    if (auto result = pfnGetOpenGLGraphicsRequirementsKHR(instance, systemId, &openglReqs); XR_FAILED(result)) {
      dmLogFatal("Failed to get OpenGl graphics requirements");
      return result;
    }
    dmLogInfo("GL requirements: min %lu, max: %lu", openglReqs.minApiVersionSupported, openglReqs.maxApiVersionSupported);

    XrGraphicsBindingOpenGLXlibKHR graphicsBindingGl = {
      .type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR,
      .xDisplay = XOpenDisplay(NULL),
      .glxDrawable = glXGetCurrentDrawable(),
      .glxContext = dmGraphics::GetNativeX11GLXContext(),
    };

    XrSessionCreateInfo sessionCreateInfo = {
      .type = XR_TYPE_SESSION_CREATE_INFO,
      .next = &graphicsBindingGl,
      .systemId = systemId
    };
    if (auto result = xrCreateSession(instance, &sessionCreateInfo, &session); XR_FAILED(result)) {
      dmLogFatal("Failed to create session");
      return result;
    }
    dmLogInfo("Successfully bound openxr gl context");


    XrReferenceSpaceCreateInfo referenceSpaceCreateinfo = {
      .type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
      .next = nullptr,
      .referenceSpaceType = referenceSpaceType,
      .poseInReferenceSpace = identityPose
    };
    if (result = xrCreateReferenceSpace(session, &referenceSpaceCreateinfo, &space); XR_FAILED(result)) {
      dmLogFatal("Failed to create play space!");
      return 1;
    }

    uint32_t swapchainFormatCount;
    if (auto result = xrEnumerateSwapchainFormats(session, 0, &swapchainFormatCount, NULL); XR_FAILED(result)) {
      dmLogFatal("Failed to get number of supported swapchain formats");
      return result;
    }
    auto swapchainFormats = std::vector<int64_t>(swapchainFormatCount);
    if (auto result = xrEnumerateSwapchainFormats(session, swapchainFormatCount, &swapchainFormatCount, swapchainFormats.data()); XR_FAILED(result)) {
      dmLogFatal("Failed to get number of supported swapchain formats");
      return result;
    }
    for (auto swapchainFormat : swapchainFormats) {
      dmLogInfo("Supported GL format: %#lx", swapchainFormat);
      if (swapchainFormat == GL_SRGB8_ALPHA8_EXT) {
        dmLogInfo("found SRGB888_ALPHA8");        
      }
      // if (swapchainFormat == GL_DEPTH_COMPONENT16) {
      //   dmLogInfo("found GL_DEPTH_COMPONENT16");
      // } ignore depth creation for the moment
    }

    for (uint32_t i = 0; i < viewConfigurationCount; i++) {
      XrSwapchainCreateInfo swapchainCreateInfo = {
        .type = XR_TYPE_SWAPCHAIN_CREATE_INFO,
        .usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT,
        .createFlags = 0,
        .format = GL_SRGB8_ALPHA8_EXT,
        .sampleCount = viewConfigurations[i].recommendedSwapchainSampleCount,
        .width = viewConfigurations[i].recommendedImageRectWidth,
        .height = viewConfigurations[i].recommendedImageRectHeight,
        .faceCount = 1,
        .arraySize = 1,
        .mipCount = 1,
        .next = NULL,
      };

      if (auto result = xrCreateSwapchain(session, &swapchainCreateInfo, &swapchains[i]); XR_FAILED(result)) {
        dmLogFatal("Failed to create swapchain %d", i);
        return result;
      }
      if (auto result = xrEnumerateSwapchainImages(swapchains[i], 0, &swapchainLengths[i], NULL); XR_FAILED(result)) {
        dmLogFatal("Failed to enumerate swapchains");
        return result;
      }
      dmLogInfo("swapchain %d's length: %d", i, swapchainLengths[i]);
      
    }
    
    // line 880
    //std::vector<XrSwapchain> swapchains = {viewConfigurationCount, {XR_TYPE_SWAPCHAIN_CREATE_INFO}};
    //std::vector<XrSwapchainImageOpenGLKHR> images = {viewConfigurationCount, {}}
  }

  return 0;
}

static int Update(lua_State* L) {
  return 0;
}

static int Restart(lua_State* L) {
  // DM_LUA_STACK_CHECK(L, 1);
  // return 1;
  return 0;
}

static int Final(lua_State* L) {
  if (instance) {
    xrDestroyInstance(instance);
  }

  if (session) {
    xrDestroySession(session);
  }

  return 0;
}

static const luaL_reg Module_methods[] = {
  { "init", Init },
  { "restart", Restart },
  { "update", Update },
  { "final", Final },
  { 0, 0 }
};

static void LuaInit(lua_State* L) {
  int top = lua_gettop(L);

  // Register lua names
  luaL_register(L, MODULE_NAME, Module_methods);

  lua_pop(L, 1);
  assert(top == lua_gettop(L));
}

dmExtension::Result AppInitializeExtension(dmExtension::AppParams* params) {
  return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeExtension(dmExtension::Params* params) {
  LuaInit(params->m_L);
  dmLogInfo("Registered %s Extension", MODULE_NAME);

  Init(params->m_L);
  
  return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizeExtension(dmExtension::AppParams* params) {
  return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizeExtension(dmExtension::Params* params) {
  return dmExtension::RESULT_OK;
}

static void OnEventExtension(dmExtension::Params* params, const dmExtension::Event* event) {
  switch(event->m_Event)
  {
    case dmExtension::EVENT_ID_ACTIVATEAPP:
      dmLogInfo("EVENT_ID_ACTIVATEAPP");
      break;
    case dmExtension::EVENT_ID_DEACTIVATEAPP:
      dmLogInfo("EVENT_ID_DEACTIVATEAPP");
      break;
    case dmExtension::EVENT_ID_ICONIFYAPP:
      dmLogInfo("EVENT_ID_ICONIFYAPP");
      break;
    case dmExtension::EVENT_ID_DEICONIFYAPP:
      dmLogInfo("EVENT_ID_DEICONIFYAPP");
      break;
    default:
      dmLogWarning("Unknown event id");
      break;
  }
}

// engine start
// extension: app_init
// extension: init (post defold api loading)
// script: init

// engine loop
// extension: update
// script: update
// extension: on_event

// engine shutdown
// script: final
// extension final
// extension app_final


DM_DECLARE_EXTENSION(openxr, LIB_NAME, AppInitializeExtension, AppFinalizeExtension, InitializeExtension, 0, OnEventExtension, FinalizeExtension)

//#endif