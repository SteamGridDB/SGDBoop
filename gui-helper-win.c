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

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_CREATE:
		{
			HFONT hfFont = GetStockObject(DEFAULT_GUI_FONT);
			hWndList = CreateWindowExA(
				WS_EX_CLIENTEDGE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR,
				WC_LISTBOX, NULL,
				LBS_NOTIFY | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL,
				0, 0, 100, 100,
				hWnd, NULL,
				GetModuleHandle(NULL), NULL
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
			HWND hEdit;
			RECT rcClient;

			GetClientRect(hWnd, &rcClient);
			SetWindowPos(hWndList, NULL, rcClient.left + 10, rcClient.top + 10, rcClient.right - 20, rcClient.bottom - 60, SWP_NOZORDER);
			SetWindowPos(hWndButtonOk, NULL, rcClient.right - (80 * 2), rcClient.bottom - 45, 70, 30, SWP_NOZORDER);
			SetWindowPos(hWndButtonCancel, NULL, rcClient.right - 80, rcClient.bottom - 45, 70, 30, SWP_NOZORDER);
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

int SelectionDialog(const wchar_t* title, int count, const char** list, int selection)
{
	InitCommonControls();

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

	const wchar_t* unicodeTitle = title;

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

	//free(unicodeTitle);

	for (int i = 0; i < count; ++i)
	{
		wchar_t * unicodeGameName = ConvertStringToUnicode(list[i]);
		SendMessageW(hWndList, LB_ADDSTRING, 0, (LPARAM)(LPCWSTR)unicodeGameName);
		free(unicodeGameName);
	}

	SendMessageW(hWndList, LB_SETCURSEL, selection, 0);

	ShowWindow(hWnd, SW_SHOWNORMAL);

	while (GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return selected_index;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	WNDCLASSEXW wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = szClass;

	if (!RegisterClassExW(&wc))
	{
		MessageBoxW(NULL, L"RegisterClassExW failed", L"Error", MB_OK);
		return 1;
	}

	HWND hWnd = CreateWindowExW(
		WS_EX_CLIENTEDGE,
		szClass,
		L"Test Title",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		500, 300,
		NULL, NULL,
		hInstance, NULL
	);

	if (!hWnd)
	{
		MessageBoxW(NULL, L"CreateWindowExW failed", L"Error", MB_OK);
		return 1;
	}

	// Set the title again for test.
	SetWindowTextW(hWnd, L"2223");

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

// Convert ANSI string to unicode (char to widechar)
wchar_t* ConvertStringToUnicode(const char* string)
{
	int unicodeStringLength = MultiByteToWideChar(CP_UTF8, 0, string, -1, NULL, 0);
	wchar_t* unicodeString = malloc(unicodeStringLength * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, string, -1, unicodeString, unicodeStringLength);
	return unicodeString;
}