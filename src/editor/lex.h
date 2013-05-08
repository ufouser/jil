#ifndef JIL_EDITOR_LEX_H_
#define JIL_EDITOR_LEX_H_
#pragma once

#include <string>

namespace jil {
namespace editor {

namespace lex {

struct Group {
  std::string name;

  // Lexical group names.
  static const std::string kComment;
  static const std::string kOperator;
  static const std::string kConstant;
  static const std::string kIdentifier;
  static const std::string kStatement;
  static const std::string kPackage;
  static const std::string kPreProc;
  static const std::string kType;
  static const std::string kSpecial;
  static const std::string kError;
  static const std::string kTodo;
};

} // namespace lex

/*
const char* const kSubGroupString = "string"; // "string"
const char* const kSubGroupChar = "char"; // 'c', '\n'
const char* const kSubGroupNumber = "number"; // 123, 0xff
const char* const kSubGroupFloat = "float"; // 3.14, 1.23e10
const char* const kSubGroupBool = "bool"; // true, false
const char* const kSubGroupNull = "null"; // NULL, nil, Null, etc.

const char* const kSubGroupFunction = "function"; // Function name, class method name, etc.

const char* const kSubGroupConditional = "conditional"; // if, then, else, endif, switch, etc.
const char* const kSubGroupRepeat = "repeat"; // for, do, while, etc.
const char* const kSubGroupLabel = "label"; // case, default, etc.
const char* const kSubGroupKeyword = "keyword"; // any other keyword
const char* const kSubGroupException = "exception"; // try, catch, throw, except, etc.

const char* const kSubGroupImport = "import"; // from, import, etc.
const char* const kSubGroupExport = "export"; // ?

const char* const kSubGroupInclude = "include"; // #include
const char* const kSubGroupMacro = "macro"; // #define
const char* const kSubGroupPreConditional = "preconditional"; // #if, #else, #endif, etc.

}
*/
//*Type			int, long, char, etc.
// StorageClass	static, register, auto, volatile, mutable, etc.
// Structure		struct, union, enum, class, namespace, etc.
//
//*Special		any special symbol
// Space		white space
// Tab			tab

} } // namespace jil::editor

#endif // JIL_EDITOR_LEX_H_
