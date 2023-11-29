
// TMView.cpp : implementation of the CTMView class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "TM.h"
#endif

#include <sstream>
#include <winternl.h>
#pragma comment(lib,"ntdll.lib")

#include <vector>
#include <set>

#include "TMDoc.h"
#include "TMView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTMView

IMPLEMENT_DYNCREATE(CTMView, CFormView)

BEGIN_MESSAGE_MAP(CTMView, CFormView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CFormView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CFormView::OnFilePrintPreview)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON1, &CTMView::OnBnClickedButton1)
	ON_NOTIFY(LVN_ITEMACTIVATE, IDC_LIST1, &CTMView::OnLvnItemActivateList1)
END_MESSAGE_MAP()

// CTMView construction/destruction

CTMView::CTMView() noexcept
	: CFormView(IDD_TM_FORM)
	, buffer(VirtualAlloc(NULL, 1024 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE))
{	
}

CTMView::~CTMView()
{
	VirtualFree(buffer, 0, MEM_RELEASE);
}

void CTMView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, processes);
	DDX_Control(pDX, IDC_BUTTON1, Killer);
}

BOOL CTMView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

std::wstring cvtMem(DWORD_PTR memUsed) {
	std::wstringstream memory;
	memory.imbue(std::locale(""));
	std::string mult = "BKMGT";
	std::size_t i = 0;

	while (memUsed > 1024 && i < mult.size()) {
		memUsed /= 1024;
		++i;
	}
	memory << memUsed << " " << mult[i];
	return memory.str();
}

class LargeInteger {
	ULARGE_INTEGER data;
public:
	LargeInteger(FILETIME const& ft) {
		data.LowPart = ft.dwLowDateTime;
		data.HighPart = ft.dwHighDateTime;
	}

	LargeInteger(uint64_t const& d) {
		data.QuadPart = d;
	}

	operator uint64_t() const { return data.QuadPart; }

	operator FILETIME() const {
		FILETIME ret;
		ret.dwLowDateTime = data.LowPart;
		ret.dwHighDateTime = data.HighPart;
		return ret;
	}

	operator SYSTEMTIME() const {
		SYSTEMTIME ret;
		FILETIME ft;
		ft.dwLowDateTime = data.LowPart;
		ft.dwHighDateTime = data.HighPart;

		FileTimeToSystemTime(&ft, &ret);
		return ret;
	}

	LargeInteger operator+(LargeInteger const& b) const {
		return LargeInteger(data.QuadPart+b.data.QuadPart);
	}
};

CString GetCpuTime(HANDLE pid) {
	auto process = OpenProcess(PROCESS_QUERY_INFORMATION, false, (DWORD)pid);
	if (process == NULL)
		return CString();

	FILETIME createTime, exitTime, kernelTime, userTime;

	GetProcessTimes(process, &createTime, &exitTime, &kernelTime, &userTime);
	CloseHandle(process);

	SYSTEMTIME t = LargeInteger(kernelTime) + LargeInteger(userTime);
	CString result;
	result.Format(L"%d:%d:%d.%3.3d", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
	return result;
}

void insert(CListCtrl& processes, PSYSTEM_PROCESS_INFORMATION spi, int i=0) {
	CString imageName(spi->ImageName.Buffer, spi->ImageName.Length);
	processes.InsertItem(i, spi->ImageName.Buffer);

	processes.SetItemText(i, 1, cvtMem(spi->PrivatePageCount).c_str());

	std::wstringstream threadCount;
	threadCount << spi->NumberOfThreads;

	processes.SetItemText(i, 2, threadCount.str().c_str());
	processes.SetItemData(i, (DWORD_PTR)spi->UniqueProcessId);
	CString cpuTime = GetCpuTime(spi->UniqueProcessId);
	processes.SetItemText(i, 3, cpuTime);
}

void CTMView::fill() {
	auto spi = (PSYSTEM_PROCESS_INFORMATION)buffer;

	NTSTATUS status;

	if (NT_SUCCESS(status = NtQuerySystemInformation(SystemProcessInformation, spi, 1024 * 1024, NULL))) {
		int i = 0;
		while (spi->NextEntryOffset) {
			insert(processes, spi, i);
			spi = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)spi + spi->NextEntryOffset);
		}
	}
}

void CTMView::update() {
	auto spi = (PSYSTEM_PROCESS_INFORMATION)buffer;

	NTSTATUS status;
	std::set<DWORD_PTR> pids;

	// Update data for existing items, add new items, delete items that no longer exist
	if (NT_SUCCESS(status = NtQuerySystemInformation(SystemProcessInformation, spi, 1024 * 1024, NULL))) {
		int i = 0;
		for (;;) {

			LVFINDINFOW crit;
			crit.flags = LVFI_PARAM;

			auto pid = *reinterpret_cast<DWORD_PTR*>(&spi->UniqueProcessId);
			pids.insert(pid);
			crit.lParam = pid;
			auto index = processes.FindItem(&crit, -1);

			if (index == -1) {
				insert(processes, spi);
			} else {
				processes.SetItemText(i, 1, cvtMem(spi->PrivatePageCount).c_str());

				auto threadCount = std::to_wstring(spi->NumberOfThreads);
				processes.SetItemText(i, 2, threadCount.c_str());
				
				processes.SetItemText(i, 3, GetCpuTime(spi->UniqueProcessId));
			}
			if (spi->NextEntryOffset == 0)
				break;
			spi = (PSYSTEM_PROCESS_INFORMATION)((LPBYTE)spi + spi->NextEntryOffset);
		}
	}
	for (int i = processes.GetItemCount()-1; i > -1; i--) {
		auto pid = processes.GetItemData(i);
		if (pids.find(pid) == pids.end()) {
			processes.DeleteItem(i);
		}
	}
}

void CTMView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();

	processes.InsertColumn(0, L"Image Name", LVCFMT_LEFT, 300, 0);
	processes.InsertColumn(1, L"Memory", LVCFMT_LEFT, 100, 1);
	processes.InsertColumn(2, L"Threads", LVCFMT_LEFT, 80, 2);
	processes.InsertColumn(3, L"CPU Time", LVCFMT_LEFT, 100, 3);
	processes.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	fill();
	SetTimer(100, 3000, NULL);
}


// CTMView printing

BOOL CTMView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTMView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTMView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CTMView::OnPrint(CDC* pDC, CPrintInfo* /*pInfo*/)
{
	// TODO: add customized printing code here
}


// CTMView diagnostics

#ifdef _DEBUG
void CTMView::AssertValid() const
{
	CFormView::AssertValid();
}

void CTMView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CTMDoc* CTMView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTMDoc)));
	return (CTMDoc*)m_pDocument;
}
#endif //_DEBUG


// CTMView message handlers


void CTMView::OnTimer(UINT_PTR nIDEvent)
{
	update();

	CFormView::OnTimer(nIDEvent);
}


void CTMView::OnBnClickedButton1()
{
	auto pid = processes.GetItemData(processes.GetSelectionMark());
	auto handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	TerminateProcess(handle, 1);
	Killer.EnableWindow(false);
}


void CTMView::OnLvnItemActivateList1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
	Killer.EnableWindow();
}
