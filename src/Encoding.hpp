#pragma once

#include <string>

namespace utils
{
	// Encoding of Pawn script strings as seen by the plugin.
	// RAW means: no conversion is performed (treat bytes as UTF-8 already).
	// WINDOWS_874 means: Pawn provides/consumes Windows-874 (CP874) byte strings.
	enum class PawnEncoding
	{
		RAW = 0,
		WINDOWS_874 = 874
	};

	void SetPawnEncoding(PawnEncoding enc);
	PawnEncoding GetPawnEncoding();

	// Convert Pawn bytes -> UTF-8 for Discord/JSON.
	// If encoding is RAW, this returns the input unchanged.
	std::string FromPawnEncoding(std::string s);

	// Convert UTF-8 -> Pawn bytes for returning strings to Pawn.
	// If encoding is RAW, this returns the input unchanged.
	std::string ToPawnEncoding(std::string const& utf8);
}

