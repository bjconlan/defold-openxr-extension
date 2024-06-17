#define LIB_NAME "OpenXR"
#define MODULE_NAME "openxr"

#include <dmsdk/sdk.h>

//#if defined(DM_PLATFORM_ANDROID) || defined(DM_PLATFORM_WINDOWS) || defined(DM_PLATFORM_LINUX)
#include <iostream>
#include <vector>
#include <openxr/openxr.h>

inline const char* GetXRErrorString(XrInstance xrInstance, XrResult result) {
  static char string[XR_MAX_RESULT_STRING_SIZE];
  xrResultToString(xrInstance, result, string);
  return string;
}

static XrInstance xr_instance = nullptr;

// lifecycle
static int Init(lua_State* L) {
  DM_LUA_STACK_CHECK(L, 2);

  if (!xr_instance) {
    XrApplicationInfo app_info = {
      "appName",
      1,
      "Defold",
      1,
      XR_CURRENT_API_VERSION
    };

    std::vector<const char*> api_layer_names = {};
    std::vector<const char*> extension_names = {};

    XrInstanceCreateInfo instance_create_info = {
      XR_TYPE_INSTANCE_CREATE_INFO,
      nullptr,
      0,
      app_info,
      static_cast<uint32_t>(api_layer_names.size()),
      api_layer_names.data(),
      static_cast<uint32_t>(extension_names.size()),
      extension_names.data()
    };

    XrResult result = xrCreateInstance(&instance_create_info, &xr_instance);
    if (!XR_SUCCEEDED(result)) {
      std::cerr << "ERROR: OPENXR: " << int(result) << "(" << (xr_instance ? GetXRErrorString(xr_instance, result) : "") << ") " << "Failed to init xr_instance" << std::endl;
    }
  }
  
  return 2;
}

static int Update(lua_State* L) {
  return 0;
}

static int Restart(lua_State* L) {
  DM_LUA_STACK_CHECK(L, 1);
  return 1;
}

static int Final(lua_State* L) {
  delete xr_instance;

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
  //int top = lua_gettop(L);
  luaL_register(L, MODULE_NAME, Module_methods);
}

dmExtension::Result AppInitializeExtension(dmExtension::AppParams* params) {
  
  return dmExtension::RESULT_OK;
}

dmExtension::Result InitializeExtension(dmExtension::Params* params) {
  LuaInit(params->m_L);
  dmLogInfo("Registered %s Extension", MODULE_NAME);
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
    dmLogInfo("OnEventMyExtension - EVENT_ID_ACTIVATEAPP");
    break;
    case dmExtension::EVENT_ID_DEACTIVATEAPP:
    dmLogInfo("OnEventMyExtension - EVENT_ID_DEACTIVATEAPP");
    break;
    case dmExtension::EVENT_ID_ICONIFYAPP:
    dmLogInfo("OnEventMyExtension - EVENT_ID_ICONIFYAPP");
    break;
    case dmExtension::EVENT_ID_DEICONIFYAPP:
    dmLogInfo("OnEventMyExtension - EVENT_ID_DEICONIFYAPP");
    break;
    default:
    dmLogWarning("OnEventMyExtension - Unknown event id");
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