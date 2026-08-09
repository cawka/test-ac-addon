#ifndef ADDON_IDENTITY_HPP
#define ADDON_IDENTITY_HPP

// Placeholder GUIDs — regenerate before this add-on ever loads into a
// real Archicad. Two add-ons sharing a GUID will conflict at startup.
// Any GUID generator works (Visual Studio's guidgen, `uuidgen`, etc).

#define ADDON_MENU_STRINGS_RESOURCE_ID   32000
#define ADDON_MENU_PROMPT_STRINGS_ID     32001
#define ID_ADDON_INFO                    32100
#define ADDON_INFO_NAME                  1
#define ADDON_INFO_DESC                  2

namespace AddOnIdentity {

// TODO: replace with a freshly generated GUID before first load.
constexpr char kAddOnGuid[]      = "00000000-0000-0000-0000-000000000000";

constexpr char kCircuitIdPropertyName[] = "Circuit ID";

} // namespace AddOnIdentity

#endif
