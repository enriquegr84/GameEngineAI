// Geometric Tools, LLC
// Copyright (c) 1998-2014
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
//
// File Version: 5.0.0 (2010/01/01)

#include "Core/Core.h"
#include "Environment.h"

//----------------------------------------------------------------------------
const eastl::wstring Environment::GetWorkingDirectory()
{
	eastl::wstring wdir;
	#if defined(_WINDOWS_API_)
		wchar_t tmp[_MAX_PATH];
		_wgetcwd(tmp, _MAX_PATH);
		wdir = tmp;
		eastl::replace(wdir.begin(), wdir.end(), L'\\', L'/');

		wdir.validate();
	#endif

	return wdir;
}

//----------------------------------------------------------------------------
eastl::string Environment::GetAbsolutePath(const eastl::string& filename)
{
#if defined(_WINDOWS_API_)
	char tmp[_MAX_PATH];
	GetModuleFileNameA(NULL, tmp, _MAX_PATH);
	eastl::string wdir = tmp;
	eastl::replace(wdir.begin(), wdir.end(), '\\', '/');
	wdir = wdir.substr(0, wdir.find_last_of(L'/'));
	wdir += filename;

	wdir.validate();

	return wdir;
#else
	return path(filename);
#endif
}

