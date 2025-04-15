#include <windows.h>
#include <CommCtrl.h>

#include "gui-helper.h"
#include "resource.h"

const wchar_t* szClass = L"winWindowClass";

#define IDC_BUTTON_OK 101
#define IDC_BUTTON_CANCEL 102

static HWND hWndList;
static HWND hWndButtonOk;
static HWND hWndButtonCancel;
static int selected_index = -1;
static HWND hWndTab;
static const char** tabLists[2];
static int tabCounts[2];


static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_CREATE:
		{
			HFONT hfFont = GetStockObject(DEFAULT_GUI_FONT);

			// Tab Control
			hWndTab = CreateWindowExW(
				0, WC_TABCONTROLW, NULL,
				WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
				0, 0, 100, 100,
				hWnd, (HMENU)2000, GetModuleHandle(NULL), NULL
			);
			SendMessage(hWndTab, WM_SETFONT, (WPARAM)hfFont, 0);

			TCITEM tie; // not TCITEMW cause it doesn't need to be
			tie.mask = TCIF_TEXT;

			tie.pszText = "Non-Steam";
			TabCtrl_InsertItem(hWndTab, 0, &tie);

			
			tie.pszText = "GoldSrc/Source Mods";
			TabCtrl_InsertItem(hWndTab, 1, &tie);

			//tie.pszText = "Steam Apps";
			//TabCtrl_InsertItem(hWndTab, 2, &tie);

			// ListBox
			hWndList = CreateWindowExA(
				WS_OVERLAPPED, // no style to look like it's inside tabs
				WC_LISTBOX, NULL,
				LBS_NOTIFY | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
				0, 0, 100, 100,
				hWnd, NULL, GetModuleHandle(NULL), NULL
			);
			SendMessage(hWndList, WM_SETFONT, (WPARAM)hfFont, 0);

			hWndButtonOk = CreateWindowExA(
				WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR,
				WC_BUTTON, "OK",
				BS_PUSHBUTTON | BS_TEXT | WS_CHILD | WS_MAXIMIZEBOX | WS_OVERLAPPED | WS_TABSTOP | WS_VISIBLE,
				0, 0, 30, 40,
				hWnd, (HMENU)IDC_BUTTON_OK,
				GetModuleHandle(NULL), NULL
			);
			SendMessage(hWndButtonOk, WM_SETFONT, (WPARAM)hfFont, 0);

			hWndButtonCancel = CreateWindowExA(
				WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR,
				WC_BUTTON, "Cancel",
				BS_PUSHBUTTON | BS_TEXT | WS_CHILD | WS_MAXIMIZEBOX | WS_OVERLAPPED | WS_TABSTOP | WS_VISIBLE,
				0, 0, 30, 40,
				hWnd, (HMENU)IDC_BUTTON_CANCEL,
				GetModuleHandle(NULL), NULL
			);
			SendMessage(hWndButtonCancel, WM_SETFONT, (WPARAM)hfFont, 0);

			break;
		}
		case WM_SIZE:
		{
			RECT rcClient;
			GetClientRect(hWnd, &rcClient);

			// Tab control
			SetWindowPos(hWndTab, NULL, 10, 10, rcClient.right - 20, rcClient.bottom - 70, SWP_NOZORDER);

			// tab display area
			RECT rcTab;
			GetClientRect(hWndTab, &rcTab);
			TabCtrl_AdjustRect(hWndTab, FALSE, &rcTab);

			// List inside tab
			SetWindowPos(hWndList, NULL, rcTab.left + 10, rcTab.top + 10, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, SWP_NOZORDER);

			// OK and Cancel buttons
			SetWindowPos(hWndButtonOk, NULL, rcClient.right - (80 * 2), rcClient.bottom - 45, 70, 30, SWP_NOZORDER);
			SetWindowPos(hWndButtonCancel, NULL, rcClient.right - 80, rcClient.bottom - 45, 70, 30, SWP_NOZORDER);
			break;
		}
		case WM_NOTIFY:
		{
			LPNMHDR pnmh = (LPNMHDR)lParam;
			if (pnmh->hwndFrom == hWndTab && pnmh->code == TCN_SELCHANGE)
			{
				int tabIndex = TabCtrl_GetCurSel(hWndTab);
				PopulateListBox(tabIndex);
			}
			break;
		}

		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
				case LBN_SELCHANGE:
					EnableWindow(hWndButtonOk, TRUE);
					break;
			}

			switch (LOWORD(wParam))
			{
				case IDC_BUTTON_OK:
				{
					LRESULT res = SendMessage(hWndList, LB_GETCURSEL, (WPARAM)NULL, (LPARAM)NULL);
					if (res != LB_ERR)
					{
						selected_index = res;
						int tabIndex = TabCtrl_GetCurSel(hWndTab);
						for (int i = 0; i < tabIndex; i++)
						{
							selected_index += tabCounts[i];
						}
						PostQuitMessage(WM_QUIT);
					}
					break;
				}

				case IDC_BUTTON_CANCEL:
					selected_index = -1;
					PostQuitMessage(WM_QUIT);
					break;
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(WM_QUIT);
			break;
		case WM_CLOSE:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProcW(hWnd, Msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

int ShowMessageBox(const char* title, const char* message)
{
	return MessageBox(NULL, message, title, MB_OK | MB_ICONEXCLAMATION);
}

int ShowMessageBoxW(const wchar_t* title, const wchar_t* message)
{
	return MessageBoxW(NULL, message, title, MB_OK);
}

int SelectionDialog(const char* title, int count, const char** list, int modsCount, const char** modsList, int selection)
{
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_TAB_CLASSES;
	InitCommonControlsEx(&icex);

	tabCounts[0] = count;
	tabCounts[1] = modsCount;

	tabLists[0] = list;
	tabLists[1] = modsList;

	MSG Msg;
	HWND hWnd;

	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASSEXW wc;
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hIcon = (HICON)LoadImageA(
		hInstance,
		MAKEINTRESOURCEA(IDI_ICON1), IMAGE_ICON,
		0, 0,
		LR_DEFAULTSIZE | LR_DEFAULTCOLOR | LR_SHARED
	);
	wc.hIconSm = (HICON)LoadImage(
		hInstance,
		MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR | LR_SHARED
	);
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = szClass;

	RegisterClassExW(&wc);

	wchar_t* unicodeTitle = ConvertStringToUnicode(title);

	hWnd = CreateWindowExW(
		WS_EX_CLIENTEDGE,
		szClass,
		unicodeTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		454, 388,
		NULL, NULL,
		hInstance, NULL
	);

	free(unicodeTitle);

	// Find current tab and selection, and populate the ListBox
	int tabIndex;
	int tabSize = sizeof(tabCounts) / sizeof(tabCounts[0]);
	for (tabIndex = 0; tabIndex < tabSize; tabIndex++)
	{
		if (tabCounts[tabIndex] < selection) {
			selection -= tabCounts[tabIndex];
		} else {
			break;
		}
	}
	PopulateListBoxWithSelection(tabIndex, selection);

	// Show window and get all events
	ShowWindow(hWnd, SW_SHOWNORMAL);

	while (GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	// Return the current selection
	return selected_index;
}

// Load values into current ListBox with a highlighted option
void PopulateListBoxWithSelection(int tabIndex, int selection)
{
	SendMessageW(hWndList, LB_RESETCONTENT, 0, 0);
	TabCtrl_SetCurSel(hWndTab, tabIndex);

	for (int i = 0; i < tabCounts[tabIndex]; ++i)
	{
		wchar_t* unicode = ConvertStringToUnicode(tabLists[tabIndex][i]);
		SendMessageW(hWndList, LB_ADDSTRING, 0, (LPARAM)unicode);
		free(unicode);
	}

	if (selection > -1) {
		SendMessageW(hWndList, LB_SETCURSEL, selection, 0);
	}
}

// Load valuies into current ListBox without a highlighted option
void PopulateListBox(int tabIndex)
{
	PopulateListBoxWithSelection(tabIndex, -1);
}


// Convert ANSI string to unicode (char to widechar)
wchar_t* ConvertStringToUnicode(const char* string)
{
	int unicodeStringLength = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
	wchar_t* unicodeString = malloc(unicodeStringLength * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, string, -1, unicodeString, unicodeStringLength);
	return unicodeString;
}