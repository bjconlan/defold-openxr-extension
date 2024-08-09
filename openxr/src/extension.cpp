#define LIB_NAME "OpenXR"
#define MODULE_NAME "openxr"

#include <dmsdk/sdk.h>

//#if defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_WINDOWS) || defined(DM_PLATFORM_LINUX)

#include <vector>

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

// lifecycle
static int Init(lua_State* L) {
  //DM_LUA_STACK_CHECK(L, 2);

  if (!instance) {
//       dmLogInfo("API layers (%d):", static_cast<uint32_t>(apiLayerProperties.size()));
//       for (auto apiLayerProperty : apiLayerProperties) {
//         dmLogInfo("\t%s v%d: %s", apiLayerProperty.layerName, apiLayerProperty.layerVersion, apiLayerProperty.description);
//       }
// 
//       dmLogInfo("Runtime supports %d extensions:", static_cast<uint32_t>(extensionProperties.size()));
//       for (auto extensionProperty : extensionProperties) {
//         dmLogInfo("\t%s v%d", extensionProperty.extensionName, extensionProperty.extensionVersion);
//       }
      
    std::vector<const char*> enabledApiLayerNames = {};
    std::vector<const char*> enabledExtensionNames = {XR_EXT_DEBUG_UTILS_EXTENSION_NAME};

    XrInstanceCreateInfo createInfo{
      .type = XR_TYPE_INSTANCE_CREATE_INFO,
      .next = nullptr,
      .createFlags = 0,
      .applicationInfo = {
        .applicationName = "[todo: use project name]",
        .applicationVersion = 1,
        .engineName = "Defold",
        .engineVersion = 0,
        .apiVersion = XR_API_VERSION_1_0,
      },
      .enabledApiLayerCount = static_cast<uint32_t>(enabledApiLayerNames.size()),
      .enabledApiLayerNames = enabledApiLayerNames.data(),
      .enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size()),
      .enabledExtensionNames = enabledExtensionNames.data(),
    };

    if (auto result = xrCreateInstance(&createInfo, &instance); XR_FAILED(result)) {
      dmLogInfo("%d (%s) Failed to init instance", int(result), (instance ? GetXRErrorString(instance, result) : ""));
    }
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