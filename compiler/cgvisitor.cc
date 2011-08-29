/**
 * Clever programming language
 * Copyright (c) 2011 Clever Team
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#include "cgvisitor.h"
#include "compiler/compiler.h"
#include "types/typeutils.h"
#include "compiler/typechecker.h"

namespace clever { namespace ast {



/**
 * Return the Value pointer related to value type
 */
Value* CodeGenVisitor::getValue(ASTNode* expr) throw() {
	Value* value = expr->getValue();

	if (value && value->hasName()) {
		Value* var = g_symtable.getValue(value->getName());

		/**
		 * If the variable is found, we should use its pointer instead of
		 * fetching on runtime
		 */
		if (EXPECTED(var != NULL)) {
			return var;
		}
		Compiler::error("Inexistent variable!", expr->getLocation());
	}
	return value;
}

/**
 * Creates a vector with the current value from a Value* pointers
 */
void CodeGenVisitor::functionArgs(ArgumentList* args) throw() {
	const NodeList& nodes = args->getNodes();
	NodeList::const_iterator it = nodes.begin(), end = nodes.end();

	while (it != end) {
		(*it)->accept(*this);
		++it;
	}
}

AST_VISITOR(CodeGenVisitor, Identifier) {
}

/**
 * Generates opcode for binary expression
 */
AST_VISITOR(CodeGenVisitor, BinaryExpr) {
	Value* lhs = expr->getLhs()->getValue();
	Value* rhs = expr->getRhs()->getValue();

	lhs->addRef();
	rhs->addRef();
	
	switch (expr->getOp()) {
		case PLUS:  emit(OP_PLUS,   &VM::plus_handler,   lhs, rhs, expr->getValue()); break;
		case DIV:   emit(OP_DIV,    &VM::div_handler,    lhs, rhs, expr->getValue()); break;
		case MULT:  emit(OP_MULT,   &VM::mult_handler,   lhs, rhs, expr->getValue()); break;
		case MINUS: emit(OP_MINUS,  &VM::minus_handler,  lhs, rhs, expr->getValue()); break;
		case XOR:   emit(OP_BW_XOR, &VM::bw_xor_handler, lhs, rhs, expr->getValue()); break;
		case OR:    emit(OP_BW_OR,  &VM::bw_or_handler,  lhs, rhs, expr->getValue()); break;
		case AND:   emit(OP_BW_AND, &VM::bw_and_handler, lhs, rhs, expr->getValue()); break;
		case MOD:   emit(OP_MOD,    &VM::mod_handler,    lhs, rhs, expr->getValue()); break;
	}
}


/**
 * Generates the variable declaration opcode
 */
AST_VISITOR(CodeGenVisitor, VariableDecl) {
	Identifier* var_expr = expr->getVariable();
	ASTNode* rhs_expr    = expr->getInitialValue();
	Value* variable      = var_expr->getValue();
	
	/* Check if the declaration contains initialization */
	if (rhs_expr) {
		Value* value = getValue(rhs_expr);
		
		if (!TypeChecker::checkCompatibleTypes(variable, value)) {
			Compiler::errorf(expr->getLocation(),
				"Cannot convert `%S' to `%S' on assignment",
				value->getTypePtr()->getName(),
				variable->getTypePtr()->getName());
		}

		variable->addRef();
		g_symtable.push(variable);

		if (value->isPrimitive()) {
			variable->copy(value);
		}

		variable->addRef();
		value->addRef();

		emit(OP_VAR_DECL, &VM::var_decl_handler, variable, value);
	} else {
		/* TODO: fix this */
		/*
		if (type == CLEVER_TYPE("Int")) {
			variable->setType(Value::INTEGER);
		}
		else if (type == CLEVER_TYPE("Double")) {
			variable->setType(Value::DOUBLE);
		}
		else if (type == CLEVER_TYPE("String")) {
			variable->setType(Value::STRING);
		}
		else {
			variable->setType(Value::USER);
			variable->setDataValue(type->allocateValue());	
		}*/

		variable->addRef();
		variable->initialize();

		emit(OP_VAR_DECL, &VM::var_decl_handler, variable);
	}
}

/**
 * Generates the pre increment opcode
 */
AST_VISITOR(CodeGenVisitor, PreIncrement) {
	Value* value = expr->getVar();

	value->addRef();

	emit(OP_PRE_INC, &VM::pre_inc_handler, value, NULL, expr->getValue());
	expr->getValue()->setTypePtr(value->getTypePtr());
}

/**
 * Generates the pos increment opcode
 */
AST_VISITOR(CodeGenVisitor, PosIncrement) {
	Value* value = expr->getVar();

	value->addRef();

	emit(OP_POS_INC, &VM::pos_inc_handler, value, NULL, expr->getValue());
	expr->getValue()->setTypePtr(value->getTypePtr());
}

/**
 * Generates the pre decrement opcode
 */
AST_VISITOR(CodeGenVisitor, PreDecrement) {
	Value* value = expr->getVar();

	value->addRef();

	emit(OP_PRE_DEC, &VM::pre_dec_handler, value, NULL, expr->getValue());
	expr->getValue()->setTypePtr(value->getTypePtr());
}

/**
 * Generates the pos decrement opcode
 */
AST_VISITOR(CodeGenVisitor, PosDecrement) {
	Value* value = expr->getVar();

	value->addRef();

	emit(OP_POS_DEC, &VM::pos_dec_handler, value, NULL, expr->getValue());
	expr->getValue()->setTypePtr(value->getTypePtr());
}

/**
 * Generates the opcode for the IF-ELSEIF-ELSE expression
 */
AST_VISITOR(CodeGenVisitor, IfExpr) {
	Value* value;
	Opcode* jmp_if;
	Opcode* jmp_else;
	Opcode* jmp_elseif;
	OpcodeList jmp_ops;

	expr->getCondition()->accept(*this);

	value = getValue(expr->getCondition());
	value->addRef();

	jmp_if = emit(OP_JMPZ, &VM::jmpz_handler);
	jmp_if->setOp1(value);

	jmp_ops.push_back(jmp_if);

	if (expr->hasBlock()) {
		expr->getBlock()->accept(*this);
	}

	if (expr->hasElseIf()) {
		NodeList& elseif_nodes = expr->getNodes();
		NodeList::const_iterator it = elseif_nodes.begin(), end = elseif_nodes.end();
		Opcode* last_jmp = jmp_if;

		while (it != end) {
			Value* cond;
			ElseIfExpr* elseif = static_cast<ElseIfExpr*>(*it);

			last_jmp->setJmpAddr1(getOpNum());

			elseif->getCondition()->accept(*this);

			cond = getValue(elseif->getCondition());
			cond->addRef();

			jmp_elseif = emit(OP_JMPZ, &VM::jmpz_handler, cond);

			jmp_ops.push_back(jmp_elseif);

			if (elseif->hasBlock()) {
				elseif->getBlock()->accept(*this);
			}

			last_jmp = jmp_elseif;
			++it;
		}
	}

	if (expr->hasElseBlock()) {
		jmp_else = emit(OP_JMP, &VM::jmp_handler);

		if (jmp_ops.size() == 1) {
			jmp_if->setJmpAddr1(getOpNum());
		}

		jmp_ops.push_back(jmp_else);

		expr->getElse()->accept(*this);
	}

	if (jmp_ops.size() == 1) {
		jmp_if->setJmpAddr1(getOpNum());
		jmp_if->setJmpAddr2(getOpNum());
	} else {
		OpcodeList::iterator it = jmp_ops.begin(), end = jmp_ops.end();

		while (it != end) {
			(*it)->setJmpAddr2(getOpNum());
			++it;
		}
	}
}

/**
 * Call the accept method of each block node
 */
AST_VISITOR(CodeGenVisitor, BlockNode) {
	NodeList& nodes = expr->getNodes();
	NodeList::const_iterator it = nodes.begin(), end = nodes.end();

	/**
	 * Create a new scope
	 */
	g_symtable.beginScope();

	/**
	 * Iterates statements inside the block
	 */
	while (it != end) {
		(*it)->accept(*this);
		++it;
	}

	/**
	 * Pops the scope
	 */
	g_symtable.endScope();
}

/**
 * Generates the JMPZ opcode for WHILE expression
 */
AST_VISITOR(CodeGenVisitor, WhileExpr) {
	Value* value;
	Opcode* jmpz;
	Opcode* jmp;
	unsigned int start_pos = 0;

	start_pos = getOpNum();

	expr->getCondition()->accept(*this);

	value = getValue(expr->getCondition());
	value->addRef();

	jmpz = emit(OP_JMPZ, &VM::jmpz_handler, value);

	if (expr->hasBlock()) {
		m_brks.push(OpcodeStack());

		expr->getBlock()->accept(*this);

		/**
		 * Points break statements to out of WHILE block
		 */
		while (!m_brks.top().empty()) {
			m_brks.top().top()->setJmpAddr1(getOpNum()+1);
			m_brks.top().pop();
		}
		m_brks.pop();
	}

	jmp = emit(OP_JMP, &VM::jmp_handler);
	jmp->setJmpAddr2(start_pos);

	jmpz->setJmpAddr1(getOpNum());
}

/**
 * Generates the opcode for FOR expression
 */
AST_VISITOR(CodeGenVisitor, ForExpr) {
	Value* value;
	Opcode* jmpz;
	Opcode* jmp;
	unsigned int start_pos = 0;

	if (!expr->isIteratorMode()) {
		
		if (expr->getVarDecl() != NULL) {
			expr->getVarDecl()->accept(*this);
		}
		
		start_pos = getOpNum();
		
		if (expr->getCondition()) {
			expr->getCondition()->accept(*this);
			
			value = getValue(expr->getCondition());
			value->addRef();
		}
		else {
			value = new Value(true);
		}
		
		jmpz = emit(OP_JMPZ, &VM::jmpz_handler, value);
		
		// If the expression has increment we must jump 2 opcodes
		unsigned int offset = (expr->getIncrement() ? 2 : 1);
		if (expr->hasBlock()) {
			m_brks.push(OpcodeStack());

			expr->getBlock()->accept(*this);

			/**
			 * Points break statements to out of FOR block
			 */
			while (!m_brks.top().empty()) {
				m_brks.top().top()->setJmpAddr1(getOpNum() + offset);
				m_brks.top().pop();
			}
			
			m_brks.pop();
		}
		
		if (expr->getIncrement() != NULL) {
			expr->getIncrement()->accept(*this);
		}
		
		jmp = emit(OP_JMP, &VM::jmp_handler);
		jmp->setJmpAddr2(start_pos);

		jmpz->setJmpAddr1(getOpNum());
	}
}


/**
 * Generates opcode for logic expression which weren't optimized
 */
AST_VISITOR(CodeGenVisitor, LogicExpr) {
	Opcode* opcode;
	Value* lhs;
	Value* rhs;

	expr->getLhs()->accept(*this);
	expr->getRhs()->accept(*this);

	lhs = getValue(expr->getLhs());
	rhs = getValue(expr->getRhs());
	
	if (!TypeChecker::checkCompatibleTypes(rhs, lhs)) {
		Compiler::errorf(expr->getLocation(),
			"Cannot convert `%s' to `%s' in logic expression",			
			rhs->getTypePtr()->getName(),
			lhs->getTypePtr()->getName());
	}

	expr->setResult(new Value(lhs->getTypePtr()));

	lhs->addRef();
	rhs->addRef();

	switch (expr->getOp()) {
		case GREATER:       emit(OP_GREATER,       &VM::greater_handler,       lhs, rhs, expr->getValue()); break;
		case LESS:          emit(OP_LESS,          &VM::less_handler,          lhs, rhs, expr->getValue()); break;
		case GREATER_EQUAL: emit(OP_GREATER_EQUAL, &VM::greater_equal_handler, lhs, rhs, expr->getValue()); break;
		case LESS_EQUAL:    emit(OP_LESS_EQUAL,    &VM::less_equal_handler,    lhs, rhs, expr->getValue()); break;
		case EQUAL:         emit(OP_EQUAL,         &VM::equal_handler,         lhs, rhs, expr->getValue()); break;
		case NOT_EQUAL:     emit(OP_NOT_EQUAL,     &VM::not_equal_handler,     lhs, rhs, expr->getValue()); break;
		case AND:
			opcode = emit(OP_JMPNZ, &VM::jmpz_handler, lhs, NULL, expr->getValue());
			opcode->setJmpAddr1(getOpNum()+1);
			opcode = emit(OP_JMPZ, &VM::jmpz_handler, rhs, NULL, expr->getValue());
			opcode->setJmpAddr1(getOpNum());
			expr->getValue()->addRef();
			break;
		case OR:
			opcode = emit(OP_JMPNZ, &VM::jmpnz_handler, lhs, NULL, expr->getValue());
			opcode->setJmpAddr1(getOpNum()+1);
			opcode = emit(OP_JMPNZ, &VM::jmpnz_handler, rhs, NULL, expr->getValue());
			opcode->setJmpAddr1(getOpNum());
			expr->getValue()->addRef();
			break;
	}
}

/**
 * Generates opcode for break statement
 */
AST_VISITOR(CodeGenVisitor, BreakNode) {
	Opcode* opcode = emit(OP_BREAK, &VM::break_handler);

	/**
	 * Pushes the break opcode to a stack which in the end
	 * sets its jump addr to end of repeat block
	 */
	m_brks.top().push(opcode);
}

/**
 * Generates opcode for function call
 */
AST_VISITOR(CodeGenVisitor, FunctionCall) {
	CallableValue* fvalue = expr->getFuncValue();
	Function* func;
	Value* arg_values = expr->getArgsValue();

	func = fvalue->getFunction();

	if (arg_values) {
		functionArgs(expr->getArgs());
		
		if (func->isUserDefined()) {
			Value* vars = func->getVars();

			emit(OP_RECV, &VM::arg_recv_handler, vars, arg_values);
			arg_values = NULL;
		}
	}

	fvalue->addRef();
	emit(OP_FCALL, &VM::fcall_handler, fvalue, arg_values, expr->getValue());
}

/**
 * Generates opcode for method call
 */
AST_VISITOR(CodeGenVisitor, MethodCall) {
	Value* variable = getValue(expr->getVariable());
	CallableValue* call = new CallableValue(expr->getMethodName());
	const Method* method = variable->getTypePtr()->getMethod(call->getName());
	ASTNode* args = expr->getArgs();
	
	if (!method) {
		Compiler::errorf(expr->getLocation(), "Method `%s::%S' not found!",
			variable->getTypePtr()->getName(), call->getName());
	}

	call->setTypePtr(variable->getTypePtr());
	call->setHandler(method);
	call->setContext(variable);
	
	expr->getValue()->setTypePtr(method->getReturn());

	Value* arg_values = NULL;
	if (args) {
		arg_values = new Value;
		arg_values->setType(Value::VECTOR);
		//arg_values->setVector(functionArgs(static_cast<ArgumentList*>(args)));
	}
	
	if (!checkArgs(method->getArgs(), arg_values ? arg_values->getVector() : NULL)) {
		Compiler::errorf(expr->getLocation(), "No matching call for %s::%S%s", 
			variable->getTypePtr()->getName(), call->getName(), 
			argsError(method->getArgs(), arg_values ? arg_values->getVector() : NULL).c_str());
	}
	
	emit(OP_MCALL, &VM::mcall_handler, call, arg_values, expr->getValue());
}

/**
 * Generates opcode for variable assignment
 */
AST_VISITOR(CodeGenVisitor, AssignExpr) {
	Value* lhs = expr->getLhs()->getValue();
	Value* rhs = expr->getRhs()->getValue();

	lhs->addRef();
	rhs->addRef();

	emit(OP_ASSIGN, &VM::assign_handler, lhs, rhs);
}

/**
 * Import statement
 */
AST_VISITOR(CodeGenVisitor, ImportStmt) {

}

/**
 * Function declaration
 */
AST_VISITOR(CodeGenVisitor, FuncDeclaration) {
	CallableValue* func = expr->getFunc();
	Function* user_func = func->getFunction();
	Opcode* jmp;

	jmp = emit(OP_JMP, &VM::jmp_handler);

	user_func->setOffset(getOpNum());

	m_funcs.push(user_func);

	expr->getBlock()->accept(*this);

	m_funcs.pop();

	emit(OP_JMP, &VM::end_func_handler);

	jmp->setJmpAddr2(getOpNum());
}

/**
 * Generates opcode for return statement
 */
AST_VISITOR(CodeGenVisitor, ReturnStmt) {
	Value* expr_value = expr->getExprValue();
	const Function* func = m_funcs.empty() ? NULL : m_funcs.top();

	/**
	 * Only for return inside function declaration
	 */
	if (func) {
		TypeChecker::checkFunctionReturn(func, expr_value, func->getReturn(), expr->getLocation());
	}

	if (expr_value) {
		expr_value->addRef();
	}

	emit(OP_RETURN, &VM::return_handler, expr_value);
}

/**
 * Generates opcodes for class declaration
 */
AST_VISITOR(CodeGenVisitor, ClassDeclaration) {
	UserTypeValue* type = new UserTypeValue(expr->getClassName());
	ClassDeclaration::DeclarationError error;	
	bool ok = expr->check(error);
	const CString* const class_name = expr->getClassName();

	if (ok) {
		//@TODO
		
		g_symtable.push(type);
	} else {
		Compiler::errorf(error.getLocation(),
			"Redefinition of %s `%S' in class `%S'",
			(error.getType() == ClassDeclaration::DeclarationError::METHOD_DECLARATION) ?
				"method" : "attribute",
			&error.getIdentifier(),
			class_name
		);
	}
}

}} // clever::ast