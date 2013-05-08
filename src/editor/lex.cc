#include "editor/lex.h"

namespace jil {
namespace editor {


namespace lex {\

const std::string Group::kComment = "comment";
const std::string Group::kOperator = "operator";
const std::string Group::kConstant = "constant";
const std::string Group::kIdentifier = "identifier";
const std::string Group::kStatement = "statement";
const std::string Group::kPackage = "package";
const std::string Group::kPreProc = "preproc";
const std::string Group::kType = "type";
const std::string Group::kSpecial = "special";
const std::string Group::kError = "error";
const std::string Group::kTodo = "todo";

}

} } // namespace jil::editor
