#pragma once

#include <tools/yulASTComparator/ScopeBimap.h>

#include <libyul/Dialect.h>
#include <libyul/Object.h>

#include <fmt/format.h>

#include <string>
#include <variant>

namespace solidity::tools::cmpast
{

class ASTComparator
{
public:

	explicit ASTComparator(yul::Dialect const& _dialect);

	bool compareObjects(yul::Object const& _a, yul::Object const& _b);

	std::string const& mismatchPath() const { return m_mismatchPath; }
	std::string const& mismatchReason() const { return m_mismatchReason; }
	std::string const& mismatchLHS() const { return m_mismatchLHS; }
	std::string const& mismatchRHS() const { return m_mismatchRHS; }

private:
	struct Path
	{
		explicit Path(ASTComparator& _comparator, std::string _segment);
		~Path();

		ASTComparator& m_comparator;
	};

	std::string currentPath() const;

	bool fail(std::string _reason);

	template<typename T>
	bool fail(std::string _reason, T const& _a, T const& _b)
	{
		yul::AsmPrinter printer(m_dialect, std::nullopt, langutil::DebugInfoSelection::None());
		m_mismatchPath = currentPath();
		m_mismatchReason = std::move(_reason);
		m_mismatchLHS = printer(_a);
		m_mismatchRHS = printer(_b);
		return false;
	}
	template<typename... T>
	bool fail(std::string _reason, std::variant<T...> const& _a, std::variant<T...> const& _b)
	{
		yulAssert(_a.index() == _b.index());
		return std::visit([&]<typename U>(U const& _aInstance) {
			return fail(_reason, _aInstance, std::get<U>(_b));
		}, _a);
	}

	template<typename... T>
	bool compare(std::variant<T...> const& _a, std::variant<T...> const& _b)
	{
		if (_a.index() != _b.index())
			return fail(fmt::format("type mismatch (index {} vs {})", _a.index(), _b.index()));

		auto const same = std::visit([&]<typename U>(U const& _statementA) {
			auto const& statementB = std::get<U>(_b);
			return compare(_statementA, statementB);
		}, _a);
		if (!same)
			return false;
		return true;
	}

	bool compare(yul::Block const& _a, yul::Block const& _b);
	bool compare(yul::ExpressionStatement const& _a, yul::ExpressionStatement const& _b);
	bool compare(yul::Assignment const& _a, yul::Assignment const& _b);
	bool compare(yul::VariableDeclaration const& _a, yul::VariableDeclaration const& _b);
	bool compare(yul::FunctionDefinition const& _a, yul::FunctionDefinition const& _b);
	bool compare(yul::If const& _a, yul::If const& _b);
	bool compare(yul::Switch const& _a, yul::Switch const& _b);
	bool compare(yul::ForLoop const& _a, yul::ForLoop const& _b);
	static bool compare(yul::Break const& _a, yul::Break const& _b);
	static bool compare(yul::Continue const& _a, yul::Continue const& _b);
	static bool compare(yul::Leave const& _a, yul::Leave const& _b);
	bool compare(yul::BuiltinName const& _a, yul::BuiltinName const& _b);
	bool compare(yul::FunctionCall const& _a, yul::FunctionCall const& _b);
	bool compare(yul::Identifier const& _a, yul::Identifier const& _b);
	bool compare(yul::Literal const& _a, yul::Literal const& _b);

	yul::Dialect const& m_dialect;
	ScopeBimap m_bimap;
	std::vector<std::string> m_pathStack;
	std::string m_mismatchPath;
	std::string m_mismatchReason;
	std::string m_mismatchLHS;
	std::string m_mismatchRHS;
};

}

