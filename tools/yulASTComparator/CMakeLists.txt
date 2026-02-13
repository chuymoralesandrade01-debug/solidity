add_library(yulASTComparatorLib
	ASTComparator.cpp
	ASTComparator.h
	ScopeBimap.h
)
target_link_libraries(yulASTComparatorLib PUBLIC solidity)

add_executable(yulASTComparator main.cpp)
target_link_libraries(yulASTComparator PRIVATE yulASTComparatorLib)
