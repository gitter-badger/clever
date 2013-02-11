/**
 * Clever programming language
 * Copyright (c) Clever Team
 *
 * This file is distributed under the MIT license. See LICENSE for details.
 */
#ifndef CLEVER_STD_FFISTRUCT_H
#define CLEVER_STD_FFISTRUCT_H

#include <iostream>
#include <string>
#include <cstdlib>
#include <map>
#include <vector>
#include "core/cstring.h"
#include "types/type.h"
#include "core/module.h"

#ifndef __APPLE__
# include <ffi.h>
#else
# include <ffi/ffi.h>
#endif

namespace clever { namespace modules { namespace std {

typedef ::std::vector<size_t> ExtMemberOffset;
typedef ::std::vector<char> ExtMemberType;
typedef ::std::vector<CString> ExtMemberName;
typedef ::std::map<CString, size_t> ExtMemberMap;

inline size_t _get_offset_ext_type(char t) {
	switch (t) {
		case 'i': return sizeof(int);
		case 'd': return sizeof(double);
		case 'b': return sizeof(char);
		case 's': return sizeof(void*);
		case 'c': return sizeof(void*);
		case 'v': return 0;
		case 'p': return sizeof(void*);
	}
	return 0;
}

class ExtStruct {
public:
	ExtStruct() {}
	void addMember(const CString& member_name, char member_type) {
		size_t acc = 0;
		size_t n = m_member_offset.size();

		if (n > 0) {
			acc = m_member_offset[n - 1];
		}

		m_member_offset.push_back(acc + _get_offset_ext_type(member_type));
		m_member_name.push_back(member_name);
		m_member_type.push_back(member_type);
		m_member_map[member_name] = n;
	}

	size_t getMember(const CString& mn) {
		return m_member_map[mn];
	}

	size_t getOffset(const CString& mn) {
		return getOffset(getMember(mn));
	}

	size_t getOffset(int i) {
		if (i < 0) return 0;
		return m_member_offset[i];
	}

	char getType(size_t i) {
		return m_member_type[i];
	}

	CString& getMemberName(size_t i) {
		return m_member_name[i];
	}

	size_t getNMembers() {
		return m_member_type.size();
	}

	size_t getSize() {
		size_t n = m_member_type.size();

		if (n == 0) return 0;

		return m_member_offset[n-1];
	}

private:
	ExtMemberOffset m_member_offset;
	ExtMemberType m_member_type;
	ExtMemberName m_member_name;
	ExtMemberMap m_member_map;

	DISALLOW_COPY_AND_ASSIGN(ExtStruct);
};

typedef ::std::map<CString, ExtStruct*> ExtStructs;

struct FFIStructData : public TypeObject {
	void* data;
	ExtStruct* m_struct_type;

	FFIStructData() {
		data = 0;
	}
	~FFIStructData() {
		if (data != 0) {
			::std::free(data);
		}
	}

	void setStruct(ExtStructs& structs_map, const CString& struct_type);
	void setMember(int i, const Value* const v);
	void setMember(const CString& member_name, const Value* const v);

	void getMember(Value* result, int i);
	void getMember(Value* result, const CString& member_name);
};


class FFIStruct : public Type {
public:
	FFIStruct()
		: Type(CSTRING("FFIStruct")) {}

	~FFIStruct() {
		ExtStructs::iterator it = m_structs.begin(), end = m_structs.end();

		while (it != end) {
			if (it->second != 0) {
				delete it->second;
			}
			++it;
		}
	}

	void init();

	void dump(const void* data, ::std::ostream& out) const {}

	virtual void increment(Value*, const VM*, CException*) const {}
	virtual void decrement(Value*, const VM*, CException*) const {}

	virtual TypeObject* allocData(CLEVER_TYPE_CTOR_ARGS) const;
	virtual void deallocData(void* data);

	CLEVER_METHOD(ctor);
	CLEVER_METHOD(addMember);
	CLEVER_METHOD(addStruct);
	CLEVER_METHOD(getMember);
	CLEVER_METHOD(setMember);

private:
	ExtStructs m_structs;

	DISALLOW_COPY_AND_ASSIGN(FFIStruct);
};

}}}

#endif