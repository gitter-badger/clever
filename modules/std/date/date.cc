/**
 * Clever programming language
 * Copyright (c) Clever Team
 *
 * This file is distributed under the MIT license. See LICENSE for details.
 */

#include "core/type.h"
#include "core/value.h"
#include "core/clever.h"
#include "modules/std/date/module.h"
#include "modules/std/date/date.h"

namespace clever { namespace modules { namespace std {

::std::string Date::toString(TypeObject* data) const
{
	const DateObject* uvalue = static_cast<DateObject*>(data);
	::std::ostringstream out;

	out << *uvalue->intern;

	return out.str();
}

static inline void clever_date_format(const ::std::vector<Value*>* args,
	const Value* obj, Value* result, bool utc)
{
	DateObject* dobj = clever_get_this(DateObject*);

	if (!dobj->intern) {
		//clever_throw(eventually)
		return;
	}

	const char* format = args->at(0)->getStr()->c_str();
	if (!format) {
		//clever_throw(eventually);
		return;
	}

	struct tm* local = utc ? gmtime(dobj->intern) : localtime(dobj->intern);
	size_t size = 50;
	// Number of times we try to double the array size to see if the string
	// fits in this size.
	int tries = 5;

	do {
		char buffer[size];

		if (strftime(buffer, size, format, local)) {
			result->setStr(new StrObject(new CString(buffer), false));
			return;
		}

		size <<= 1;
	} while (tries--);

	result->setNull();
}

/*
String Date::format(string specstring)
Formats this date object as a string:
specifier	Replaced by								Example
%a			Abbreviated weekday name				Thu
%A			Full weekday name						Thursday
%b			Abbreviated month name					Aug
%B			Full month name							August
%c			Date and time representation			Thu Aug 23 14:55:02 2001
%d			Day of the month (01-31)				23
%H			Hour in 24h format (00-23)				14
%I			Hour in 12h format (01-12)				02
%j			Day of the year (001-366)				235
%m			Month as a decimal number (01-12)		08
%M			Minute (00-59)							55
%p			AM or PM designation					PM
%S			Second (00-61)							02
%U			Week number (00-53)						33
%w			Weekday with Sunday as 0 (0-6)			4
%W			Week number (00-53)						34
%x			Date representation 					08/23/01
%X			Time representation 					14:55:02
%y			Year, last two digits (00-99)			01
%Y			Year									2001
%Z			Timezone name or abbreviation			CDT
%%			A % sign								%
*/
CLEVER_METHOD(Date::format)
{
	if (!clever_check_args("s")) {
		return;
	}

	clever_date_format(&args, clever_this(), result, false);
}

// String Date.uformat(string specstring)
// Formats this date as a string, using the same specifiers as Date.format
// Treats the date as UTC
CLEVER_METHOD(Date::uformat)
{
	if (!clever_check_args("s")) {
		return;
	}

	clever_date_format(&args, clever_this(), result, true);
}

// Int Date::getTime()
// Returns the time represented by this Date object as a Unix timestamp
CLEVER_METHOD(Date::getTime)
{
	DateObject* dobj = clever_get_this(DateObject*);

	if (!dobj->intern) {
		//clever_throw(eventually)
		return;
	}

	result->setInt(*dobj->intern);
}

// Date Date.new([int time])
// Constructs a new Date object, uses the current time if none is provided
CLEVER_METHOD(Date::ctor)
{
	if (!clever_check_args("|i")) {
		return;
	}

	DateObject* dobj = new DateObject();

	if (args.empty()) {
		time(dobj->intern);
	} else {
		*dobj->intern = static_cast<time_t>(args[0]->getInt());
	}

	result->setObj(this, dobj);
}

CLEVER_TYPE_INIT(Date::init)
{
	setConstructor((MethodPtr)&Date::ctor);

	addMethod(new Function("format",  (MethodPtr)&Date::format));
	addMethod(new Function("uformat", (MethodPtr)&Date::uformat));
	addMethod(new Function("getTime", (MethodPtr)&Date::getTime));
}

}}} // clever::modules::std
