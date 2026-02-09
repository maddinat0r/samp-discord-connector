#include "Encoding.hpp"

#include <atomic>
#include <cstdint>

namespace utils
{
	static std::atomic<int> g_PawnEncoding{ static_cast<int>(PawnEncoding::RAW) };

	void SetPawnEncoding(PawnEncoding enc)
	{
		g_PawnEncoding.store(static_cast<int>(enc), std::memory_order_relaxed);
	}

	PawnEncoding GetPawnEncoding()
	{
		return static_cast<PawnEncoding>(g_PawnEncoding.load(std::memory_order_relaxed));
	}

	static inline void AppendUtf8(std::string& out, uint32_t cp)
	{
		if (cp <= 0x7Fu)
		{
			out.push_back(static_cast<char>(cp));
		}
		else if (cp <= 0x7FFu)
		{
			out.push_back(static_cast<char>(0xC0u | ((cp >> 6) & 0x1Fu)));
			out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
		}
		else if (cp <= 0xFFFFu)
		{
			out.push_back(static_cast<char>(0xE0u | ((cp >> 12) & 0x0Fu)));
			out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
			out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
		}
		else if (cp <= 0x10FFFFu)
		{
			out.push_back(static_cast<char>(0xF0u | ((cp >> 18) & 0x07u)));
			out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
			out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
			out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
		}
		else
		{
			out.push_back('?');
		}
	}

	static inline uint32_t Cp1252ToUnicode(unsigned char b)
	{
		// Only valid for 0x80..0x9F. (Windows-874 shares this range with Windows-1252.)
		// Undefined bytes are mapped to their C1 control codepoints, which is a reasonable
		// "lossless" fallback for round-tripping unknown bytes.
		static const uint32_t table[32] = {
			0x20ACu, 0x0081u, 0x201Au, 0x0192u, 0x201Eu, 0x2026u, 0x2020u, 0x2021u,
			0x02C6u, 0x2030u, 0x0160u, 0x2039u, 0x0152u, 0x008Du, 0x017Du, 0x008Fu,
			0x0090u, 0x2018u, 0x2019u, 0x201Cu, 0x201Du, 0x2022u, 0x2013u, 0x2014u,
			0x02DCu, 0x2122u, 0x0161u, 0x203Au, 0x0153u, 0x009Du, 0x017Eu, 0x0178u
		};
		return table[b - 0x80u];
	}

	static inline bool UnicodeToCp1252(uint32_t cp, unsigned char& out_b)
	{
		switch (cp)
		{
		case 0x20ACu: out_b = 0x80u; return true;
		case 0x201Au: out_b = 0x82u; return true;
		case 0x0192u: out_b = 0x83u; return true;
		case 0x201Eu: out_b = 0x84u; return true;
		case 0x2026u: out_b = 0x85u; return true;
		case 0x2020u: out_b = 0x86u; return true;
		case 0x2021u: out_b = 0x87u; return true;
		case 0x02C6u: out_b = 0x88u; return true;
		case 0x2030u: out_b = 0x89u; return true;
		case 0x0160u: out_b = 0x8Au; return true;
		case 0x2039u: out_b = 0x8Bu; return true;
		case 0x0152u: out_b = 0x8Cu; return true;
		case 0x017Du: out_b = 0x8Eu; return true;
		case 0x2018u: out_b = 0x91u; return true;
		case 0x2019u: out_b = 0x92u; return true;
		case 0x201Cu: out_b = 0x93u; return true;
		case 0x201Du: out_b = 0x94u; return true;
		case 0x2022u: out_b = 0x95u; return true;
		case 0x2013u: out_b = 0x96u; return true;
		case 0x2014u: out_b = 0x97u; return true;
		case 0x02DCu: out_b = 0x98u; return true;
		case 0x2122u: out_b = 0x99u; return true;
		case 0x0161u: out_b = 0x9Au; return true;
		case 0x203Au: out_b = 0x9Bu; return true;
		case 0x0153u: out_b = 0x9Cu; return true;
		case 0x017Eu: out_b = 0x9Eu; return true;
		case 0x0178u: out_b = 0x9Fu; return true;
		default: return false;
		}
	}

	static std::string Cp874ToUtf8(std::string const& in)
	{
		std::string out;
		out.reserve(in.size() * 2);

		for (unsigned char b : in)
		{
			uint32_t cp = 0;
			if (b < 0x80u)
			{
				cp = b;
			}
			else if (b >= 0x80u && b <= 0x9Fu)
			{
				cp = Cp1252ToUnicode(b);
			}
			else if (b == 0xA0u)
			{
				cp = 0x00A0u; // NBSP
			}
			else if (b >= 0xA1u && b <= 0xFBu)
			{
				// Windows-874 maps 0xA1..0xFB to Unicode Thai block U+0E01..U+0E5B.
				cp = 0x0E01u + (b - 0xA1u);
			}
			else if (b >= 0xFCu)
			{
				// Remaining 0xFC..0xFF keep Latin-1.
				cp = 0x00FCu + (b - 0xFCu);
			}
			else
			{
				// Unspecified byte.
				cp = '?';
			}

			AppendUtf8(out, cp);
		}

		return out;
	}

	static inline bool NextUtf8Codepoint(std::string const& s, size_t& i, uint32_t& cp)
	{
		if (i >= s.size())
			return false;

		const unsigned char c0 = static_cast<unsigned char>(s[i++]);
		if (c0 < 0x80u)
		{
			cp = c0;
			return true;
		}

		// Reject overlongs and invalid sequences; replace with '?'.
		auto need_cont = [&](int n, uint32_t acc, uint32_t min_cp) -> bool
		{
			for (int k = 0; k < n; ++k)
			{
				if (i >= s.size())
					return false;
				const unsigned char cx = static_cast<unsigned char>(s[i++]);
				if ((cx & 0xC0u) != 0x80u)
					return false;
				acc = (acc << 6) | (cx & 0x3Fu);
			}
			if (acc < min_cp || acc > 0x10FFFFu || (acc >= 0xD800u && acc <= 0xDFFFu))
				return false;
			cp = acc;
			return true;
		};

		if ((c0 & 0xE0u) == 0xC0u)
			return need_cont(1, c0 & 0x1Fu, 0x80u);
		if ((c0 & 0xF0u) == 0xE0u)
			return need_cont(2, c0 & 0x0Fu, 0x800u);
		if ((c0 & 0xF8u) == 0xF0u)
			return need_cont(3, c0 & 0x07u, 0x10000u);

		return false;
	}

	static std::string Utf8ToCp874(std::string const& in)
	{
		std::string out;
		out.reserve(in.size());

		size_t i = 0;
		while (i < in.size())
		{
			uint32_t cp = 0;
			size_t prev = i;
			if (!NextUtf8Codepoint(in, i, cp))
			{
				// Invalid UTF-8 byte, consume one and replace.
				i = prev + 1;
				out.push_back('?');
				continue;
			}

			if (cp <= 0x7Fu)
			{
				out.push_back(static_cast<char>(cp));
				continue;
			}

			// Thai block
			if (cp >= 0x0E01u && cp <= 0x0E5Bu)
			{
				out.push_back(static_cast<char>(0xA1u + (cp - 0x0E01u)));
				continue;
			}

			// NBSP
			if (cp == 0x00A0u)
			{
				out.push_back(static_cast<char>(0xA0u));
				continue;
			}

			// Keep 0xFC..0xFF Latin-1
			if (cp >= 0x00FCu && cp <= 0x00FFu)
			{
				out.push_back(static_cast<char>(0xFCu + (cp - 0x00FCu)));
				continue;
			}

			// Windows-1252 special punctuation range
			unsigned char b = 0;
			if (UnicodeToCp1252(cp, b))
			{
				out.push_back(static_cast<char>(b));
				continue;
			}

			// Not representable in Windows-874.
			out.push_back('?');
		}

		return out;
	}

	std::string FromPawnEncoding(std::string s)
	{
		if (GetPawnEncoding() == PawnEncoding::WINDOWS_874)
			return Cp874ToUtf8(s);
		return s;
	}

	std::string ToPawnEncoding(std::string const& utf8)
	{
		if (GetPawnEncoding() == PawnEncoding::WINDOWS_874)
			return Utf8ToCp874(utf8);
		return utf8;
	}
}

