// Required Add-On lifecycle entry points. These four function names
// and signatures are fixed by the Archicad Add-On Manager — it looks
// them up by name in the built module, so don't rename them.

#include "ACAPinc.h"
#include "AddOnIdentity.hpp"
#include "Commands/MenuCommands.hpp"
#include "Circuit/CircuitProperty.hpp"
#include "Wiring/WireConnection.hpp"

API_AddonType __ACENV_CALL CheckEnvironment (API_EnvirParams* envir)
{
	RSGetIndString (&envir->addOnInfo.name, ID_ADDON_INFO, ADDON_INFO_NAME, ACAPI_GetOwnResModule ());
	RSGetIndString (&envir->addOnInfo.description, ID_ADDON_INFO, ADDON_INFO_DESC, ACAPI_GetOwnResModule ());

	return APIAddon_Normal;
}

GSErrCode __ACENV_CALL RegisterInterface (void)
{
	return Commands::RegisterMenu ();
}

GSErrCode __ACENV_CALL Initialize (void)
{
	GSErrCode err = Commands::InstallMenuHandler ();
	if (err != NoError)
		return err;

	err = Circuit::EnsureCircuitPropertyDefinition ();
	if (err != NoError)
		return err;

	// Re-attach live element observers for wires connected in a
	// previous session — see docs/ARCHITECTURE.md, section 2.
	return Wiring::RestoreAllConnectionObservers ();
}

GSErrCode __ACENV_CALL FreeData (void)
{
	return NoError;
}
