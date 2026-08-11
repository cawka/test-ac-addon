// Entry-point boilerplate (CheckEnvironment/RegisterInterface/Initialize/
// FreeData) is taken from GRAPHISOFT/archicad-addon-cmake's
// Src/AddOnMain.cpp (MIT licensed) — see README.md. Everything past
// "our own setup, on top of the template" below is this add-on's.

#include "APIEnvir.h"
#include "ACAPinc.h"

#include "RS.hpp"

#include "GitVersion.hpp"
#include "Debug.hpp"

#include "ResourceIds.hpp"
#include "commands/MenuCommands.hpp"
#include "circuit/CircuitProperty.hpp"
#include "wiring/WireConnection.hpp"

static const GSResID AddOnInfoID		= ID_ADDON_INFO;
static const Int32 AddOnNameID			= 1;
static const Int32 AddOnDescriptionID	= 2;

// One resource per command — see ResourceIds.hpp for why a single
// [title, cmd, cmd, ...] STR# doesn't stay flat under MenuCode_UserDef.
static const GSResID AddOnMenuResIDs[] = {
	ID_ADDON_MENU_CREATE_WIRE,
	ID_ADDON_MENU_CONNECT_WIRE_ENDPOINT,
	ID_ADDON_MENU_CREATE_WIRE_BETWEEN_OBJECTS,
	ID_ADDON_MENU_SELECT_CIRCUIT_WIRING,
	ID_ADDON_MENU_SELECT_CIRCUIT_OBJECTS,
	ID_ADDON_MENU_ABOUT,
};

// ---- Required Add-On lifecycle entry points ----

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, AddOnInfoID, AddOnNameID, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, AddOnInfoID, AddOnDescriptionID, ACAPI_GetOwnResModule ());

	return APIAddon_Normal;
}

GSErrCode RegisterInterface (void)
{
	A2E_TRACE ("A2E: RegisterInterface start\n");
#ifdef ServerMainVers_2700
	// MenuCode_UserDef + a menu-title string as item [1] of each
	// resource (RINT/AddOn.grc) makes this add-on register its own
	// top-level "A2" menu instead of inserting into an existing one
	// (MenuCode_Tools landed under "Options" — not what was wanted).
	// Registered once per command (see ResourceIds.hpp) so Archicad
	// keeps them as flat siblings instead of nesting cmd[2].. under
	// cmd[1] as a submenu.
	for (GSResID menuResID : AddOnMenuResIDs) {
		GSFlags flags = (menuResID == ID_ADDON_MENU_ABOUT) ? MenuFlag_SeparatorBefore : MenuFlag_Default;
		GSErrCode err = ACAPI_MenuItem_RegisterMenu (menuResID, 0, MenuCode_UserDef, flags);
		A2E_TRACE ("A2E: RegisterInterface - RegisterMenu(%d) returned %d\n", (int) menuResID, (int) err);
		if (err != NoError)
			return err;
	}

	// RINT/BuiltInLibParts.grc always ships a built-in library part
	// ("Circuit Wire") in this add-on, so this is called unconditionally
	// rather than detected at runtime — the upstream template's
	// RSEnumResourceTypes-based HasBuiltInLibPart() check existed to
	// make its example work whether or not a given build actually
	// included one; we always do, so that whole detection dance was
	// just one more thing that could silently come back false and skip
	// registration with no error.
	GSErrCode err = ACAPI_AddOnIntegration_RegisterBuiltInLibrary ();
	A2E_TRACE ("A2E: RegisterInterface - RegisterBuiltInLibrary returned %d\n", (int) err);
	return err;
#else
	return ACAPI_Register_Menu (ID_ADDON_MENU_CREATE_WIRE, 0, MenuCode_Tools, MenuFlag_Default);
#endif
}

GSErrCode Initialize (void)
{
	A2E_TRACE ("A2E: Initialize start\n");

	GSErrCode err;
#ifdef ServerMainVers_2700
	for (GSResID menuResID : AddOnMenuResIDs) {
		err = ACAPI_MenuItem_InstallMenuHandler (menuResID, Commands::MenuCommandHandler);
		A2E_TRACE ("A2E: Initialize - InstallMenuHandler(%d) returned %d\n", (int) menuResID, (int) err);
		if (err != NoError)
			return err;
	}
#else
	err = ACAPI_Install_MenuHandler (ID_ADDON_MENU_CREATE_WIRE, Commands::MenuCommandHandler);
	if (err != NoError)
		return err;
#endif

	// Visible, no-debugger-needed confirmation of which build actually
	// loaded — see Window > Report (or wherever this Archicad build
	// surfaces it). A2E_GIT_VERSION comes from `git describe
	// --always --dirty --long`, regenerated on every build (see
	// CMakeLists.txt / cmake/GenerateGitVersion.cmake), not just every
	// reconfigure.
	ACAPI_WriteReport (GS::UniString ("A2 Electrical loaded — build " A2E_GIT_VERSION), false);

	A2E_TRACE ("A2E: Initialize done\n");
	return NoError;
}

GSErrCode FreeData (void)
{
	return NoError;
}
