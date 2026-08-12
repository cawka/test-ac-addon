// Required Add-On lifecycle entry points (CheckEnvironment/
// RegisterInterface/Initialize/FreeData) — these must stay free
// functions with these exact names; Archicad calls them by symbol.
// Boilerplate shape taken from GRAPHISOFT/archicad-addon-cmake's
// Src/AddOnMain.cpp (MIT licensed) — see README.md.

#include "APIEnvir.h"
#include "ACAPinc.h"

#include "RS.hpp"

#include "GitVersion.hpp"
#include "Debug.hpp"

#include "ResourceIds.hpp"
#include "commands/MenuCommands.hpp"
#include "circuit/CircuitProperty.hpp"
#include "wiring/WireConnection.hpp"

namespace {

const GSResID AddOnInfoID		= ID_ADDON_INFO;
const Int32 AddOnNameID		= 1;
const Int32 AddOnDescriptionID	= 2;

// One resource per command — a single [title, cmd, cmd, ...] STR# does
// not stay flat under MenuCode_UserDef (see ResourceIds.hpp).
const GSResID AddOnMenuResIDs[] = {
	ID_ADDON_MENU_CREATE_WIRE,
	ID_ADDON_MENU_CONNECT_WIRE_ENDPOINT,
	ID_ADDON_MENU_CREATE_WIRE_BETWEEN_OBJECTS,
	ID_ADDON_MENU_SELECT_CIRCUIT_WIRING,
	ID_ADDON_MENU_SELECT_CIRCUIT_OBJECTS,
	ID_ADDON_MENU_ABOUT,
};

// Currently a no-op; registered alongside the element observers below
// mainly to keep the init/free lifecycle complete.
GSErrCode ProjectEventHandlerProc (API_NotifyEventID /*notifID*/, Int32 /*param*/)
{
	return NoError;
}

} // namespace

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, AddOnInfoID, AddOnNameID, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, AddOnInfoID, AddOnDescriptionID, ACAPI_GetOwnResModule ());

	return APIAddon_Normal;
}

GSErrCode RegisterInterface (void)
{
#ifdef ServerMainVers_2700
	for (GSResID menuResID : AddOnMenuResIDs) {
		GSFlags flags = (menuResID == ID_ADDON_MENU_ABOUT) ? MenuFlag_SeparatorBefore : MenuFlag_Default;
		GSErrCode err = ACAPI_MenuItem_RegisterMenu (menuResID, 0, MenuCode_UserDef, flags);
		if (err != NoError)
			return err;
	}

	// RINT/BuiltInLibParts.grc always ships the "Circuit Wire" built-in
	// library part, so this runs unconditionally.
	return ACAPI_AddOnIntegration_RegisterBuiltInLibrary ();
#else
	return ACAPI_Register_Menu (ID_ADDON_MENU_CREATE_WIRE, 0, MenuCode_Tools, MenuFlag_Default);
#endif
}

GSErrCode Initialize (void)
{
	GSErrCode err;
#ifdef ServerMainVers_2700
	for (GSResID menuResID : AddOnMenuResIDs) {
		err = ACAPI_MenuItem_InstallMenuHandler (menuResID, Commands::MenuCommandDispatcher::Handle);
		if (err != NoError)
			return err;
	}
#else
	err = ACAPI_Install_MenuHandler (ID_ADDON_MENU_CREATE_WIRE, Commands::MenuCommandDispatcher::Handle);
	if (err != NoError)
		return err;
#endif

	ACAPI_ProjectOperation_CatchProjectEvent (API_AllProjectNotificationMask, ProjectEventHandlerProc);

	// A notification registration isn't a database write, so it belongs
	// in Initialize(), not inside a menu command's undoable-command
	// block — see ConnectionManager::AttachHostObserver's NOTE.
	err = ACAPI_Element_InstallElementObserver (Wiring::ConnectionManager::OnHostElementChanged);
	A2E_TRACE ("A2E: Initialize - InstallElementObserver returned %d\n", (int) err);

	// Catches every newly created element so a future connect can find
	// hosts without a separate discovery step. Currently unused by any
	// command but registered/freed here to keep the lifecycle complete.
	if (err == NoError)
		ACAPI_Element_CatchNewElement (nullptr, Wiring::ConnectionManager::OnHostElementChanged);

	// Visible, no-debugger-needed confirmation of which build loaded.
	// A2E_GIT_VERSION comes from `git describe --always --dirty --long`,
	// regenerated on every build (see cmake/GenerateGitVersion.cmake).
	ACAPI_WriteReport (GS::UniString ("A2 Electrical loaded — build " A2E_GIT_VERSION), false);

	return NoError;
}

GSErrCode FreeData (void)
{
	// Mirror of Initialize()'s three registrations — nullptr handlerProc
	// is the documented way to unregister each of these.
	ACAPI_ProjectOperation_CatchProjectEvent (0, nullptr);
	ACAPI_Element_InstallElementObserver (nullptr);
	ACAPI_Element_CatchNewElement (nullptr, nullptr);

	return NoError;
}
