#pragma once

#include <string>
#include <vector>

#include "CSingleton.hpp"

using std::string;
using std::vector;


class SampConfigReader : public CSingleton<SampConfigReader>
{
	friend class CSingleton<SampConfigReader>;
private:
	SampConfigReader();
	~SampConfigReader() = default;

public:
	bool GetVar(string varname, string &dest);
	bool GetVarList(string varname, vector<string> &dest);
	bool GetGamemodeList(vector<string> &dest);

private:
	vector<string> m_FileContent;
};
