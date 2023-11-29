
// TM.h : main header file for the TM application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CTMApp:
// See TM.cpp for the implementation of this class
//

class CTMApp : public CWinApp
{
public:
	CTMApp() noexcept;


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CTMApp theApp;
