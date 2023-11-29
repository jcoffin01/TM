
// TMView.h : interface of the CTMView class
//

#pragma once


class CTMView : public CFormView
{
protected: // create from serialization only
	CTMView() noexcept;
	DECLARE_DYNCREATE(CTMView)

public:
#ifdef AFX_DESIGN_TIME
	enum{ IDD = IDD_TM_FORM };
#endif

// Attributes
public:
	CTMDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CTMView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void fill();
	void update();

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl processes;
	PVOID buffer;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnLvnItemActivateList1(NMHDR* pNMHDR, LRESULT* pResult);
	CButton Killer;
};

#ifndef _DEBUG  // debug version in TMView.cpp
inline CTMDoc* CTMView::GetDocument() const
   { return reinterpret_cast<CTMDoc*>(m_pDocument); }
#endif

