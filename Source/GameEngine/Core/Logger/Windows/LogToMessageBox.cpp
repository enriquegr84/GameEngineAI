// David Eberly, Geometric Tools, Redmond WA 98052
// Copyright (c) 1998-2017
// Distributed under the Boost Software License, Version 1.0.
// http://www.boost.org/LICENSE_1_0.txt
// http://www.geometrictools.com/License/Boost/LICENSE_1_0.txt
// File Version: 3.0.0 (2016/06/19)

#include "LogToMessageBox.h"

LogToMessageBox::LogToMessageBox(int flags)
    :
    Logger::Listener(flags)
{
}

void LogToMessageBox::Report(eastl::string const& message)
{
	eastl::string output = message + "Do you want to debug?";
	eastl::wstring outputConvertion(message.c_str());

    int selection = MessageBox(nullptr, outputConvertion.c_str(), L"Report",
        MB_ICONERROR | MB_YESNOCANCEL | MB_APPLMODAL | MB_TOPMOST);

    switch (selection)
    {
    case IDYES:
        // Break and debug.
        __debugbreak();
        break;

    case IDNO:
        // Continue execution.
        break;

    case IDCANCEL:
    default:
        // Terminate execution.
        exit(0);
        break;
    }
}
