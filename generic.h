#ifndef GENERIC_H
#define GENERIC_H
#pragma once

#define SAFE_ENUM(Name, ...) \
struct _##Name \
{ \
	enum E \
	{ \
		__VA_ARGS__, \
		_Count \
	}; \
}; \
\
typedef _##Name::E Name;

#endif // !GENERIC_H
