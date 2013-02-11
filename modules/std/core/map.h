/**
 * Clever programming language
 * Copyright (c) Clever Team
 *
 * This file is distributed under the MIT license. See LICENSE for details.
 */

#ifndef CLEVER_MAP_H
#define CLEVER_MAP_H

#include <map>
#include <utility>
#include "core/cstring.h"
#include "core/value.h"
#include "types/type.h"

namespace clever {

typedef std::pair<std::string, Value*> MapObjectPair;

class MapObject : public TypeObject{
public:
	MapObject() {}

	~MapObject() {}

	std::map<std::string, Value*>& getData() { return m_data; }
private:
	std::map<std::string, Value*> m_data;

	DISALLOW_COPY_AND_ASSIGN(MapObject);
};

class MapType : public Type {
public:
	MapType()
		: Type(CSTRING("Map")) {}

	~MapType() {}

	void init();

	TypeObject* allocData(CLEVER_TYPE_CTOR_ARGS) const;
	void deallocData(void*);

	void dump(TypeObject*, std::ostream&) const;

	CLEVER_METHOD(ctor);
	CLEVER_METHOD(insert);
	CLEVER_METHOD(each);
	CLEVER_METHOD(size);
private:
	DISALLOW_COPY_AND_ASSIGN(MapType);
};

} // clever

#endif // CLEVER_MAP_H
