
#include "TypeChecker.h"
#include <iostream>


TypeChecker::TypeChecker(ErrorReporter &reporter)
   : errors(reporter) {}


void TypeChecker::check(ASTNode *root)
{
   if (auto program = dynamic_cast<ProgramNode *>(root))
   {
       checkProgram(program);
   }
}


// ---------------- statements ----------------


void TypeChecker::checkProgram(ProgramNode *node)
{
   for (const auto &stmt : node->statements)
   {
       checkStatement(stmt.get());
   }
}


void TypeChecker::checkStatement(ASTNode *node)
{
   if (auto n = dynamic_cast<DeclNode *>(node))
   {
       checkDecl(n);
       return;
   }
   if (auto n = dynamic_cast<AssignNode *>(node))
   {
       checkAssign(n);
       return;
   }
   if (auto n = dynamic_cast<IfNode *>(node))
   {
       checkIf(n);
       return;
   }
   if (auto n = dynamic_cast<WhileNode *>(node))
   {
       checkWhile(n);
       return;
   }
   if (auto n = dynamic_cast<ForNode *>(node))
   {
       checkFor(n);
       return;
   }
   if (auto n = dynamic_cast<PrintNode *>(node))
   {
       checkPrint(n);
       return;
   }
   if (auto n = dynamic_cast<BlockNode *>(node))
   {
       checkBlock(n);
       return;
   }
}


void TypeChecker::checkDecl(DeclNode *node)
{
   ValueType declaredType = typeFromKeyword(node->varType);
   ValueType valueType = inferType(node->value.get());


   if (!symbols.declare(node->name, declaredType))
   {
       errors.report(node->line, "Variable '" + node->name + "' is already declared in this scope",
                     "Type error");
       return;
   }


   if (!isAssignable(declaredType, valueType))
   {
       errors.report(node->line,
                     "Cannot assign value of type " + valueTypeName(valueType) +
                         " to variable '" + node->name + "' of type " + valueTypeName(declaredType),
                     "Type error");
   }


   // Remember the declared type: the code generator needs it to widen an
   // int value that is being stored in a decimal variable.
   node->inferredType = declaredType;
}


void TypeChecker::checkAssign(AssignNode *node)
{
   ValueType existingType;
   if (!symbols.lookup(node->name, existingType))
   {
       errors.report(node->line, "Variable '" + node->name + "' was not declared before use",
                     "Type error");
       return;
   }


   ValueType valueType = inferType(node->value.get());
   if (!isAssignable(existingType, valueType))
   {
       errors.report(node->line,
                     "Cannot assign value of type " + valueTypeName(valueType) +
                         " to variable '" + node->name + "' of type " + valueTypeName(existingType),
                     "Type error");
   }


   // Same as for declarations: codegen needs the variable's type.
   node->inferredType = existingType;
}


void TypeChecker::checkIf(IfNode *node)
{
   ValueType condType = inferType(node->condition.get());
   if (condType != ValueType::BOOL)
   {
       errors.report(node->line,
                     "Condition in 'যদি' must be a boolean expression, got " + valueTypeName(condType),
                     "Type error");
   }


   symbols.enterScope();
   checkStatement(node->thenBlock.get());
   symbols.exitScope();


   if (node->elseBlock)
   {
       symbols.enterScope();
       checkStatement(node->elseBlock.get());
       symbols.exitScope();
   }
}


void TypeChecker::checkWhile(WhileNode *node)
{
   ValueType condType = inferType(node->condition.get());
   if (condType != ValueType::BOOL)
   {
       errors.report(node->line,
                     "Condition in 'যতক্ষণ' must be a boolean expression, got " + valueTypeName(condType),
                     "Type error");
   }


   symbols.enterScope();
   checkStatement(node->body.get());
   symbols.exitScope();
}


void TypeChecker::checkFor(ForNode *node)
{
   // The range bounds are evaluated in the ENCLOSING scope, before the
   // loop variable exists — so ০ থেকে গণনা refers to an outer গণনা.
   ValueType startType = inferType(node->start.get());
   if (startType != ValueType::INT && startType != ValueType::UNKNOWN)
   {
       errors.report(node->line,
                     "The starting value of 'প্রতি' must be পূর্ণসংখ্যা, got " +
                         valueTypeName(startType),
                     "Type error");
   }


   ValueType endType = inferType(node->end.get());
   if (endType != ValueType::INT && endType != ValueType::UNKNOWN)
   {
       errors.report(node->line,
                     "The ending value of 'প্রতি' must be পূর্ণসংখ্যা, got " +
                         valueTypeName(endType),
                     "Type error");
   }


   if (node->step)
   {
       ValueType stepType = inferType(node->step.get());
       if (stepType != ValueType::INT && stepType != ValueType::UNKNOWN)
       {
           errors.report(node->line,
                         "The 'ধাপ' value must be পূর্ণসংখ্যা, got " + valueTypeName(stepType),
                         "Type error");
       }
       // A step of zero would never finish, so reject it when we can see it.
       if (auto lit = dynamic_cast<IntLiteralNode *>(node->step.get()))
       {
           if (lit->value == 0)
           {
               errors.report(node->line, "The 'ধাপ' value cannot be zero", "Type error");
           }
       }
   }


   // The loop variable belongs to the loop, so it must not reuse the name
   // of a variable that already exists. Python has no block scoping: if we
   // allowed it, the loop would silently overwrite the outer variable in
   // the generated code and our scoping rule would be a lie.
   ValueType existing;
   if (symbols.lookup(node->varName, existing))
   {
       errors.report(node->line,
                     "Loop variable '" + node->varName +
                         "' has the same name as an existing variable; choose another name",
                     "Type error");
   }


   // The loop variable lives in its own scope and is always পূর্ণসংখ্যা.
   symbols.enterScope();
   symbols.declare(node->varName, ValueType::INT);
   node->inferredType = ValueType::INT;
   checkStatement(node->body.get());
   symbols.exitScope();
}


void TypeChecker::checkPrint(PrintNode *node)
{
   // Any type is printable — we just need to make sure the expression
   // itself is well-typed (inferType will report errors internally).
   inferType(node->expression.get());
}


void TypeChecker::checkBlock(BlockNode *node)
{
   for (const auto &stmt : node->statements)
   {
       checkStatement(stmt.get());
   }
}


// ---------------- expression type inference ----------------


// Works out the type of an expression AND records it on the node, so that
// the code generator can see the same conclusion the type checker reached.
ValueType TypeChecker::inferType(ASTNode *node)
{
   ValueType result = ValueType::UNKNOWN;


   if (dynamic_cast<IntLiteralNode *>(node))
   {
       result = ValueType::INT;
   }
   else if (dynamic_cast<DecimalLiteralNode *>(node))
   {
       result = ValueType::DECIMAL;
   }
   else if (dynamic_cast<StringLiteralNode *>(node))
   {
       result = ValueType::TEXT;
   }
   else if (dynamic_cast<BoolLiteralNode *>(node))
   {
       result = ValueType::BOOL;
   }
   else if (auto n = dynamic_cast<IdentifierNode *>(node))
   {
       ValueType type;
       if (symbols.lookup(n->name, type))
       {
           result = type;
       }
       else
       {
           errors.report(n->line, "Variable '" + n->name + "' was not declared before use",
                         "Type error");
           result = ValueType::UNKNOWN;
       }
   }
   else if (auto n = dynamic_cast<BinOpNode *>(node))
   {
       result = inferBinOp(n);
   }
   else if (auto n = dynamic_cast<UnaryOpNode *>(node))
   {
       result = inferUnaryOp(n);
   }


   if (node)
   {
       node->inferredType = result;
   }
   return result;
}


ValueType TypeChecker::inferBinOp(BinOpNode *node)
{
   ValueType left = inferType(node->left.get());
   ValueType right = inferType(node->right.get());
   const std::string &op = node->op;


   bool leftNumeric = (left == ValueType::INT || left == ValueType::DECIMAL);
   bool rightNumeric = (right == ValueType::INT || right == ValueType::DECIMAL);


   // Text joining: "হ্যালো" + "বিশ্ব".  '+' is the only operator text
   // supports, and both sides must be text — no implicit number-to-text
   // conversion, so ("ক" + ১) is an error rather than a surprise.
   if (op == "+" && (left == ValueType::TEXT || right == ValueType::TEXT))
   {
       if (left != ValueType::TEXT || right != ValueType::TEXT)
       {
           errors.report(node->line,
                         "Cannot join " + valueTypeName(left) + " with " + valueTypeName(right) +
                             " using '+'; both sides must be লেখা",
                         "Type error");
           return ValueType::UNKNOWN;
       }
       return ValueType::TEXT;
   }


   // Arithmetic: + - * /
   if (op == "+" || op == "-" || op == "*" || op == "/")
   {
       if (!leftNumeric || !rightNumeric)
       {
           errors.report(node->line,
                         "Arithmetic operator '" + op + "' requires numeric operands, got " +
                             valueTypeName(left) + " and " + valueTypeName(right),
                         "Type error");
           return ValueType::UNKNOWN;
       }


       // Division always produces a decimal, even for two integers:
       // ৭ / ২ is ৩.৫, not ৩. This matches ordinary arithmetic and
       // matches Python's '/' exactly, so the constant folder and the
       // generated code can never disagree.
       if (op == "/")
       {
           return ValueType::DECIMAL;
       }


       // For + - *, if either side is decimal the result is decimal.
       return (left == ValueType::DECIMAL || right == ValueType::DECIMAL)
                  ? ValueType::DECIMAL
                  : ValueType::INT;
   }


   // Relational: < > <= >=
   if (op == "<" || op == ">" || op == "<=" || op == ">=")
   {
       if (!leftNumeric || !rightNumeric)
       {
           errors.report(node->line,
                         "Comparison operator '" + op + "' requires numeric operands, got " +
                             valueTypeName(left) + " and " + valueTypeName(right),
                         "Type error");
       }
       return ValueType::BOOL;
   }


   // Equality: == !=
   if (op == "==" || op == "!=")
   {
       if (left != right && !(leftNumeric && rightNumeric))
       {
           errors.report(node->line,
                         "Cannot compare " + valueTypeName(left) + " with " + valueTypeName(right),
                         "Type error");
       }
       return ValueType::BOOL;
   }


   // Logical: && ||
   if (op == "&&" || op == "||")
   {
       if (left != ValueType::BOOL || right != ValueType::BOOL)
       {
           errors.report(node->line,
                         "Logical operator '" + op + "' requires boolean operands, got " +
                             valueTypeName(left) + " and " + valueTypeName(right),
                         "Type error");
       }
       return ValueType::BOOL;
   }


   return ValueType::UNKNOWN;
}


ValueType TypeChecker::inferUnaryOp(UnaryOpNode *node)
{
   ValueType operandType = inferType(node->operand.get());


   if (node->op == "-")
   {
       if (operandType != ValueType::INT && operandType != ValueType::DECIMAL)
       {
           errors.report(node->line,
                         "Unary '-' requires a numeric operand, got " + valueTypeName(operandType),
                         "Type error");
           return ValueType::UNKNOWN;
       }
       return operandType;
   }


   if (node->op == "!")
   {
       if (operandType != ValueType::BOOL)
       {
           errors.report(node->line,
                         "Unary '!' requires a boolean operand, got " + valueTypeName(operandType),
                         "Type error");
           return ValueType::UNKNOWN;
       }
       return ValueType::BOOL;
   }


   return ValueType::UNKNOWN;
}


// ---------------- assignability rule ----------------


bool TypeChecker::isAssignable(ValueType targetType, ValueType valueType) const
{
   if (targetType == valueType)
       return true;
   // allow widening: int value into a decimal variable
   if (targetType == ValueType::DECIMAL && valueType == ValueType::INT)
       return true;
   return false;
}



