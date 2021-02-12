// ICR8600-UDP.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ICR8600-UDP.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND guiButton;
HWND startButton;
HWND ipAddressLabel;
HWND udpPortLabel;
HWND ipAddressText;
HWND udpPortText;
HWND freqText;
HWND freqLabel;
HWND seqNumEnableCheck;
HWND seqNumEnableLabel;
HWND sendFloatCheck;
HWND sendFloatLabel;


BOOL guiVisible = FALSE;
BOOL radioStarted = FALSE;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                guiButtonClicked(void);
void                startButtonClicked(void);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    if (!radioOpen())
    {
        MessageBox(NULL, L"Failed to open radio\nCheck for ExtIO DLL", NULL, MB_OK);
        return FALSE;
    }

    // if open failes... MessageBox(NULL, L"hello, world", L"caption", 0);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ICR8600UDP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ICR8600UDP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICR8600UDP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_ICR8600UDP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 300, 300, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   guiButton = CreateWindow(
       L"BUTTON",
       L"Show GUI",
       WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
       10, 175,
       100, 35,
       hWnd,
       (HMENU)IDC_SHOWGUI,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   startButton = CreateWindow(
       L"BUTTON",
       L"Start",
       WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
       170, 175,
       100, 35,
       hWnd,
       (HMENU)IDC_START,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   freqLabel = CreateWindow(
       L"STATIC",
       L"Frequency (Hz)",
       WS_CHILD | WS_VISIBLE | SS_SIMPLE,
       10, 35,
       100, 35,
       hWnd,
       (HMENU)IDC_IPADDRESS_LABEL,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   freqText = CreateWindow(
       L"EDIT",
       NULL,
       WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_NUMBER,
       120, 30,
       150, 25,
       hWnd,
       (HMENU)IDC_IPADDRESS,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );



   ipAddressLabel = CreateWindow(
       L"STATIC",
       L"IP Address",
       WS_CHILD | WS_VISIBLE | SS_SIMPLE,
       10, 65,
       100, 35,
       hWnd,
       (HMENU)IDC_IPADDRESS_LABEL,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   ipAddressText = CreateWindow(
       L"EDIT",
       NULL,
       WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT,
       120, 60,
       150, 25,
       hWnd,
       (HMENU)IDC_IPADDRESS,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   udpPortLabel = CreateWindow(
       L"STATIC",
       L"UDP Port",
       WS_CHILD | WS_VISIBLE | SS_SIMPLE,
       10, 95,
       100, 35,
       hWnd,
       (HMENU)IDC_IPADDRESS_LABEL,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   udpPortText = CreateWindow(
       L"EDIT",
       NULL,
       WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT | ES_NUMBER,
       120, 90,
       50, 25,
       hWnd,
       (HMENU)IDC_UDPPORT,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   seqNumEnableLabel = CreateWindow(
       L"STATIC",
       L"Send Sequence",
       WS_CHILD | WS_VISIBLE | SS_SIMPLE,
       10, 125,
       100, 35,
       hWnd,
       (HMENU)IDC_SEQNUM_LABEL,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   seqNumEnableCheck = CreateWindow(
       L"BUTTON",
       NULL,
       WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
       120, 120,
       35, 25,
       hWnd,
       (HMENU)IDC_SEQNUM,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   sendFloatLabel = CreateWindow(
       L"STATIC",
       L"Float",
       WS_CHILD | WS_VISIBLE | SS_SIMPLE,
       10, 155,
       100, 35,
       hWnd,
       (HMENU)IDC_FLOAT_LABEL,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   sendFloatCheck = CreateWindow(
       L"BUTTON",
       NULL,
       WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
       120, 150,
       35, 25,
       hWnd,
       (HMENU)IDC_FLOAT,
       (HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE),
       NULL
   );

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            int noteCode = HIWORD(wParam);

            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDC_SHOWGUI:
                if (noteCode == BN_CLICKED)
                {
                    guiButtonClicked();
                }
                break;
            case IDC_START:
                if (noteCode == BN_CLICKED)
                {
                    startButtonClicked();
                }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        radioClose();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void guiButtonClicked()
{
    if (!guiVisible) 
    {
        SendMessage(guiButton, WM_SETTEXT, NULL, (LPARAM)L"Hide GUI");
        radioShowGui();
        guiVisible = TRUE;
    }
    else
    {
        SendMessage(guiButton, WM_SETTEXT, NULL, (LPARAM)L"Show GUI");
        radioHideGui();
        guiVisible = FALSE;
    }
    SetFocus(NULL);
}

void startButtonClicked()
{
    static char ip[256];
    static char udp[64];
    static char f[256];
    static USHORT udpPort;
    static int64_t freq;
    static BOOL sendSequenceNumber;
    static BOOL sendAsFloat;

    if (!radioStarted)
    {
        SendMessage(startButton, WM_SETTEXT, NULL, (LPARAM)L"Stop");

        GetWindowTextA(ipAddressText, ip, sizeof(ip));
        GetWindowTextA(udpPortText, udp, sizeof(udp));
        GetWindowTextA(freqText, f, sizeof(f));

        udp[sizeof(udp) - 1] = 0;
        ip[sizeof(ip) - 1] = 0;
        f[sizeof(f) - 1] = 0;

        udpPort = (USHORT)strtoul(udp, NULL, 10);
        freq = _strtoui64(f, NULL, 10);

        sendSequenceNumber = Button_GetCheck(seqNumEnableCheck);
        sendAsFloat = Button_GetCheck(sendFloatCheck);

        if (radioStartStream(ip, udpPort, freq, sendSequenceNumber, sendAsFloat)) {
            radioStarted = TRUE;
            if (guiVisible) {
                guiButtonClicked();
            }
            EnableWindow(guiButton, FALSE);
            EnableWindow(freqText, FALSE);
            EnableWindow(ipAddressText, FALSE);
            EnableWindow(udpPortText, FALSE);
            EnableWindow(seqNumEnableCheck, FALSE);
            EnableWindow(sendFloatCheck, FALSE);
        }
        else {
            MessageBox(NULL, L"Failed to start radio", NULL, MB_OK);
            radioStarted = FALSE;
        }
    }
    else
    {
        radioStopStream();
        SendMessage(startButton, WM_SETTEXT, NULL, (LPARAM)L"Start");
        EnableWindow(guiButton, TRUE);
        EnableWindow(freqText, TRUE);
        EnableWindow(ipAddressText, TRUE);
        EnableWindow(udpPortText, TRUE);
        EnableWindow(seqNumEnableCheck, TRUE);
        EnableWindow(sendFloatCheck, TRUE);
        radioStarted = FALSE;
    }
    SetFocus(NULL);
}
