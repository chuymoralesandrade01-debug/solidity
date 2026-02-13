/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0

/// Compares two Yul object trees structurally, treating variable and  user-defined function names as equivalent
/// if they correspond 1:1 (tracked via a scoped bidirectional map). Prints a diff at the first point of divergence.

#include <tools/yulASTComparator/ASTComparator.h>

#include <libyul/AST.h>
#include <libyul/Object.h>
#include <libyul/ObjectParser.h>
#include <libyul/Dialect.h>
#include <libyul/backends/evm/EVMDialect.h>

#include <liblangutil/CharStream.h>
#include <liblangutil/ErrorReporter.h>
#include <liblangutil/EVMVersion.h>
#include <liblangutil/Scanner.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>


using namespace solidity;
using namespace solidity::yul;
using namespace solidity::langutil;

static std::string readFile(std::string_view _path)
{
	std::filesystem::path const file(_path);
	if (!std::filesystem::exists(file) || !std::filesystem::is_regular_file(file))
		throw std::runtime_error("File does not exist: " + std::string(_path));
	std::ifstream is(file, std::ios::binary | std::ios::ate);
	if (!is)
		throw std::runtime_error("Failed to open file: " + std::string(_path));
	auto size = is.tellg();
	std::string result(static_cast<std::size_t>(size), '\0');
	is.seekg(0);
	if (!is.read(result.data(), size))
		throw std::runtime_error("Failed to read file: " + std::string(_path));
	return result;
}

static std::shared_ptr<Object> parseYulFile(std::string_view const _path)
{
	std::string source = readFile(_path);
	Dialect const& dialect = EVMDialect::strictAssemblyForEVMObjects(EVMVersion::current(), std::nullopt);
	ErrorList errors;
	ErrorReporter errorReporter(errors);
	auto const charStream = std::make_shared<CharStream>(source, std::string(_path));
	auto const scanner = std::make_shared<Scanner>(*charStream);
	auto object = ObjectParser(errorReporter, dialect).parse(scanner, false);
	if (!object || errorReporter.hasErrors())
	{
		std::cerr << "Parse errors in " << _path << " (" << errors.size() << " error(s))\n";
		return nullptr;
	}
	return object;
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: yulASTComparator <file1.yul> <file2.yul>\n";
		return EXIT_FAILURE;
	}

	auto objA = parseYulFile(argv[1]);
	auto objB = parseYulFile(argv[2]);

	if (!objA || !objB)
	{
		std::cerr << "Aborting due to parse errors.\n";
		return EXIT_FAILURE;
	}

	Dialect const* dialect = objA->dialect();
	if (!dialect)
	{
		std::cerr << "No dialect available.\n";
		return EXIT_FAILURE;
	}

	tools::cmpast::ASTComparator cmp(*dialect);
	if (cmp.compareObjects(*objA, *objB))
	{
		std::cout << "EQUIVALENT\n";
		return EXIT_SUCCESS;
	}
	else
	{
		std::cout << "MISMATCH\n";
		std::cout << "  at:     " << cmp.mismatchPath() << "\n";
		std::cout << "  reason: " << cmp.mismatchReason() << "\n";
		if (!cmp.mismatchLHS().empty())
		{
			std::cout << "\n  --- LHS ---\n" << cmp.mismatchLHS() << "\n";
			std::cout << "\n  --- RHS ---\n" << cmp.mismatchRHS() << "\n";
		}
		return EXIT_FAILURE;
	}
}
