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

static const short AddOnMenuID			= ID_ADDON_MENU;

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
	// MenuCode_UserDef + a menu-title string as the first ID_ADDON_MENU
	// entry (RINT/AddOn.grc) makes this add-on register its own
	// top-level "A2" menu instead of inserting into an existing one
	// (MenuCode_Tools landed under "Options" — not what was wanted).
	GSErrCode err = ACAPI_MenuItem_RegisterMenu (AddOnMenuID, 0, MenuCode_UserDef, MenuFlag_Default);
	if (err != NoError)
		return err;

	// RINT/BuiltInLibParts.grc always ships a built-in library part
	// ("Circuit Wire") in this add-on, so this is called unconditionally
	// rather than detected at runtime — the upstream template's
	// RSEnumResourceTypes-based HasBuiltInLibPart() check existed to
	// make its example work whether or not a given build actually
	// included one; we always do, so that whole detection dance was
	// just one more thing that could silently come back false and skip
	// registration with no error.
	err = ACAPI_AddOnIntegration_RegisterBuiltInLibrary ();
	A2E_TRACE ("A2E: RegisterInterface - RegisterBuiltInLibrary returned %d\n", (int) err);
	return err;
#else
	return ACAPI_Register_Menu (AddOnMenuID, 0, MenuCode_Tools, MenuFlag_Default);
#endif
}

GSErrCode Initialize (void)
{
	A2E_TRACE ("A2E: Initialize start\n");

	GSErrCode err;
#ifdef ServerMainVers_2700
	err = ACAPI_MenuItem_InstallMenuHandler (AddOnMenuID, Commands::MenuCommandHandler);
#else
	err = ACAPI_Install_MenuHandler (AddOnMenuID, Commands::MenuCommandHandler);
#endif
	A2E_TRACE ("A2E: Initialize - InstallMenuHandler returned %d\n", (int) err);
	if (err != NoError)
		return err;

	// Our own setup, on top of the template:
	err = Circuit::EnsureCircuitPropertyDefinition ();
	A2E_TRACE ("A2E: Initialize - EnsureCircuitPropertyDefinition returned %d\n", (int) err);
	if (err != NoError)
		return err;

	err = Wiring::RestoreAllConnectionObservers ();
	A2E_TRACE ("A2E: Initialize - RestoreAllConnectionObservers returned %d\n", (int) err);
	if (err != NoError)
		return err;

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
