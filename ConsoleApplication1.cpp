// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <UIAutomationClient.h>
#include <iostream>
#include <windows.h>

IUIAutomation* g_pAutomation = NULL;
std::wstring g_chromeUrl = L"";
HWND g_hWnd = NULL;
BOOL g_bRunning = TRUE;

IUIAutomationElement* GetTopLevelWindowByName(LPWSTR windowName)
{
	if (!windowName)
	{
		return NULL;
	}

	IUIAutomationElement* pRoot = NULL;
	IUIAutomationElement* pFound = NULL;
	VARIANT varProp;
	varProp.vt = VT_BSTR;
	varProp.bstrVal = SysAllocString(windowName);

	// Get the desktop element
	HRESULT hr = g_pAutomation->GetRootElement(&pRoot);

	// Get a top-level element by name, such as "Program Manager"
	if (pRoot)
	{
		IUIAutomationCondition* pCondition;
		g_pAutomation->CreatePropertyCondition(UIA_ClassNamePropertyId,
			varProp, &pCondition);
		pRoot->FindFirst(TreeScope_Children, pCondition, &pFound);
		pRoot->Release();
		pCondition->Release();
	}

	VariantClear(&varProp);
	return pFound;
}

void ListDescendants(IUIAutomationElement* pParent)
{
	if (pParent == NULL)
		return;

	IUIAutomationTreeWalker* pControlWalker = NULL;
	IUIAutomationElement* pNode = NULL;

	g_pAutomation->get_ControlViewWalker(&pControlWalker);
	if (pControlWalker == NULL)
		goto cleanup;

	pControlWalker->GetFirstChildElement(pParent, &pNode);
	if (pNode == NULL)
		goto cleanup;

	while (pNode)
	{
		BSTR name;
		pNode->get_CurrentName(&name);
		if (name && 0 == wcscmp(name, L"Address and search bar"))
		{
			IUIAutomationValuePattern* pValuePattern = NULL;
			HRESULT hr = pNode->GetCurrentPatternAs(UIA_ValuePatternId, IID_IUIAutomationValuePattern, (void**)&pValuePattern);
			if (SUCCEEDED(hr) && pValuePattern != NULL)
			{
				BSTR tmpBstr;
				HRESULT hr = pValuePattern->get_CurrentValue(&tmpBstr);
				if (SUCCEEDED(hr) && tmpBstr != NULL) {
					std::wstring ws(tmpBstr, SysStringLen(tmpBstr));
					if (ws != g_chromeUrl)
					{
						g_chromeUrl = ws;

						if (g_hWnd)
						{
							RECT rc;
							HDC hdc = GetDC(g_hWnd);
							
							GetClientRect(g_hWnd, &rc);
							InvalidateRect(g_hWnd, &rc, TRUE); // clean the previous text
							SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

							SetTextColor(hdc, RGB(0, 0, 255));

							DrawText(hdc, g_chromeUrl.c_str(), g_chromeUrl.length(), &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

							ReleaseDC(g_hWnd, hdc);
						}
					}
				}
			}
		}
		
		SysFreeString(name);

		ListDescendants(pNode);
		IUIAutomationElement* pNext;
		pControlWalker->GetNextSiblingElement(pNode, &pNext);
		pNode->Release();
		pNode = pNext;
	}

cleanup:
	if (pControlWalker != NULL)
		pControlWalker->Release();

	if (pNode != NULL)
		pNode->Release();

	return;
}


long __stdcall WindowProcedure(HWND window, unsigned int msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
		std::cout << "\ndestroying window\n";
		PostQuitMessage(0);
		g_bRunning = FALSE;
		return 0L;
	default:
		return DefWindowProc(window, msg, wp, lp);
	}
}

void createWindow()
{
	const wchar_t* const myclass = L"myclass";
	WNDCLASSEX wndclass = { sizeof(WNDCLASSEX), CS_DBLCLKS, WindowProcedure,
		0, 0, GetModuleHandle(0), LoadIcon(0, IDI_APPLICATION),
		LoadCursor(0, IDC_ARROW), HBRUSH(COLOR_WINDOW + 1),
		0, myclass, LoadIcon(0, IDI_APPLICATION) };
	if (RegisterClassEx(&wndclass))
	{
		g_hWnd = CreateWindowEx(0, myclass, L"Chrome Url Viewer",
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(0), 0);
		if (g_hWnd)
		{
			ShowWindow(g_hWnd, SW_SHOWDEFAULT);
			MSG msg;
			while (GetMessage(&msg, 0, 0, 0))
			{
				DispatchMessage(&msg);
			}
		}
	}


}


DWORD threadEntryPoint(LPVOID pParam)
{
	createWindow();
	return 0;
}

int _tmain(int argc, _TCHAR* argv[])
{
	FreeConsole();
	// Initialize COM and create the main Automation object
	CoInitialize(NULL);
	HRESULT hr = CoCreateInstance(__uuidof(CUIAutomation), NULL,
		CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation),
		(void**)&g_pAutomation);
	if (FAILED(hr))
		return (hr);

	IUIAutomationElement* element = GetTopLevelWindowByName(L"Chrome_WidgetWin_1");

	::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&threadEntryPoint, NULL, THREAD_PRIORITY_NORMAL, NULL);
	
	if (element)
	{
		while (g_bRunning)
		{
			ListDescendants(element);
		}
	}
	
	// Clean up our COM pointers
	g_pAutomation->Release();
	CoUninitialize();
	return 0;
}