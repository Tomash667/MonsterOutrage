#include "Pch.h"
#include "Base.h"

cstring Format(cstring str, ...)
{
	const uint FORMAT_STRINGS = 8;
	const uint FORMAT_LENGTH = 2048;

	assert(str);

	static char buf[FORMAT_STRINGS][FORMAT_LENGTH];
	static int marker = 0;

	va_list list;
	va_start(list,str);
	_vsnprintf_s((char*)buf[marker],FORMAT_LENGTH,FORMAT_LENGTH-1,str,list);
	char* cbuf = buf[marker];
	cbuf[FORMAT_LENGTH-1] = 0;

	marker = (marker+1)%FORMAT_STRINGS;

	return cbuf;
}
