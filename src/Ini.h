// Small INI parsing helpers shared by Host and CrateGood (comma lists, ints).
#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <cctype>
#include <cstdlib>

namespace CrateGoodExt::Ini
{
	inline bool IsNoneToken(const std::string& s)
	{
		return s.empty() || _stricmp(s.c_str(), "none") == 0 || _stricmp(s.c_str(), "<none>") == 0;
	}

	inline std::string Trim(const std::string& s)
	{
		size_t a = 0, b = s.size();
		while (a < b && std::isspace((unsigned char)s[a])) ++a;
		while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
		return s.substr(a, b - a);
	}

	inline std::vector<std::string> SplitList(const char* raw)
	{
		std::vector<std::string> out;
		std::string cur;
		for (const char* p = raw; ; ++p)
		{
			if (*p == ',' || *p == '\0')
			{
				std::string tok = Trim(cur);
				if (!IsNoneToken(tok)) out.push_back(tok);
				cur.clear();
				if (*p == '\0') break;
			}
			else cur.push_back(*p);
		}
		return out;
	}

	inline std::vector<int> SplitInts(const char* raw)
	{
		std::vector<int> out;
		for (const std::string& tok : SplitList(raw))
			out.push_back(atoi(tok.c_str()));
		return out;
	}

	// Parses a chance list into [0,1] doubles. Accepts "50%" or "0.5" (values > 1
	// without a '%' are still treated as percentages, matching common mod usage).
	inline std::vector<double> SplitChances(const char* raw)
	{
		std::vector<double> out;
		for (const std::string& tok : SplitList(raw))
		{
			bool pct = tok.find('%') != std::string::npos;
			double v = atof(tok.c_str());
			if (pct || v > 1.0) v /= 100.0;
			if (v < 0.0) v = 0.0;
			out.push_back(v);
		}
		return out;
	}
}
