#include "globals.hpp"
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/desktop/Workspace.hpp>
#include "monocleLayout.hpp"
#include <unistd.h>
#include <thread>
// Methods
inline std::unique_ptr<CHyprMonocleLayout> g_pMonocleLayout;


// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}


APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    g_pMonocleLayout = std::make_unique<CHyprMonocleLayout>();

	
	
    HyprlandAPI::addLayout(PHANDLE, "monocle", g_pMonocleLayout.get());

    HyprlandAPI::reloadConfig();

    return {"Monocle layout", "Plugin for monocle layout", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::invokeHyprctlCommand("seterror", "disable");
}
