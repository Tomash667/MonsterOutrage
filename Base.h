#pragma once

typedef const char* cstring;
typedef unsigned char byte;
typedef unsigned int uint;

struct INT2
{
	int x, y;

	INT2() {}
	INT2(int x, int y) : x(x), y(y) {}

	inline INT2 operator + (const INT2& pt) const
	{
		return INT2(x+pt.x, y+pt.y);
	}
};

cstring Format(cstring str, ...);

inline byte lerp(byte a, byte b, float t)
{
	int ia = (int)a,
		ib = (int)b;
	return byte(ia+(ib-ia)*t);
}

template<typename T>
inline void RemoveElement(vector<T>& v, const T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			std::iter_swap(it, end-1);
			v.pop_back();
			return;
		}
	}

	assert(0 && "Can't find element to remove!");
}

template<typename T>
inline void RemoveElement(vector<T>* v, const T& e)
{
	RemoveElement(*v, e);
}

template<typename T>
inline T& Add1(vector<T>& v)
{
	v.resize(v.size()+1);
	return v.back();
}

template<typename T>
inline T& Add1(vector<T>* v)
{
	v->resize(v->size()+1);
	return v->back();
}
