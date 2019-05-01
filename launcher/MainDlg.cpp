
// MainDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LauncherApp.h"
#include "MainDlg.h"
#include "OptionsDlg.h"
#include "afxdialogex.h"
#include "GameConfig.h"
#include "UpdateChecker.h"
#include "version.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// MainDlg dialog

#define WM_UPDATE_CHECK (WM_USER+10)

MainDlg::MainDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MAIN, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_ICON);
}

void MainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(MainDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_MESSAGE(WM_UPDATE_CHECK, OnUpdateCheck)
    ON_BN_CLICKED(IDC_OPTIONS_BTN, &MainDlg::OnBnClickedOptionsBtn)
    ON_BN_CLICKED(IDOK, &MainDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_EDITOR_BTN, &MainDlg::OnBnClickedEditorBtn)
    ON_BN_CLICKED(IDC_SUPPORT_BTN, &MainDlg::OnBnClickedSupportBtn)
    END_MESSAGE_MAP()


// MainDlg message handlers

BOOL MainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

    SetWindowTextA(PRODUCT_NAME_VERSION);

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    // Set header bitmap
    CStatic *picture = (CStatic *)GetDlgItem(IDC_HEADER_PIC);
    HBITMAP hbm = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_HEADER), IMAGE_BITMAP, 0, 0, 0);
    picture->SetBitmap(hbm);

    m_ToolTip.Create(this);
    m_ToolTip.Activate(TRUE);

#ifdef NDEBUG
    m_pUpdateChecker = new UpdateChecker(m_hWnd);
    m_pUpdateChecker->check_async([=]() {
        PostMessageA(WM_UPDATE_CHECK, 0, 0);
    });
#else
    m_pUpdateChecker = nullptr;
#endif

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL MainDlg::PreTranslateMessage(MSG* pMsg)
{
    if (m_ToolTip)
        m_ToolTip.RelayEvent(pMsg);

    return CDialogEx::PreTranslateMessage(pMsg);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void MainDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR MainDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT MainDlg::OnUpdateCheck(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    CStatic *updateStatus = (CStatic*)GetDlgItem(IDC_UPDATE_STATUS);

    if (m_pUpdateChecker->has_error())
    {
        updateStatus->SetWindowTextA("Failed to check for update");
        m_ToolTip.AddTool(updateStatus, m_pUpdateChecker->get_error().c_str());
        return 0;
    }

    if (!m_pUpdateChecker->is_new_version_available())
        updateStatus->SetWindowTextA("No update is available.");
    else
    {
        updateStatus->SetWindowTextA("New version available!");
        int iResult = MessageBoxA(m_pUpdateChecker->get_message().c_str(),
            "Dash Faction update is available!", MB_OKCANCEL | MB_ICONEXCLAMATION);
        if (iResult == IDOK)
        {
            ShellExecuteA(m_hWnd, "open", m_pUpdateChecker->get_url().c_str(), NULL, NULL, SW_SHOW);
            EndDialog(0);
        }
    }

    return 0;
}

void MainDlg::OnBnClickedOptionsBtn()
{
    try
    {
        OptionsDlg dlg;
        INT_PTR nResponse = dlg.DoModal();
    }
    catch (std::exception &e)
    {
        MessageBoxA(e.what(), NULL, MB_ICONERROR | MB_OK);
    }
}

void MainDlg::OnBnClickedOk()
{
    if (m_pUpdateChecker)
        m_pUpdateChecker->abort();

    if (static_cast<LauncherApp*>(AfxGetApp())->LaunchGame(m_hWnd))
        CDialogEx::OnOK();
}

void MainDlg::OnBnClickedEditorBtn()
{
    if (m_pUpdateChecker)
        m_pUpdateChecker->abort();

    if (static_cast<LauncherApp*>(AfxGetApp())->LaunchEditor(m_hWnd))
        EndDialog(0);
}

void MainDlg::OnBnClickedSupportBtn()
{
    ShellExecuteA(m_hWnd, "open", "https://discord.gg/bC2WzvJ", NULL, NULL, SW_SHOW);
}
