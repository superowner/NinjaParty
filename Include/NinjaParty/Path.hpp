#ifndef NINJAPARTY_PATH_HPP
#define NINJAPARTY_PATH_HPP

#include <string>
#include <vector>

namespace NinjaParty
{
	namespace Path
	{
		// split a string on an input delimiter
		void split(std::vector<std::string>& output, const std::string &input, const std::string &delimiter);

		// combine two paths, the second relative to the first
		// path1: foo/bar/blah
		// path2: ../../asdf/fdsa
		// returns: foo/asdf/fdsa
		std::string CombinePaths(const std::string &path1, const std::string &path2);
		
		// gets the path to the specified file
		// path: foo/bar/blah/file.foo
		// returns: foo/bar/blah/
		std::string GetPathName(const std::string &path);
	}
}

#endif//NINJAPARTY_PATH_HPP
