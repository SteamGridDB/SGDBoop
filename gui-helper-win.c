#include <windows.h>
#include <CommCtrl.h>

#include "gui-helper.h"
#include "resource.h"

const char szClass[] = "winWindowClass";

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
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

int ShowMessageBox(const char* title, const char* message)
{
	return MessageBox(NULL, title, message, MB_OK | MB_ICONEXCLAMATION);
}

int SelectionDialog(const char* title, int count, const char** list, int selection)
{
	InitCommonControls();

	MSG Msg;
	HWND hWnd;

	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASSEXA wc;
	wc.cbSize = sizeof(WNDCLASSEXA);
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

	RegisterClassExA(&wc);

	hWnd = CreateWindowExA(
		WS_EX_CLIENTEDGE,
		szClass,
		title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		454, 388,
		NULL, NULL,
		hInstance, NULL
	);

	for (int i = 0; i < count; ++i)
	{
		SendMessage(hWndList, LB_ADDSTRING, (WPARAM)NULL, (LPARAM)list[i]);
	}

	ShowWindow(hWnd, SW_SHOWNORMAL);

	while (GetMessage(&Msg, NULL, 0, 0))
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	return selected_index;
}