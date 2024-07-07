#pragma once

#include <functional>
#include <vector>

#define XR_NO_PROTOTYPES 1
#include <openxr/openxr.h>

extern PFN_xrGetInstanceProcAddr xrGetInstanceProcAddr;
extern PFN_xrEnumerateApiLayerProperties xrEnumerateApiLayerProperties;
extern PFN_xrEnumerateInstanceExtensionProperties xrEnumerateInstanceExtensionProperties;
extern PFN_xrCreateInstance xrCreateInstance;
extern PFN_xrDestroyInstance xrDestroyInstance;
extern PFN_xrCreateSession xrCreateSession;
extern PFN_xrDestroySession xrDestroySession;
extern PFN_xrResultToString xrResultToString;

XrResult xrLoadAndCreateInstance(
  std::function<const XrInstanceCreateInfo&(std::vector<XrApiLayerProperties>, std::vector<XrExtensionProperties>)> instanceCreateFn,
  XrInstance& instance
);