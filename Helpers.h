#pragma once
#include <string>
#include <map>
#include <windows.h>

using namespace std;

namespace GP4PP
{
	struct CGP4MeshShaderData {
		DWORD* shader;
		DWORD params[8];
	};

	inline void OutputGP4PPDebugString(const string& message)
	{
		OutputDebugStringA(("GP4++: " + message).c_str());
	}

	inline string replaceVariables(const string& input, const map<string, string>& replacements) {
		string result = input;
		size_t startPos = 0;

		// Loop through the string to find occurrences of variables
		while ((startPos = result.find('{', startPos)) != string::npos) {
			size_t endPos = result.find('}', startPos);
			if (endPos == string::npos) {
				// No closing brace found, stop processing
				break;
			}

			// Extract the variable name (between the curly braces)
			string varName = result.substr(startPos + 1, endPos - startPos - 1);

			// Check if the variable is in the replacements map
			if (replacements.find(varName) != replacements.end()) {
				// Replace the entire {varName} with the corresponding value
				result.replace(startPos, endPos - startPos + 1, replacements.at(varName));

				// Move past the current replaced text to search for the next variable
				endPos = startPos + replacements.at(varName).length();
			}
			else
			{
				// Move past the current variable to search for the next one
				startPos = endPos + 1;
			}
		}

		return result;
	}
};