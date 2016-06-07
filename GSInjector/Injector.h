#pragma once
#include <vector>
#include <type_traits>

#define INJECT_STDCALL __stdcall
#define INJECT_CDECL __cdecl

// Helper class to calculate argument size.
template <typename Arg>
struct SingleArgSize
{
	enum {
		size = std::conditional< std::is_reference<Arg>::value,
		                         std::integral_constant<int, 4>,
		                         std::integral_constant<int, sizeof(Arg)> >::type::value
	};
};
template <typename... Args>
struct CountArgSize;

template <>
struct CountArgSize<>
{
	enum { size = 0 };
};

template <typename Arg1, typename... Args>
struct CountArgSize<Arg1, Args...>
{
	enum { size = SingleArgSize<Arg1>::size + CountArgSize<Args...>::size };
};

// A single function injected.
/*
* Stdcall: Arguments passed by value, cleaned by the callee.
* CDecl:   Arguments passed by value, not cleaned.
*   InjectedFunction use this to modify arguments (and return value).
*/
class InjectedFunction
{
public:
	SIZE_T szArgSize = 0;
	LPVOID lpStdcallFunction = nullptr;
	LPVOID lpCdeclFunction = nullptr;

	template <typename Ret, typename... Args>
	InjectedFunction(Ret(INJECT_STDCALL *f)(Args...))
	{
		szArgSize = CountArgSize<Args...>::size;
		lpStdcallFunction = f;
	}

	template <typename Ret, typename... Args>
	InjectedFunction(Ret(INJECT_CDECL *f)(Args...))
	{
		szArgSize = CountArgSize<Args...>::size;
		lpCdeclFunction = f;
	}

	InjectedFunction()
	{
	}

	DWORD Call(LPVOID lpStackTop, LPDWORD lpReturnValue, DWORD dwMaxArgSize);
};

// Storage of an injector. Allocated automatically.
class InjectorInformation
{
public:
	InjectedFunction replaced;
	std::vector<InjectedFunction> injectedBefore;
	std::vector<InjectedFunction> injectedAfter;
	LPVOID original;
	bool injected;
	DWORD dwMaxArgSize;
};

// Interface to maniplulate injection. This is the representation of a injection point.
class Injector
{
	InjectorInformation* info;

public:
	Injector(InjectorInformation* info) : info(info)
	{
	}

	void Replace(InjectedFunction fDest)
	{
		info->replaced = fDest;
	}
	void Before(InjectedFunction fDest)
	{
		info->injectedBefore.push_back(fDest);
	}
	void After(InjectedFunction fDest)
	{
		info->injectedAfter.push_back(fDest);
	}

	void Run(LPVOID lpStackTop);
};
