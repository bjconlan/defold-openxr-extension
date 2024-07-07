#include "xr.hpp"
#include "openxr/openxr.h"

#ifdef _WIN32
	typedef const char* LPCSTR;
	typedef struct HINSTANCE__* HINSTANCE;
	typedef HINSTANCE HMODULE;
	#if defined(_MINWINDEF_)
		/* minwindef.h defines FARPROC, and attempting to redefine it may conflict with -Wstrict-prototypes */
	#elif defined(_WIN64)
		typedef __int64 (__stdcall* FARPROC)(void);
	#else
		typedef int (__stdcall* FARPROC)(void);
	#endif
#else
#include <dlfcn.h>
#endif

PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr;
PFN_xrEnumerateApiLayerProperties xrEnumerateApiLayerProperties;
PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties;
PFN_xrCreateInstance xrCreateInstance;
PFN_xrDestroyInstance xrDestroyInstance;
PFN_xrCreateSession xrCreateSession;
PFN_xrDestroySession xrDestroySession;
PFN_xrResultToString xrResultToString;

#define XR_TOKEN_PASTE(x, y) x ## y
#define XR_TOKEN_PASTE2(x, y) XR_TOKEN_PASTE(x, y)
#define XR_TMP XR_TOKEN_PASTE2(xr_tmp_, __LINE__)
#define XR_CHECK(r)\
  auto && XR_TMP = r;\
  if (XR_FAILED(XR_TMP))\
    return XR_TMP;
#define XR_BIND(x, fn) xrGetInstanceProcAddr(x, #fn, (PFN_xrVoidFunction*)&fn);

XrResult xrLoadAndCreateInstance(
		std::function<const XrInstanceCreateInfo&(std::vector<XrApiLayerProperties>, std::vector<XrExtensionProperties>)> instanceCreateFn,
		XrInstance& instance) {
#if defined(_WIN32)
	HMODULE module = LoadLibraryA("openxr_loader.dll");
	if (!module)
		return XR_ERROR_INITIALIZATION_FAILED;
#else
	void* module = dlopen("libopenxr_loader.so.1", RTLD_NOW | RTLD_LOCAL);
	if (!module)
		module = dlopen("libopenxr_loader.so", RTLD_NOW | RTLD_LOCAL);
	if (!module)
		return XR_ERROR_INITIALIZATION_FAILED;
#endif
	xrGetInstanceProcAddr = (PFN_xrGetInstanceProcAddr)dlsym(module, "xrGetInstanceProcAddr");
	if (xrGetInstanceProcAddr == nullptr) {
		return XR_ERROR_HANDLE_INVALID; // double check this
	}

	uint32_t apiLayerCount = 0;
	XR_CHECK(XR_BIND(instance, xrEnumerateApiLayerProperties));
	XR_CHECK(xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr));
	std::vector<XrApiLayerProperties> apiLayerProperties = {apiLayerCount, {XR_TYPE_API_LAYER_PROPERTIES}};
	XR_CHECK(xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, apiLayerProperties.data()));

	uint32_t extensionCount = 0;
	XR_CHECK(XR_BIND(instance, xrEnumerateInstanceExtensionProperties));
	XR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr));
	std::vector<XrExtensionProperties> extensionProperties = {extensionCount, {XR_TYPE_EXTENSION_PROPERTIES}};
	XR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.data()));

	XR_CHECK(XR_BIND(instance, xrCreateInstance));
	auto createInstance = instanceCreateFn(apiLayerProperties, extensionProperties);
	XR_CHECK(xrCreateInstance(&createInstance, &instance));

	XR_CHECK(XR_BIND(instance, xrDestroyInstance));
	XR_CHECK(XR_BIND(instance, xrCreateSession));
	XR_CHECK(XR_BIND(instance, xrDestroySession));
	XR_CHECK(XR_BIND(instance, xrResultToString));

	return XR_SUCCESS;
}
