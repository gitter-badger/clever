/**
 * Clever programming language
 * Copyright (c) Clever Team
 *
 * This file is distributed under the MIT license. See LICENSE for details.
 */

#ifndef CLEVER_STD_MATH_H
#define CLEVER_STD_MATH_H

#include "core/module.h"
#include "core/type.h"

namespace clever { namespace modules { namespace std {

/// Standard Math Module
class Math : public Module {
public:
	Math()
		: Module("std.math") {}

	~Math() {}

	CLEVER_MODULE_VIRTUAL_METHODS_DECLARATION;
private:
	DISALLOW_COPY_AND_ASSIGN(Math);
};

}}} // clever::modules::std

#endif // CLEVER_STD_MATH_H
