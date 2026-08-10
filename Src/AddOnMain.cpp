// Entry-point boilerplate (CheckEnvironment/RegisterInterface/Initialize/
// FreeData, the built-in-library-part detection dance) is taken from
// GRAPHISOFT/archicad-addon-cmake's Src/AddOnMain.cpp (MIT licensed) —
// see README.md. Everything past "our own setup, on top of the
// template" below is this add-on's.

#include "APIEnvir.h"
#include "ACAPinc.h"

#include "RS.hpp"
#include "UniString.hpp"

#include "ResourceIds.hpp"
#include "Commands/MenuCommands.hpp"
#include "Circuit/CircuitProperty.hpp"
#include "Wiring/WireConnection.hpp"

static const GSResID AddOnInfoID		= ID_ADDON_INFO;
	static const Int32 AddOnNameID		= 1;
	static const Int32 AddOnDescriptionID	= 2;

static const short AddOnMenuID			= ID_ADDON_MENU;

// ---- Built-in library part detection, from the upstream template ----
// (needed because RINT/BuiltInLibParts.grc + RINT/ACLib/Src/CircuitWire/
// ship the "Circuit Wire" GDL object baked into the add-on itself,
// rather than requiring it to be added to a project library by hand.)

#ifdef ServerMainVers_2700
#if defined(macintosh)

bool CCALL ResourceTypesCallback (GSResType resType, GSResModule /*resModule*/, void *userData)
{
	constexpr GSResType libPartType = 'FLDR';
	if (resType == libPartType) {
		bool* hasLibPart = static_cast<bool*> (userData);
		*hasLibPart = true;
		return false; // stop enumeration
	}
	return true; // continue enumeration
}

#else

bool CCALL ResourceTypesCallback (const WCHAR* resType, GSResModule /*resModule*/, void *userData)
{
	constexpr auto libPartType = "FLDR";
	if (GS::UniString (resType) == libPartType) {
		bool* hasLibPart = static_cast<bool*> (userData);
		*hasLibPart = true;
		return false; // stop enumeration
	}
	return true; // continue enumeration
}
#endif

static bool HasBuiltInLibPart ()
{
	bool hasLibPart = false;
	RSEnumResourceTypes (ResourceTypesCallback, &hasLibPart, ACAPI_GetOwnResModule ());

	return hasLibPart;
}

#endif

// ---- Required Add-On lifecycle entry points ----

API_AddonType CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, AddOnInfoID, AddOnNameID, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, AddOnInfoID, AddOnDescriptionID, ACAPI_GetOwnResModule ());

	return APIAddon_Normal;
}

GSErrCode RegisterInterface (void)
{
#ifdef ServerMainVers_2700
	GSErrCode err = ACAPI_MenuItem_RegisterMenu (AddOnMenuID, 0, MenuCode_Tools, MenuFlag_Default);
	if (err == NoError && HasBuiltInLibPart ())
		err = ACAPI_AddOnIntegration_RegisterBuiltInLibrary ();

	return err;
#else
	return ACAPI_Register_Menu (AddOnMenuID, 0, MenuCode_Tools, MenuFlag_Default);
#endif
}

GSErrCode Initialize (void)
{
	GSErrCode err;
#ifdef ServerMainVers_2700
	err = ACAPI_MenuItem_InstallMenuHandler (AddOnMenuID, Commands::MenuCommandHandler);
#else
	err = ACAPI_Install_MenuHandler (AddOnMenuID, Commands::MenuCommandHandler);
#endif
	if (err != NoError)
		return err;

	// Our own setup, on top of the template:
	err = Circuit::EnsureCircuitPropertyDefinition ();
	if (err != NoError)
		return err;

	// Re-attach live element observers for wires connected in a
	// previous session — see docs/ARCHITECTURE.md, section 2.
	return Wiring::RestoreAllConnectionObservers ();
}

GSErrCode FreeData (void)
{
	return NoError;
}
