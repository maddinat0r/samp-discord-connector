#pragma once

#include "sdk.hpp"

#define AMX_DECLARE_NATIVE(native) \
	cell AMX_NATIVE_CALL native(AMX *amx, cell *params)

#define AMX_DEFINE_NATIVE(native) \
	{#native, Native::native},


namespace Native
{
	//AMX_DECLARE_NATIVE(native_name);
	AMX_DECLARE_NATIVE(DCC_Connect);
	AMX_DECLARE_NATIVE(DCC_SendMessage);
};
