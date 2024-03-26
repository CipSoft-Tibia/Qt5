// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "qaxwidget.h"
#include "qaxwidget_p.h"
#include "qaxbase_p.h"
#include <QtAxBase/private/qaxutils_p.h>
#include <QtAxBase/private/qaxtypefunctions_p.h>

#include <QtAxServer/qaxaggregated.h>

#include <qabstracteventdispatcher.h>
#include <qapplication.h>
#include <private/qapplication_p.h>
#include <qdockwidget.h>
#include <qevent.h>
#include <qlayout.h>
#include <qmainwindow.h>
#include <qmenu.h>
#include <qmenubar.h>
#include <qmetaobject.h>
#include <qpainter.h>
#include <qpointer.h>
#include <qregularexpression.h>
#include <quuid.h>
#include <qwhatsthis.h>
#include <qabstractnativeeventfilter.h>
#include <private/qpixmap_win_p.h>

#include <windowsx.h>
#include <ocidl.h>
#include <olectl.h>
#include <docobj.h>

// #define QAX_DEBUG

#ifdef QAX_DEBUG
#define AX_DEBUG(x) qDebug(#x);
#else
#define AX_DEBUG(x);
#endif

// #define QAX_SUPPORT_WINDOWLESS
// #define QAX_SUPPORT_BORDERSPACE

// missing interface from win32api
#if defined(Q_CC_GNU) && !defined(__MINGW64_VERSION_MAJOR)
    DECLARE_INTERFACE_(IOleInPlaceObjectWindowless,IOleInPlaceObject)
    {
       STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
       STDMETHOD_(ULONG,AddRef)(THIS) PURE;
       STDMETHOD_(ULONG,Release)(THIS) PURE;
       STDMETHOD(GetWindow)(THIS_ HWND*) PURE;
       STDMETHOD(ContextSensitiveHelp)(THIS_ BOOL) PURE;
       STDMETHOD(InPlaceDeactivate)(THIS) PURE;
       STDMETHOD(UIDeactivate)(THIS) PURE;
       STDMETHOD(SetObjectRects)(THIS_ LPCRECT,LPCRECT) PURE;
       STDMETHOD(ReactivateAndUndo)(THIS) PURE;
       STDMETHOD(OnWindowMessage)(THIS_ UINT, WPARAM, LPARAM, LRESULT*) PURE;
       STDMETHOD(GetDropTarget)(THIS_ IDropTarget**) PURE;
    };
#endif

#include "../shared/qaxtypes_p.h"

QT_BEGIN_NAMESPACE

void QAxWidgetPrivate::emitException(int code, const QString &source, const QString &desc,
                                     const QString &help)
{
    Q_Q(QAxWidget);
    emit q->exception(code, source, desc, help);
}

void  QAxWidgetPrivate::emitPropertyChanged(const QString &name)
{
    Q_Q(QAxWidget);
    emit q->propertyChanged(name);
}

void  QAxWidgetPrivate::emitSignal(const QString &name, int argc, void *argv)
{
    Q_Q(QAxWidget);
    emit q->signal(name, argc, argv);
}

/*  \class QAxHostWidget
    \brief The QAxHostWidget class is the actual container widget.

    \internal
*/
class QAxHostWidget : public QWidget
{
    Q_DISABLE_COPY_MOVE(QAxHostWidget)
    friend class QAxClientSite;
public:
    QAxHostWidget(QWidget *parent, QAxClientSite *ax);
    ~QAxHostWidget() override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    int qt_metacall(QMetaObject::Call, int isignal, void **argv) override;
    void *qt_metacast(const char *clname) override;

    inline QAxClientSite *clientSite() const
    {
        return axhost;
    }

    QWindow *hostWindow() const;

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif
    bool event(QEvent *e) override;
    bool eventFilter(QObject *o, QEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void showEvent(QShowEvent *e) override;
    QPaintEngine *paintEngine() const override
    {
        return nullptr;
    }

private:
    void resizeObject();

    int setFocusTimer;
    bool hasFocus;
    QAxClientSite *axhost;
};

/*  \class QAxClientSite
    \brief The QAxClientSite class implements the client site interfaces.

    \internal
*/
class QAxClientSite : public IDispatch,
                    public IOleClientSite,
                    public IOleControlSite,
#ifdef QAX_SUPPORT_WINDOWLESS
                    public IOleInPlaceSiteWindowless,
#else
                    public IOleInPlaceSite,
#endif
                    public IOleInPlaceFrame,
                    public IOleDocumentSite,
                    public IAdviseSink
{
    Q_DISABLE_COPY_MOVE(QAxClientSite)
    friend class QAxHostWidget;
public:
    QAxClientSite(QAxWidget *c);
    virtual ~QAxClientSite();

    bool activateObject(bool initialized, const QByteArray &data);

    void releaseAll();
    void deactivate();
    inline void reset(QWidget *p)
    {
        if (widget == p)
            widget = nullptr;
        else if (host == p)
            host = nullptr;
    }

    inline IOleInPlaceActiveObject *inPlaceObject() const
    {
        return m_spInPlaceActiveObject;
    }

    inline HRESULT doVerb(LONG index)
    {
        if (!m_spOleObject)
            return E_NOTIMPL;
        if (!host)
            return OLE_E_NOT_INPLACEACTIVE;

        RECT rcPos = qaxNativeWidgetRect(host);
        return m_spOleObject->DoVerb(index, nullptr, this, 0,
                                     reinterpret_cast<HWND>(host->winId()),
                                     &rcPos);
    }

    // IUnknown
    unsigned long WINAPI AddRef() override;
    unsigned long WINAPI Release() override;
    STDMETHOD(QueryInterface)(REFIID iid, void **iface) override;

    // IDispatch
    HRESULT __stdcall GetTypeInfoCount(unsigned int *) override
    { return E_NOTIMPL; }
    HRESULT __stdcall GetTypeInfo(UINT, LCID, ITypeInfo **) override
    { return E_NOTIMPL; }
    HRESULT __stdcall GetIDsOfNames(const _GUID &, wchar_t **, unsigned int,
                                    unsigned long, long *) override
    { return E_NOTIMPL; }
    HRESULT __stdcall Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
                             WORD wFlags, DISPPARAMS *pDispParams,
                             VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                             UINT *puArgErr) override;
    void emitAmbientPropertyChange(DISPID dispid);

    // IOleClientSite
    STDMETHOD(SaveObject)() override;
    STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk) override;
    STDMETHOD(GetContainer)(LPOLECONTAINER FAR* ppContainer) override;
    STDMETHOD(ShowObject)() override;
    STDMETHOD(OnShowWindow)(BOOL fShow) override;
    STDMETHOD(RequestNewObjectLayout)() override;

    // IOleControlSite
    STDMETHOD(OnControlInfoChanged)() override;
    STDMETHOD(LockInPlaceActive)(BOOL fLock) override;
    STDMETHOD(GetExtendedControl)(IDispatch** ppDisp) override;
    STDMETHOD(TransformCoords)(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags) override;
    STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, DWORD grfModifiers) override;
    STDMETHOD(OnFocus)(BOOL fGotFocus) override;
    STDMETHOD(ShowPropertyFrame)() override;

    // IOleWindow
    STDMETHOD(GetWindow)(HWND *phwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // IOleInPlaceSite
    STDMETHOD(CanInPlaceActivate)() override;
    STDMETHOD(OnInPlaceActivate)() override;
    STDMETHOD(OnUIActivate)() override;
    STDMETHOD(GetWindowContext)(IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo) override;
    STDMETHOD(Scroll)(SIZE scrollExtant) override;
    STDMETHOD(OnUIDeactivate)(BOOL fUndoable) override;
    STDMETHOD(OnInPlaceDeactivate)() override;
    STDMETHOD(DiscardUndoState)() override;
    STDMETHOD(DeactivateAndUndo)() override;
    STDMETHOD(OnPosRectChange)(LPCRECT lprcPosRect) override;

#ifdef QAX_SUPPORT_WINDOWLESS
// IOleInPlaceSiteEx ###
    STDMETHOD(OnInPlaceActivateEx)(BOOL* /*pfNoRedraw*/, DWORD /*dwFlags*/) override
    {
        return S_OK;
    }
    STDMETHOD(OnInPlaceDeactivateEx)(BOOL /*fNoRedraw*/) override
    {
        return S_OK;
    }
    STDMETHOD(RequestUIActivate)() override
    {
        return S_OK;
    }

// IOleInPlaceSiteWindowless ###
    STDMETHOD(CanWindowlessActivate)() override
    {
        return S_OK;
    }
    STDMETHOD(GetCapture)() override
    {
        return S_FALSE;
    }
    STDMETHOD(SetCapture)(BOOL /*fCapture*/) override
    {
        return S_FALSE;
    }
    STDMETHOD(GetFocus)() override
    {
        return S_FALSE;
    }
    STDMETHOD(SetFocus)(BOOL /*fCapture*/) override
    {
        return S_FALSE;
    }
    STDMETHOD(GetDC)(LPCRECT /*pRect*/, DWORD /*grfFlags*/, HDC *phDC) override
    {
        *phDC = 0;
        return S_OK;
    }
    STDMETHOD(ReleaseDC)(HDC hDC) override
    {
        ::ReleaseDC((HWND)widget->winId(), hDC);
        return S_OK;
    }
    STDMETHOD(InvalidateRect)(LPCRECT pRect, BOOL fErase) override
    {
        ::InvalidateRect((HWND)host->winId(), pRect, fErase);
        return S_OK;
    }
    STDMETHOD(InvalidateRgn)(HRGN hRGN, BOOL fErase) override
    {
        ::InvalidateRgn((HWND)host->winId(), hRGN, fErase);
        return S_OK;
    }
    STDMETHOD(ScrollRect)(int /*dx*/, int /*dy*/, LPCRECT /*pRectScroll*/, LPCRECT /*pRectClip*/) override
    {
        return S_OK;
    }
    STDMETHOD(AdjustRect)(LPRECT /*prc*/) override
    {
        return S_OK;
    }
#ifdef Q_CC_GNU // signature incorrect in win32api
    STDMETHOD(AdjustRect)(LPCRECT /*prc*/) override
    {
        RECT rect;
        return AdjustRect(&rect);
    }
#endif

    STDMETHOD(OnDefWindowMessage)(UINT /*msg*/, WPARAM /*wPara*/, LPARAM /*lParam*/, LRESULT* /*plResult*/) override
    {
        return S_FALSE;
    }
#endif

    // IOleInPlaceFrame
    STDMETHOD(InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)) override;
    STDMETHOD(SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)) override;
    STDMETHOD(RemoveMenus(HMENU hmenuShared)) override;
    STDMETHOD(SetStatusText(LPCOLESTR pszStatusText)) override;
    STDMETHOD(EnableModeless(BOOL fEnable)) override;
    STDMETHOD(TranslateAccelerator(LPMSG lpMsg, WORD grfModifiers)) override;

    // IOleInPlaceUIWindow
    STDMETHOD(GetBorder(LPRECT lprectBorder)) override;
    STDMETHOD(RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)) override;
    STDMETHOD(SetBorderSpace(LPCBORDERWIDTHS pborderwidths)) override;
    STDMETHOD(SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)) override;

    // IOleDocumentSite
    STDMETHOD(ActivateMe(IOleDocumentView *pViewToActivate)) override;

    // IAdviseSink
    STDMETHOD_(void, OnDataChange)(FORMATETC* /*pFormatetc*/, STGMEDIUM* /*pStgmed*/) override
    {
        AX_DEBUG(QAxClientSite::OnDataChange);
    }
    STDMETHOD_(void, OnViewChange)(DWORD /*dwAspect*/, LONG /*lindex*/) override
    {
        AX_DEBUG(QAxClientSite::OnViewChange);
    }
    STDMETHOD_(void, OnRename)(IMoniker* /*pmk*/) override
    {
    }
    STDMETHOD_(void, OnSave)() override
    {
    }
    STDMETHOD_(void, OnClose)() override
    {
    }

    QSize sizeHint() const { return sizehint; }
    QSize minimumSizeHint() const;
    inline void resize(QSize sz) { if (host) host->resize(sz); }

    bool translateKeyEvent(int message, int keycode) const
    {
        if (!widget)
            return false;
        return widget->translateKeyEvent(message, keycode);
    }

    int qt_metacall(QMetaObject::Call, int isignal, void **argv);
    void windowActivationChange();

    bool eventTranslated : 1;

private:
    struct OleMenuItem {
        OleMenuItem(HMENU hm = nullptr, int ID = 0, QMenu *menu = nullptr)
            : hMenu(hm), subMenu(menu), id(ID)
        {}
        HMENU hMenu;
        QMenu *subMenu;
        int id;
    };
    using MenuItemMap = QMap<QAction*, OleMenuItem>;

    QMenu *generatePopup(HMENU subMenu, QWidget *parent);

    IOleObject *m_spOleObject = nullptr;
    IOleControl *m_spOleControl = nullptr;
    IOleInPlaceObjectWindowless *m_spInPlaceObject = nullptr;
    IOleInPlaceActiveObject *m_spInPlaceActiveObject = nullptr;
    IOleDocumentView *m_spActiveView = nullptr;

    QAxAggregated *aggregatedObject = nullptr;

    bool inPlaceObjectWindowless :1;
    bool inPlaceModelessEnabled :1;
    bool canHostDocument : 1;

    DWORD m_dwOleObject = 0;
    HWND m_menuOwner = nullptr;
    CONTROLINFO control_info;

    QSize sizehint;
    LONG ref = 1;
    QAxWidget *widget;
    QAxHostWidget *host = nullptr;
    QPointer<QMenuBar> menuBar;
    MenuItemMap menuItemMap;
};

static const ushort mouseTbl[] = {
    WM_MOUSEMOVE,       QEvent::MouseMove,              0,
    WM_LBUTTONDOWN,     QEvent::MouseButtonPress,       Qt::LeftButton,
    WM_LBUTTONUP,       QEvent::MouseButtonRelease,     Qt::LeftButton,
    WM_LBUTTONDBLCLK,   QEvent::MouseButtonDblClick,    Qt::LeftButton,
    WM_RBUTTONDOWN,     QEvent::MouseButtonPress,       Qt::RightButton,
    WM_RBUTTONUP,       QEvent::MouseButtonRelease,     Qt::RightButton,
    WM_RBUTTONDBLCLK,   QEvent::MouseButtonDblClick,    Qt::RightButton,
    WM_MBUTTONDOWN,     QEvent::MouseButtonPress,       Qt::MiddleButton,
    WM_MBUTTONUP,       QEvent::MouseButtonRelease,     Qt::MiddleButton,
    WM_MBUTTONDBLCLK,   QEvent::MouseButtonDblClick,    Qt::MiddleButton,
    0,                  0,                              0
};

static Qt::MouseButtons translateMouseButtonState(int s)
{
    Qt::MouseButtons bst;
    if (s & MK_LBUTTON)
        bst |= Qt::LeftButton;
    if (s & MK_MBUTTON)
        bst |= Qt::MiddleButton;
    if (s & MK_RBUTTON)
        bst |= Qt::RightButton;

    return bst;
}

static Qt::KeyboardModifiers translateModifierState(int s)
{
    Qt::KeyboardModifiers bst;
    if (s & MK_SHIFT)
        bst |= Qt::ShiftModifier;
    if (s & MK_CONTROL)
        bst |= Qt::ControlModifier;
    if (GetKeyState(VK_MENU) < 0)
        bst |= Qt::AltModifier;

    return bst;
}

static const wchar_t *qaxatom = L"QAxContainer4_Atom";

// The filter procedure listening to user interaction on the control
class QAxNativeEventFilter : public QAbstractNativeEventFilter
{
public:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *) override;
#else
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *) override;
#endif
};
Q_GLOBAL_STATIC(QAxNativeEventFilter, s_nativeEventFilter)

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool QAxNativeEventFilter::nativeEventFilter(const QByteArray &, void *m, qintptr *)
#else
bool QAxNativeEventFilter::nativeEventFilter(const QByteArray &, void *m, long *)
#endif
{
    MSG *msg = static_cast<MSG *>(m);
    const uint message = msg->message;
    if (message == WM_DISPLAYCHANGE)
        qaxClearCachedSystemLogicalDpi();
    if ((message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) || (message >= WM_KEYFIRST && message <= WM_KEYLAST)) {
        HWND hwnd = msg->hwnd;
        QAxWidget *ax = nullptr;
        QAxHostWidget *host = nullptr;
        while (!host && hwnd) {
            // FIXME: 4.10.2011: Does this still work?
            QWidget *widget = QWidget::find(reinterpret_cast<WId>(hwnd));
            if (widget && widget->inherits("QAxHostWidget"))
                host = qobject_cast<QAxHostWidget*>(widget);
            hwnd = ::GetParent(hwnd);
        }
        if (host)
            ax = qobject_cast<QAxWidget*>(host->parentWidget());
        if (ax && msg->hwnd != reinterpret_cast<HWND>(host->winId())) {
            if (message >= WM_KEYFIRST && message <= WM_KEYLAST) {
                QAxClientSite *site = host->clientSite();
                site->eventTranslated = true; // reset in QAxClientSite::TranslateAccelerator
                HRESULT hres = S_FALSE;
                if (site && site->inPlaceObject() && site->translateKeyEvent(msg->message, msg->wParam))
                    hres = site->inPlaceObject()->TranslateAccelerator(msg);
                // if the object calls our TranslateAccelerator implementation, then continue with normal event processing
                // otherwise the object has translated the accelerator, and the event should be stopped
                if (site->eventTranslated && hres == S_OK)
                    return true;
            } else {
                int i;
                for (i = 0; UINT(mouseTbl[i]) != message && mouseTbl[i]; i += 3)
                    ;

                if (mouseTbl[i]) {
                    const QEvent::Type type = static_cast<QEvent::Type>(mouseTbl[++i]);
                    int button = mouseTbl[++i];
                    if (type != QEvent::MouseMove || ax->hasMouseTracking() || button) {
                        if (type == QEvent::MouseMove)
                            button = 0;

                        DWORD ol_pos = GetMessagePos();
                        const QPoint nativeGlobalPos(GET_X_LPARAM(ol_pos), GET_Y_LPARAM(ol_pos));
                        const QPoint gpos = qaxFromNativePosition(ax, nativeGlobalPos);
                        QPoint pos = ax->mapFromGlobal(gpos);

                        QMouseEvent e(type, pos, gpos, static_cast<Qt::MouseButton>(button),
                            translateMouseButtonState(int(msg->wParam)),
                            translateModifierState(int(msg->wParam)));
                        QCoreApplication::sendEvent(ax, &e);
                    }
                }
            }
        }
    }

    return false;
}

QAxClientSite::QAxClientSite(QAxWidget *c)
: eventTranslated(true), widget(c)
{
    aggregatedObject = widget->createAggregate();
    if (aggregatedObject) {
        aggregatedObject->controlling_unknown = static_cast<IUnknown *>(static_cast<IDispatch *>(this));
        aggregatedObject->the_object = c;
    }

    inPlaceObjectWindowless = false;
    inPlaceModelessEnabled = true;
    canHostDocument = false;

    memset(&control_info, 0, sizeof(control_info));
}

bool QAxClientSite::activateObject(bool initialized, const QByteArray &data)
{
    if (!host)
        host = new QAxHostWidget(widget, this);

    bool showHost = false;
    if (!m_spOleObject)
        widget->queryInterface(IID_IOleObject, reinterpret_cast<void**>(&m_spOleObject));
    if (m_spOleObject) {
        DWORD dwMiscStatus = 0;
        m_spOleObject->GetMiscStatus(DVASPECT_CONTENT, &dwMiscStatus);

        IOleDocument *document = nullptr;
        m_spOleObject->QueryInterface(IID_IOleDocument, reinterpret_cast<void**>(&document));
        if (document) {
            IPersistStorage *persistStorage = nullptr;
            document->QueryInterface(IID_IPersistStorage, reinterpret_cast<void**>(&persistStorage));
            if (persistStorage) {
            // try to activate as document server
                IStorage *storage = nullptr;
                ILockBytes * bytes = nullptr;
                ::CreateILockBytesOnHGlobal(nullptr, TRUE, &bytes);
                ::StgCreateDocfileOnILockBytes(bytes, STGM_SHARE_EXCLUSIVE|STGM_CREATE|STGM_READWRITE, 0, &storage);

                persistStorage->InitNew(storage);
                persistStorage->Release();
                canHostDocument = true;
                storage->Release();
                bytes->Release();

                m_spOleObject->SetClientSite(this);
                OleRun(m_spOleObject);
            }
            document->Release();
        }

        if (!canHostDocument) {
            // activate as control
            if(dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
                m_spOleObject->SetClientSite(this);

            if (!initialized) {
                IPersistStreamInit *spPSI = nullptr;
                m_spOleObject->QueryInterface(IID_IPersistStreamInit, reinterpret_cast<void**>(&spPSI));
                if (spPSI) {
                    if (data.length()) {
                        IStream *pStream = nullptr;
                        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, data.length());
                        if (hGlobal) {
                            if (auto pStByte = GlobalLock(hGlobal))
                                memcpy(pStByte, data.data(), data.length());
                            GlobalUnlock(hGlobal);
                            if (SUCCEEDED(CreateStreamOnHGlobal(hGlobal, TRUE, &pStream))) {
                                spPSI->Load(pStream);
                                pStream->Release();
                            }
                            GlobalFree(hGlobal);
                        }
                    } else {
                        spPSI->InitNew();
                    }
                    spPSI->Release();
                } else if (data.length()) { //try initializing using a IPersistStorage
                    IPersistStorage *spPS = nullptr;
                    m_spOleObject->QueryInterface( IID_IPersistStorage, reinterpret_cast<void **>(&spPS));
                    if (spPS) {
                        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size_t(data.length()));
                        if (hGlobal) {
                            if (BYTE* pbData = static_cast<BYTE *>(GlobalLock(hGlobal)))
                                memcpy(pbData, data.data(), size_t(data.length()));
                            GlobalUnlock(hGlobal);
                            // open an IStorage on the data and pass it to Load
                            LPLOCKBYTES pLockBytes = nullptr;
                            if (SUCCEEDED(CreateILockBytesOnHGlobal(hGlobal, TRUE, &pLockBytes))) {
                                LPSTORAGE pStorage = nullptr;
                                if (SUCCEEDED(StgOpenStorageOnILockBytes(pLockBytes, nullptr,
                                              STGM_READWRITE | STGM_SHARE_EXCLUSIVE, nullptr, 0, &pStorage))) {
                                    spPS->Load(pStorage);
                                    pStorage->Release();
                                }
                                pLockBytes->Release();
                            }
                            GlobalFree(hGlobal);
                        }
                        spPS->Release();
                    }
                }
            }

            if(!(dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST))
                m_spOleObject->SetClientSite(this);
        }

        IViewObject *spViewObject = nullptr;
        m_spOleObject->QueryInterface(IID_IViewObject, reinterpret_cast<void **>(&spViewObject));

        m_spOleObject->Advise(this, &m_dwOleObject);
        IAdviseSink *spAdviseSink = nullptr;
        QueryInterface(IID_IAdviseSink, reinterpret_cast<void **>(&spAdviseSink));
        if (spAdviseSink && spViewObject) {
            if (spViewObject)
                spViewObject->SetAdvise(DVASPECT_CONTENT, 0, spAdviseSink);
        }
        if (spAdviseSink)
            spAdviseSink->Release();
        if (spViewObject)
            spViewObject->Release();

        m_spOleObject->SetHostNames(OLESTR("AXWIN"), nullptr);

        if (!(dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME)) {
            SIZEL hmSize = qaxMapPixToLogHiMetrics(QSize(250, 250), widget);

            m_spOleObject->SetExtent(DVASPECT_CONTENT, &hmSize);
            m_spOleObject->GetExtent(DVASPECT_CONTENT, &hmSize);

            sizehint = qaxMapLogHiMetricsToPix(hmSize, widget);
            showHost = true;
        } else {
            sizehint = QSize(0, 0);
            host->hide();
        }
        if (!(dwMiscStatus & OLEMISC_NOUIACTIVATE)) {
            host->setFocusPolicy(Qt::StrongFocus);
        } else {
            host->setFocusPolicy(Qt::NoFocus);
        }

        RECT rcPos = qaxQRect2Rect(QRect(qaxNativeWidgetPosition(host), qaxToNativeSize(host, sizehint)));

        const HWND hostWnd = reinterpret_cast<HWND>(host->winId());
        m_spOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, nullptr, static_cast<IOleClientSite *>(this), 0,
                              hostWnd,
                              &rcPos);

        HWND controlWnd = {};
        {
            IOleWindow *oleWindow = nullptr;
            m_spOleObject->QueryInterface(IID_IOleWindow, reinterpret_cast<void **>(&oleWindow));
            if (oleWindow) {
                oleWindow->GetWindow(&controlWnd);
                oleWindow->Release();
            }
        }

        if (controlWnd && !GetParent(controlWnd)) {
            // re-parent control window
            // this is necessary if the control is running in a "low integrity" process that isn't
            // permitted to create a child window with hostWnd as parent
            int winStyle = GetWindowLongPtr(controlWnd, GWL_STYLE);
            winStyle &= ~WS_CAPTION; // remove title bar
            winStyle |= WS_CHILD;    // convert to child window
            SetWindowLongPtr(controlWnd, GWL_STYLE, winStyle);
            SetParent(controlWnd, hostWnd);
        }

        if (!m_spOleControl)
            m_spOleObject->QueryInterface(IID_IOleControl, reinterpret_cast<void **>(&m_spOleControl));
        if (m_spOleControl) {
            m_spOleControl->OnAmbientPropertyChange(DISPID_AMBIENT_BACKCOLOR);
            m_spOleControl->OnAmbientPropertyChange(DISPID_AMBIENT_FORECOLOR);
            m_spOleControl->OnAmbientPropertyChange(DISPID_AMBIENT_FONT);
            m_spOleControl->OnAmbientPropertyChange(DISPID_AMBIENT_USERMODE);

            control_info.cb = sizeof(control_info);
            m_spOleControl->GetControlInfo(&control_info);
        }

        BSTR userType;
        HRESULT result = m_spOleObject->GetUserType(USERCLASSTYPE_SHORT, &userType);
        if (result == S_OK) {
            widget->setWindowTitle(QString::fromWCharArray(userType));
            CoTaskMemFree(userType);
        }
    } else {
        IObjectWithSite *spSite = nullptr;
        widget->queryInterface(IID_IObjectWithSite, reinterpret_cast<void **>(&spSite));
        if (spSite) {
            spSite->SetSite(static_cast<IUnknown *>(static_cast<IDispatch *>(this)));
            spSite->Release();
        }
    }

    host->resize(widget->size());
    if (showHost)
        host->show();

    if (host->focusPolicy() != Qt::NoFocus) {
        widget->setFocusProxy(host);
        widget->setFocusPolicy(host->focusPolicy());
    }

    return true;
}

QAxClientSite::~QAxClientSite()
{
    if (host) {
        host->axhost = nullptr;
    }

    if (aggregatedObject)
        aggregatedObject->the_object = nullptr;
    delete aggregatedObject;
    delete host;
}

void QAxClientSite::releaseAll()
{
    if (m_spOleControl)
        m_spOleControl->Release();
    m_spOleControl = nullptr;
    if (m_spOleObject) {
        m_spOleObject->Unadvise(m_dwOleObject);
        m_spOleObject->SetClientSite(nullptr);
        m_spOleObject->Release();
    }
    m_spOleObject = nullptr;
    if (m_spInPlaceObject) m_spInPlaceObject->Release();
    m_spInPlaceObject = nullptr;
    if (m_spInPlaceActiveObject) m_spInPlaceActiveObject->Release();
    m_spInPlaceActiveObject = nullptr;

    inPlaceObjectWindowless = false;
}

void QAxClientSite::deactivate()
{
    if (!m_spInPlaceObject)
        return;

    // InPlaceDeactivate should trigger an OnInPlaceDeactivate callback
    HRESULT hr = m_spInPlaceObject->InPlaceDeactivate();

    // call fails if an out-of-process control crashes
    if (FAILED(hr)) {
        // Call OnInPlaceDeactivate directly to clean up
        OnInPlaceDeactivate();
        // schedule release of QAxClientSite references that were held by the control
        CoDisconnectObject(static_cast<IUnknown *>(static_cast<IDispatch *>(this)), 0);
    }

    Q_ASSERT(m_spInPlaceObject == nullptr);
}

//**** IUnknown
unsigned long WINAPI QAxClientSite::AddRef()
{
    return InterlockedIncrement(&ref);
}

unsigned long WINAPI QAxClientSite::Release()
{
    LONG refCount = InterlockedDecrement(&ref);
    if (!refCount)
        delete this;

    return refCount;
}

HRESULT WINAPI QAxClientSite::QueryInterface(REFIID iid, void **iface)
{
    *iface = nullptr;

    if (iid == IID_IUnknown) {
        *iface = static_cast<IUnknown *>(static_cast<IDispatch *>(this));
    } else {
        HRESULT res = S_OK;
        if (aggregatedObject)
            res = aggregatedObject->queryInterface(iid, iface);
        if (*iface)
            return res;
    }

    if (!(*iface)) {
        if (iid == IID_IDispatch)
            *iface = static_cast<IDispatch *>(this);
        else if (iid == IID_IOleClientSite)
            *iface = static_cast<IOleClientSite *>(this);
        else if (iid == IID_IOleControlSite)
            *iface = static_cast<IOleControlSite *>(this);
        else if (iid == IID_IOleWindow)
            *iface = static_cast<IOleWindow *>(static_cast<IOleInPlaceSite *>(this));
        else if (iid == IID_IOleInPlaceSite)
            *iface = static_cast<IOleInPlaceSite *>(this);
#ifdef QAX_SUPPORT_WINDOWLESS
        else if (iid == IID_IOleInPlaceSiteEx)
            *iface = static_cast<IOleInPlaceSiteEx *>(this);
        else if (iid == IID_IOleInPlaceSiteWindowless)
            *iface = static_cast<IOleInPlaceSiteWindowless *>(this);
#endif
        else if (iid == IID_IOleInPlaceFrame)
            *iface = static_cast<IOleInPlaceFrame *>(this);
        else if (iid == IID_IOleInPlaceUIWindow)
            *iface = static_cast<IOleInPlaceUIWindow *>(this);
        else if (iid == IID_IOleDocumentSite && canHostDocument)
            *iface = static_cast<IOleDocumentSite *>(this);
        else if (iid == IID_IAdviseSink)
            *iface = static_cast<IAdviseSink *>(this);
    }
    if (!*iface)
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

bool qax_runsInDesignMode = false;

//**** IDispatch
HRESULT WINAPI QAxClientSite::Invoke(DISPID dispIdMember,
                                     REFIID /*riid*/,
                                     LCID /*lcid*/,
                                     WORD /*wFlags*/,
                                     DISPPARAMS * /*pDispParams*/,
                                     VARIANT *pVarResult,
                                     EXCEPINFO * /*pExcepInfo*/,
                                     UINT * /*puArgErr*/)
{
    if (!pVarResult)
        return E_POINTER;
    if (!widget || !host)
        return E_UNEXPECTED;

    switch(dispIdMember) {
    case DISPID_AMBIENT_USERMODE:
        pVarResult->vt = VT_BOOL;
        pVarResult->boolVal = !qax_runsInDesignMode;
        return S_OK;

    case DISPID_AMBIENT_AUTOCLIP:
    case DISPID_AMBIENT_SUPPORTSMNEMONICS:
        pVarResult->vt = VT_BOOL;
        pVarResult->boolVal = true;
        return S_OK;

    case DISPID_AMBIENT_SHOWHATCHING:
    case DISPID_AMBIENT_SHOWGRABHANDLES:
    case DISPID_AMBIENT_DISPLAYASDEFAULT:
    case DISPID_AMBIENT_MESSAGEREFLECT:
        pVarResult->vt = VT_BOOL;
        pVarResult->boolVal = false;
        return S_OK;

    case DISPID_AMBIENT_DISPLAYNAME:
        pVarResult->vt = VT_BSTR;
        pVarResult->bstrVal = QStringToBSTR(widget->windowTitle());
        return S_OK;

    case DISPID_AMBIENT_FONT:
        QVariantToVARIANT(widget->font(), *pVarResult);
        return S_OK;

    case DISPID_AMBIENT_BACKCOLOR:
        pVarResult->vt = VT_UI4;
        pVarResult->lVal = QColorToOLEColor(widget->palette().color(widget->backgroundRole()));
        return S_OK;

    case DISPID_AMBIENT_FORECOLOR:
        pVarResult->vt = VT_UI4;
        pVarResult->lVal = QColorToOLEColor(widget->palette().color(widget->foregroundRole()));
        return S_OK;

    case DISPID_AMBIENT_UIDEAD:
        pVarResult->vt = VT_BOOL;
        pVarResult->boolVal = !widget->isEnabled();
        return S_OK;

    default:
        break;
    }

    return DISP_E_MEMBERNOTFOUND;
}

void QAxClientSite::emitAmbientPropertyChange(DISPID dispid)
{
    if (m_spOleControl)
        m_spOleControl->OnAmbientPropertyChange(dispid);
}

//**** IOleClientSite
HRESULT WINAPI QAxClientSite::SaveObject()
{
    return E_NOTIMPL;
}

HRESULT WINAPI QAxClientSite::GetMoniker(DWORD, DWORD, IMoniker **ppmk)
{
    if (!ppmk)
        return E_POINTER;

    *ppmk = nullptr;
    return E_NOTIMPL;
}

HRESULT WINAPI QAxClientSite::GetContainer(LPOLECONTAINER *ppContainer)
{
    if (!ppContainer)
        return E_POINTER;

    *ppContainer = nullptr;
    return E_NOINTERFACE;
}

HRESULT WINAPI QAxClientSite::ShowObject()
{
    return S_OK;
}

HRESULT WINAPI QAxClientSite::OnShowWindow(BOOL /*fShow*/)
{
    return S_OK;
}

HRESULT WINAPI QAxClientSite::RequestNewObjectLayout()
{
    return E_NOTIMPL;
}

//**** IOleControlSite
HRESULT WINAPI QAxClientSite::OnControlInfoChanged()
{
    if (m_spOleControl)
        m_spOleControl->GetControlInfo(&control_info);

    return S_OK;
}

HRESULT WINAPI QAxClientSite::LockInPlaceActive(BOOL /*fLock*/)
{
    AX_DEBUG(QAxClientSite::LockInPlaceActive);
    return S_OK;
}

HRESULT WINAPI QAxClientSite::GetExtendedControl(IDispatch** ppDisp)
{
    if (!ppDisp)
        return E_POINTER;

    *ppDisp = nullptr;
    return E_NOTIMPL;
}

HRESULT WINAPI QAxClientSite::TransformCoords(POINTL* /*pPtlHimetric*/, POINTF* /*pPtfContainer*/, DWORD /*dwFlags*/)
{
    return S_OK;
}

HRESULT WINAPI QAxClientSite::TranslateAccelerator(LPMSG lpMsg, DWORD /*grfModifiers*/)
{
    if (lpMsg->message == WM_KEYDOWN && !lpMsg->wParam)
        return S_OK;

    bool ActiveQtDetected = false;
    bool fromInProcServer = false;
#ifdef GWLP_USERDATA
    LONG_PTR serverType = GetWindowLongPtr(lpMsg->hwnd, GWLP_USERDATA);
#else
    LONG serverType = GetWindowLong(lpMsg->hwnd, GWL_USERDATA);
#endif
    if (serverType == QAX_INPROC_SERVER) {
        ActiveQtDetected = true;
        fromInProcServer = true;
    } else if (serverType == QAX_OUTPROC_SERVER) {
        ActiveQtDetected = true;
        fromInProcServer = false;
    }

    eventTranslated = false;
    if (!ActiveQtDetected || !fromInProcServer) {
        // if the request is coming from an out-of-proc server or a non ActiveQt server,
        // we send the message to the host window. This will make sure this key event
        // comes to Qt for processing.
        SendMessage(reinterpret_cast<HWND>(host->winId()), lpMsg->message, lpMsg->wParam, lpMsg->lParam);
        if (ActiveQtDetected && !fromInProcServer) {
            // ActiveQt based servers will need further processing of the event
            // (eg. <SPACE> key for a checkbox), so we return false.
            return S_FALSE;
        }
    }
    // ActiveQt based in-processes-servers will handle the event properly, so
    // we don't need to send this key event to the host.
    return S_OK;
}

HRESULT WINAPI QAxClientSite::OnFocus(BOOL bGotFocus)
{
    AX_DEBUG(QAxClientSite::OnFocus);
    if (host) {
        host->hasFocus = bGotFocus;
        qApp->removeEventFilter(host);
        if (bGotFocus)
            qApp->installEventFilter(host);
    }
    return S_OK;
}

HRESULT WINAPI QAxClientSite::ShowPropertyFrame()
{
    return E_NOTIMPL;
}

//**** IOleWindow
HRESULT WINAPI QAxClientSite::GetWindow(HWND *phwnd)
{
    if (!phwnd)
        return E_POINTER;

    *phwnd = reinterpret_cast<HWND>(host->winId());
    return S_OK;
}

HRESULT WINAPI QAxClientSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    if (fEnterMode)
        QWhatsThis::enterWhatsThisMode();
    else
        QWhatsThis::leaveWhatsThisMode();

    return S_OK;
}

//**** IOleInPlaceSite
HRESULT WINAPI QAxClientSite::CanInPlaceActivate()
{
    AX_DEBUG(QAxClientSite::CanInPlaceActivate);
    return S_OK;
}

HRESULT WINAPI QAxClientSite::OnInPlaceActivate()
{
    AX_DEBUG(QAxClientSite::OnInPlaceActivate);
    OleLockRunning(m_spOleObject, true, false);
    if (!m_spInPlaceObject) {
/* ### disabled for now
        m_spOleObject->QueryInterface(IID_IOleInPlaceObjectWindowless, (void**) &m_spInPlaceObject);
*/
        if (m_spInPlaceObject) {
            inPlaceObjectWindowless = true;
        } else {
            inPlaceObjectWindowless = false;
            m_spOleObject->QueryInterface(IID_IOleInPlaceObject, reinterpret_cast<void **>(&m_spInPlaceObject));
        }
    }

    return S_OK;
}

HRESULT WINAPI QAxClientSite::OnUIActivate()
{
    AX_DEBUG(QAxClientSite::OnUIActivate);
    return S_OK;
}

HRESULT WINAPI QAxClientSite::GetWindowContext(IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    if (!ppFrame || !ppDoc || !lprcPosRect || !lprcClipRect || !lpFrameInfo)
        return E_POINTER;

    QueryInterface(IID_IOleInPlaceFrame, reinterpret_cast<void **>(ppFrame));
    QueryInterface(IID_IOleInPlaceUIWindow, reinterpret_cast<void **>(ppDoc));

    const HWND hwnd = reinterpret_cast<HWND>(host->winId());
    ::GetClientRect(hwnd, lprcPosRect);
    ::GetClientRect(hwnd, lprcClipRect);

    lpFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
    lpFrameInfo->fMDIApp = false;
    lpFrameInfo->haccel = nullptr;
    lpFrameInfo->cAccelEntries = 0;
    // FIXME: 4.10.2011: Parent's HWND required, should work.
    lpFrameInfo->hwndFrame = widget ? hwnd : nullptr;

    return S_OK;
}

HRESULT WINAPI QAxClientSite::Scroll(SIZE /*scrollExtant*/)
{
    return S_FALSE;
}

HRESULT WINAPI QAxClientSite::OnUIDeactivate(BOOL)
{
    AX_DEBUG(QAxClientSite::OnUIDeactivate);
    if (host && host->hasFocus) {
        qApp->removeEventFilter(host);
        host->hasFocus = false;
    }
    return S_OK;
}

HRESULT WINAPI QAxClientSite::OnInPlaceDeactivate()
{
    AX_DEBUG(QAxClientSite::OnInPlaceDeactivate);
    if (m_spInPlaceObject)
        m_spInPlaceObject->Release();
    m_spInPlaceObject = nullptr;
    inPlaceObjectWindowless = false;
    OleLockRunning(m_spOleObject, false, false);

    return S_OK;
}

HRESULT WINAPI QAxClientSite::DiscardUndoState()
{
    return S_OK;
}

HRESULT WINAPI QAxClientSite::DeactivateAndUndo()
{
    if (m_spInPlaceObject)
        m_spInPlaceObject->UIDeactivate();

    return S_OK;
}

HRESULT WINAPI QAxClientSite::OnPosRectChange(LPCRECT /*lprcPosRect*/)
{
    AX_DEBUG(QAxClientSite::OnPosRectChange);
    // ###
    return S_OK;
}

//**** IOleInPlaceFrame
HRESULT WINAPI QAxClientSite::InsertMenus(HMENU /*hmenuShared*/, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    AX_DEBUG(QAxClientSite::InsertMenus);
    QMenuBar *mb = menuBar;
    if (!mb)
        mb = widget->window()->findChild<QMenuBar*>();
    if (!mb)
        return E_NOTIMPL;
    menuBar = mb;

    QMenu *fileMenu = nullptr;
    QMenu *viewMenu = nullptr;
    QMenu *windowMenu = nullptr;
    const auto actions = menuBar->actions();
    for (QAction *action : actions) {
        QString text = action->text().remove(QLatin1Char('&'));
        if (text == QLatin1String("File")) {
            fileMenu = action->menu();
        } else if (text == QLatin1String("View")) {
            viewMenu = action->menu();
        } else if (text == QLatin1String("Window")) {
            windowMenu = action->menu();
        }
    }
    if (fileMenu)
        lpMenuWidths->width[0] = fileMenu->actions().size();
    if (viewMenu)
        lpMenuWidths->width[2] = viewMenu->actions().size();
    if (windowMenu)
        lpMenuWidths->width[4] = windowMenu->actions().size();

    return S_OK;
}

static int menuItemEntry(HMENU menu, int index, MENUITEMINFO item, QString &text, QPixmap &/*icon*/)
{
    if (item.fType == MFT_STRING && item.cch) {
        wchar_t *titlebuf = new wchar_t[item.cch + 1];
        item.dwTypeData = titlebuf;
        item.cch++;
        ::GetMenuItemInfo(menu, UINT(index), true, &item);
        text = QString::fromWCharArray(titlebuf);
        delete [] titlebuf;
        return MFT_STRING;
    }
#if 0
    else if (item.fType == MFT_BITMAP) {
        HBITMAP hbm = (HBITMAP)LOWORD(item.hbmpItem);
        SIZE bmsize;
        GetBitmapDimensionEx(hbm, &bmsize);
        QPixmap pixmap(1,1);
        QSize sz(MAP_LOGHIM_TO_PIX(bmsize.cx, pixmap.logicalDpiX()),
                 MAP_LOGHIM_TO_PIX(bmsize.cy, pixmap.logicalDpiY()));

        pixmap.resize(bmsize.cx, bmsize.cy);
        if (!pixmap.isNull()) {
            HDC hdc = ::CreateCompatibleDC(pixmap.handle());
            ::SelectObject(hdc, hbm);
            BOOL res = ::BitBlt(pixmap.handle(), 0, 0, pixmap.width(), pixmap.height(), hdc, 0, 0, SRCCOPY);
            ::DeleteObject(hdc);
        }

        icon = pixmap;
    }
#endif
    return -1;
}

QMenu *QAxClientSite::generatePopup(HMENU subMenu, QWidget *parent)
{
    QMenu *popup = nullptr;
    int count = GetMenuItemCount(subMenu);
    if (count)
        popup = new QMenu(parent);
    for (int i = 0; i < count; ++i) {
        MENUITEMINFO item;
        memset(&item, 0, sizeof(MENUITEMINFO));
        item.cbSize = sizeof(MENUITEMINFO);
        item.fMask = MIIM_ID | MIIM_TYPE | MIIM_SUBMENU;
        ::GetMenuItemInfo(subMenu, UINT(i), true, &item);

        QAction *action = nullptr;
        QMenu *popupMenu = nullptr;
        if (item.fType == MFT_SEPARATOR) {
            action = popup->addSeparator();
        } else {
            QString text;
            QPixmap icon;
            QKeySequence accel;
            popupMenu = item.hSubMenu ? generatePopup(item.hSubMenu, popup) : nullptr;
            int res = menuItemEntry(subMenu, i, item, text, icon);

            int lastSep = text.lastIndexOf(QRegularExpression(QLatin1String("[\\s]")));
            if (lastSep != -1) {
                QString keyString = text.right(text.length() - lastSep);
                accel = keyString;
                if (!accel.isEmpty())
                    text.truncate(lastSep);
            }

            if (popupMenu)
                popupMenu->setTitle(text);

            switch (res) {
            case MFT_STRING:
                if (popupMenu)
                    action = popup->addMenu(popupMenu);
                else
                    action = popup->addAction(text);
                break;
            case MFT_BITMAP:
                if (popupMenu)
                    action = popup->addMenu(popupMenu);
                else
                    action = popup->addAction(icon, text);
                break;
            }

            if (action) {
                if (!accel.isEmpty())
                    action->setShortcut(accel);
                if (!icon.isNull())
                    action->setIcon(icon);
            }
        }

        if (action) {
            OleMenuItem oleItem(subMenu, int(item.wID), popupMenu);
            menuItemMap.insert(action, oleItem);
        }
    }
    return popup;
}

HRESULT WINAPI QAxClientSite::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    AX_DEBUG(QAxClientSite::SetMenu);

    if (hmenuShared) {
        m_menuOwner = hwndActiveObject;
        QMenuBar *mb = menuBar;
        if (!mb)
            mb = widget->window()->findChild<QMenuBar*>();
        if (!mb)
            return E_NOTIMPL;
        menuBar = mb;

        int count = GetMenuItemCount(hmenuShared);
        for (int i = 0; i < count; ++i) {
            MENUITEMINFO item;
            memset(&item, 0, sizeof(MENUITEMINFO));
            item.cbSize = sizeof(MENUITEMINFO);
            item.fMask = MIIM_ID | MIIM_TYPE | MIIM_SUBMENU;
            ::GetMenuItemInfo(hmenuShared, UINT(i), true, &item);

            QAction *action = nullptr;
            QMenu *popupMenu = nullptr;
            if (item.fType == MFT_SEPARATOR) {
                action = menuBar->addSeparator();
            } else {
                QString text;
                QPixmap icon;
                popupMenu = item.hSubMenu ? generatePopup(item.hSubMenu, menuBar) : nullptr;
                int res = menuItemEntry(hmenuShared, i, item, text, icon);

                if (popupMenu)
                    popupMenu->setTitle(text);

                switch(res) {
                case MFT_STRING:
                    if (popupMenu)
                        action = menuBar->addMenu(popupMenu);
                    else
                        action = menuBar->addAction(text);
                    break;
                case MFT_BITMAP:
                    if (popupMenu)
                        action = menuBar->addMenu(popupMenu);
                    else
                        action = menuBar->addAction(text);
                    break;
                default:
                    break;
                }
                if (action && !icon.isNull())
                    action->setIcon(icon);
            }

            if (action) {
                OleMenuItem oleItem(hmenuShared, int(item.wID), popupMenu);
                menuItemMap.insert(action, oleItem);
            }
        }
        if (count) {
            const QMetaObject *mbmo = menuBar->metaObject();
            int index = mbmo->indexOfSignal("triggered(QAction*)");
            Q_ASSERT(index != -1);
            menuBar->disconnect(SIGNAL(triggered(QAction*)), host);
            QMetaObject::connect(menuBar, index, host, index);
        }
    } else if (menuBar) {
        m_menuOwner = nullptr;
        for (auto it = menuItemMap.begin(), mend = menuItemMap.end(); it != mend; ++it)
            delete it.key();
        menuItemMap.clear();
    }

    OleSetMenuDescriptor(holemenu, widget ? hwndForWidget(widget) : nullptr, m_menuOwner, this, m_spInPlaceActiveObject);
    return S_OK;
}

int QAxClientSite::qt_metacall(QMetaObject::Call call, int isignal, void **argv)
{
    if (!m_spOleObject || call != QMetaObject::InvokeMetaMethod || !menuBar)
        return isignal;

    if (isignal != menuBar->metaObject()->indexOfSignal("triggered(QAction*)"))
        return isignal;

    QAction *action = *reinterpret_cast<QAction **>(argv[1]);
    // ###

    OleMenuItem oleItem = menuItemMap.value(action);
    if (oleItem.hMenu)
        ::PostMessage(m_menuOwner, WM_COMMAND, WPARAM(oleItem.id), 0);
    return -1;
}


HRESULT WINAPI QAxClientSite::RemoveMenus(HMENU /*hmenuShared*/)
{
    AX_DEBUG(QAxClientSite::RemoveMenus);
    for (auto it = menuItemMap.begin(), mend = menuItemMap.end(); it != mend; ++it) {
        it.key()->setVisible(false);
        delete it.key();
    }
    menuItemMap.clear();
    return S_OK;
}

HRESULT WINAPI QAxClientSite::SetStatusText(LPCOLESTR pszStatusText)
{
    QStatusTipEvent tip(QString::fromWCharArray(pszStatusText));
    QCoreApplication::sendEvent(widget, &tip);
    return S_OK;
}

extern Q_GUI_EXPORT bool qt_win_ignoreNextMouseReleaseEvent;

HRESULT WINAPI QAxClientSite::EnableModeless(BOOL fEnable)
{
    EnableWindow(hwndForWidget(host), fEnable);

    QWindow *hostWindow = host->hostWindow();
    if (!hostWindow)
        return S_FALSE;

    if (!fEnable) {
        if (!QApplicationPrivate::isBlockedByModal(host))
            QGuiApplicationPrivate::showModalWindow(hostWindow);
    } else {
        if (QApplicationPrivate::isBlockedByModal(host))
            QGuiApplicationPrivate::hideModalWindow(hostWindow);
    }
    // FIXME 4.10.2011: No longer exists  in Lighthouse.
    // qt_win_ignoreNextMouseReleaseEvent = false;
    return S_OK;
}

HRESULT WINAPI QAxClientSite::TranslateAccelerator(LPMSG lpMsg, WORD grfModifiers)
{
    return TranslateAccelerator(lpMsg, DWORD(grfModifiers));
}

//**** IOleInPlaceUIWindow
HRESULT WINAPI QAxClientSite::GetBorder(LPRECT lprectBorder)
{
#ifndef QAX_SUPPORT_BORDERSPACE
    Q_UNUSED(lprectBorder);
    return INPLACE_E_NOTOOLSPACE;
#else
    AX_DEBUG(QAxClientSite::GetBorder);

    QMainWindow *mw = qobject_cast<QMainWindow*>(widget->window());
    if (!mw)
        return INPLACE_E_NOTOOLSPACE;

    RECT border = { 0,0, 300, 200 };
    *lprectBorder = border;
    return S_OK;
#endif
}

HRESULT WINAPI QAxClientSite::RequestBorderSpace(LPCBORDERWIDTHS /*pborderwidths*/)
{
#ifndef QAX_SUPPORT_BORDERSPACE
    return INPLACE_E_NOTOOLSPACE;
#else
    AX_DEBUG(QAxClientSite::RequestBorderSpace);

    QMainWindow *mw = qobject_cast<QMainWindow*>(widget->window());
    if (!mw)
        return INPLACE_E_NOTOOLSPACE;

    return S_OK;
#endif
}

HRESULT WINAPI QAxClientSite::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
#ifndef QAX_SUPPORT_BORDERSPACE
    Q_UNUSED(pborderwidths);
    return OLE_E_INVALIDRECT;
#else
    AX_DEBUG(QAxClientSite::SetBorderSpace);

    // object has no toolbars and wants container toolbars to remain
    if (!pborderwidths)
        return S_OK;

    QMainWindow *mw = qobject_cast<QMainWindow*>(widget->window());
    if (!mw)
        return OLE_E_INVALIDRECT;

    bool removeToolBars = !(pborderwidths->left || pborderwidths->top || pborderwidths->right || pborderwidths->bottom);

    // object has toolbars, and wants container to remove toolbars
    if (removeToolBars) {
        if (mw) {
            //### remove our toolbars
        }
    }

    if (pborderwidths->left) {
        QDockWidget *left = new QDockWidget(mw);
        left->setFixedWidth(pborderwidths->left);
        mw->addDockWidget(Qt::LeftDockWidgetArea, left);
        left->show();
    }
    if (pborderwidths->top) {
        QDockWidget *top = new QDockWidget(mw);
        top->setFixedHeight(pborderwidths->top);
        mw->addDockWidget(Qt::TopDockWidgetArea, top);
        top->show();
    }

    return S_OK;
#endif
}

HRESULT WINAPI QAxClientSite::SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    AX_DEBUG(QAxClientSite::SetActiveObject);

    Q_UNUSED(pszObjName);
    // we are ignoring the name of the object, as suggested by MSDN documentation
    // for IOleInPlaceUIWindow::SetActiveObject().

    if (m_spInPlaceActiveObject) {
        if (!inPlaceModelessEnabled)
            m_spInPlaceActiveObject->EnableModeless(true);
        inPlaceModelessEnabled = true;
        m_spInPlaceActiveObject->Release();
    }

    m_spInPlaceActiveObject = pActiveObject;
    if (m_spInPlaceActiveObject)
        m_spInPlaceActiveObject->AddRef();

    return S_OK;
}

//**** IOleDocumentSite
HRESULT WINAPI QAxClientSite::ActivateMe(IOleDocumentView *pViewToActivate)
{
    AX_DEBUG(QAxClientSite::ActivateMe);

    if (m_spActiveView)
        m_spActiveView->Release();
    m_spActiveView = nullptr;

    if (!pViewToActivate) {
        IOleDocument *document = nullptr;
        m_spOleObject->QueryInterface(IID_IOleDocument, reinterpret_cast<void **>(&document));
        if (!document)
            return E_FAIL;

        document->CreateView(this, nullptr, 0, &pViewToActivate);

        document->Release();
        if (!pViewToActivate)
            return E_OUTOFMEMORY;
    } else {
        pViewToActivate->SetInPlaceSite(this);
    }

    m_spActiveView = pViewToActivate;
    m_spActiveView->AddRef();

    m_spActiveView->UIActivate(TRUE);

    RECT rect;
    GetClientRect(HWND(widget->winId()), &rect);
    m_spActiveView->SetRect(&rect);
    m_spActiveView->Show(TRUE);

    return S_OK;
}

QSize QAxClientSite::minimumSizeHint() const
{
    if (!m_spOleObject)
        return QSize();

    SIZE sz = { 0, 0 };
    m_spOleObject->SetExtent(DVASPECT_CONTENT, &sz);
    return SUCCEEDED(m_spOleObject->GetExtent(DVASPECT_CONTENT, &sz))
        ? qaxMapLogHiMetricsToPix(sz, widget) : QSize();
}

void QAxClientSite::windowActivationChange()
{
    AX_DEBUG(QAxClientSite::windowActivationChange);

    if (m_spInPlaceActiveObject && widget) {
        QWidget *modal = QApplication::activeModalWidget();
        if (modal && inPlaceModelessEnabled) {
            m_spInPlaceActiveObject->EnableModeless(false);
            inPlaceModelessEnabled = false;
        } else if (!inPlaceModelessEnabled) {
            m_spInPlaceActiveObject->EnableModeless(true);
            inPlaceModelessEnabled = true;
        }
        m_spInPlaceActiveObject->OnFrameWindowActivate(widget->isActiveWindow());
    }
}


//**** QWidget

QAxHostWidget::QAxHostWidget(QWidget *parent, QAxClientSite *ax)
: QWidget(parent), setFocusTimer(0), hasFocus(false), axhost(ax)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_PaintOnScreen);

    setObjectName(parent->objectName() + QLatin1String(" - QAxHostWidget"));
}

QAxHostWidget::~QAxHostWidget()
{
    if (axhost)
        axhost->reset(this);
}

int QAxHostWidget::qt_metacall(QMetaObject::Call call, int isignal, void **argv)
{
    if (axhost)
        return axhost->qt_metacall(call, isignal, argv);
    return -1;
}

void* QAxHostWidget::qt_metacast(const char *clname)
{
    if (!clname) return nullptr;
    if (!qstrcmp(clname,"QAxHostWidget"))
        return static_cast<void*>(const_cast< QAxHostWidget*>(this));
    return QWidget::qt_metacast(clname);
}

QWindow *QAxHostWidget::hostWindow() const
{
    if (QWindow *w = windowHandle())
        return w;
    if (QWidget *parent = nativeParentWidget())
        return parent->windowHandle();
    return nullptr;
}

QSize QAxHostWidget::sizeHint() const
{
    return axhost ? axhost->sizeHint() : QWidget::sizeHint();
}

QSize QAxHostWidget::minimumSizeHint() const
{
    QSize size;
    if (axhost)
        size = axhost->minimumSizeHint();
    if (size.isValid())
        return size;
    return QWidget::minimumSizeHint();
}

void QAxHostWidget::resizeObject()
{
    if (!axhost)
        return;

    // document server - talk to view?
    if (axhost->m_spActiveView) {
        RECT rect;
        GetClientRect(reinterpret_cast<HWND>(winId()), &rect);
        axhost->m_spActiveView->SetRect(&rect);

        return;
    }

    SIZEL hmSize = qaxMapPixToLogHiMetrics(size(), this);
    if (axhost->m_spOleObject)
        axhost->m_spOleObject->SetExtent(DVASPECT_CONTENT, &hmSize);
    if (axhost->m_spInPlaceObject) {
        RECT rcPos = qaxNativeWidgetRect(this);
        axhost->m_spInPlaceObject->SetObjectRects(&rcPos, &rcPos);
    }
}

void QAxHostWidget::resizeEvent(QResizeEvent *)
{
    resizeObject();
}

void QAxHostWidget::showEvent(QShowEvent *)
{
    resizeObject();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool QAxHostWidget::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
#else
bool QAxHostWidget::nativeEvent(const QByteArray &eventType, void *message, long *result)
#endif
{
    if (axhost && axhost->inPlaceObjectWindowless
        && eventType == QByteArrayLiteral("windows_generic_MSG")) {
        Q_ASSERT(axhost->m_spInPlaceObject);
        MSG *msg = static_cast<MSG *>(message);
        IOleInPlaceObjectWindowless *windowless = axhost->m_spInPlaceObject;
        Q_ASSERT(windowless);
        LRESULT lres;
        HRESULT hres = windowless->OnWindowMessage(msg->message, msg->wParam, msg->lParam, &lres);
        if (hres == S_OK)
            return true;
    }

    QWidget::nativeEvent(eventType, message, result);
    return false;
}

bool QAxHostWidget::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Timer:
        if (axhost && static_cast<const QTimerEvent *>(e)->timerId() == setFocusTimer) {
            killTimer(setFocusTimer);
            setFocusTimer = 0;
            RECT rcPos = qaxNativeWidgetRect(this);
            axhost->m_spOleObject->DoVerb(OLEIVERB_UIACTIVATE, nullptr, static_cast<IOleClientSite *>(axhost), 0,
                                          reinterpret_cast<HWND>(winId()), &rcPos);
            if (axhost->m_spActiveView)
                axhost->m_spActiveView->UIActivate(TRUE);
        }
        break;
    case QEvent::WindowBlocked:
        if (IsWindowEnabled(reinterpret_cast<HWND>(winId()))) {
            EnableWindow(reinterpret_cast<HWND>(winId()), false);
            if (axhost && axhost->m_spInPlaceActiveObject) {
                axhost->inPlaceModelessEnabled = false;
                axhost->m_spInPlaceActiveObject->EnableModeless(false);
            }
        }
        break;
    case QEvent::WindowUnblocked:
        if (!IsWindowEnabled(reinterpret_cast<HWND>(winId()))) {
            EnableWindow(reinterpret_cast<HWND>(winId()), true);
            if (axhost && axhost->m_spInPlaceActiveObject) {
                axhost->inPlaceModelessEnabled = true;
                axhost->m_spInPlaceActiveObject->EnableModeless(true);
            }
        }
        break;
    default:
        break;
    }

    return QWidget::event(e);
}

bool QAxHostWidget::eventFilter(QObject *o, QEvent *e)
{
    // focus goes to Qt while ActiveX still has it - deactivate
    QWidget *newFocus = qobject_cast<QWidget*>(o);
    if (e->type() == QEvent::FocusIn && hasFocus
        && newFocus && newFocus->window() == window()) {
        if (axhost && axhost->m_spInPlaceActiveObject && axhost->m_spInPlaceObject)
            axhost->m_spInPlaceObject->UIDeactivate();
        qApp->removeEventFilter(this);
    }

    return QWidget::eventFilter(o, e);
}

void QAxHostWidget::focusInEvent(QFocusEvent *e)
{
    QWidget::focusInEvent(e);

    if (!axhost || !axhost->m_spOleObject)
        return;

    // this is called by QWidget::setFocus which calls ::SetFocus on "this",
    // so we have to UIActivate the control after all that had happend.
    AX_DEBUG(Setting focus on in-place object);
    setFocusTimer = startTimer(0);
}

void QAxHostWidget::focusOutEvent(QFocusEvent *e)
{
    QWidget::focusOutEvent(e);
    if (setFocusTimer) {
        killTimer(setFocusTimer);
        setFocusTimer = 0;
    }
    if (e->reason() == Qt::PopupFocusReason || e->reason() == Qt::MenuBarFocusReason)
        return;

    if (!axhost || !axhost->m_spInPlaceActiveObject || !axhost->m_spInPlaceObject)
        return;

    AX_DEBUG(Deactivating in-place object);
    axhost->m_spInPlaceObject->UIDeactivate();
}

void QAxHostWidget::paintEvent(QPaintEvent*)
{
    // QWidget having redirected paint device indicates non-regular paint, which implies
    // somebody is grabbing the widget instead of painting it to screen.
    QPoint dummyOffset(0, 0);
    if (!redirected(&dummyOffset))
        return;

    IViewObject *view = nullptr;
    if (axhost)
        axhost->widget->queryInterface(IID_IViewObject, reinterpret_cast<void **>(&view));
    if (!view)
        return;

    QPixmap pm(qaxNativeWidgetSize(this));
    pm.fill();

    HBITMAP hBmp = qt_pixmapToWinHBITMAP(pm);
    const HDC displayDc = GetDC(nullptr);
    HDC hBmp_hdc = CreateCompatibleDC(displayDc);
    HGDIOBJ old_hBmp = SelectObject(hBmp_hdc, hBmp);

    RECTL bounds;
    bounds.left = 0;
    bounds.right = pm.width();
    bounds.top = 0;
    bounds.bottom = pm.height();

    view->Draw(DVASPECT_CONTENT, -1, nullptr, nullptr, nullptr, hBmp_hdc, &bounds, nullptr, nullptr /*fptr*/, 0);
    view->Release();

    QPainter painter(this);
    QPixmap pixmap = qt_pixmapFromWinHBITMAP(hBmp);
    pixmap.setDevicePixelRatio(devicePixelRatio());
    painter.drawPixmap(0, 0, pixmap);

    SelectObject(hBmp_hdc, old_hBmp);
    DeleteObject(hBmp);
    DeleteDC(hBmp_hdc);
    ReleaseDC(nullptr, displayDc);
}

/*!
   \class QAxBaseWidget
   \brief QAxBaseWidget provides static properties and signals for QAxWidget.
   \inmodule QAxContainer
   \since 6.0
*/

/*!
    \property QAxBaseWidget::classContext
    \brief the context the ActiveX control will run in (default CLSCTX_SERVER).

    The property affects the "dwClsContext" argument when calling
    CoCreateInstance. This can be used to control in-proc vs. out-of-proc
    startup for controls supporting both alternatives. Also, it can be used to
    modify/reduce control permissions when used with CLSCTX_ENABLE_CLOAKING
    and an impersonation token.

    Note that it must be set before setControl() to have any effect.
    \sa control
*/

/*!
    \property QAxBaseWidget::control
    \brief the name of the COM object wrapped by this QAxBaseWidget object.

    Setting this property initializes the COM object. Any COM object
    previously set is shut down.

    The most efficient way to set this property is by using the
    registered component's UUID, e.g.

    \snippet src_activeqt_container_qaxbase.cpp 7

    The second fastest way is to use the registered control's class
    name (with or without version number), e.g.

    \snippet src_activeqt_container_qaxbase.cpp 8

    The slowest, but easiest way to use is to use the control's full
    name, e.g.

    \snippet src_activeqt_container_qaxbase.cpp 9

    It is also possible to initialize the object from a file, e.g.

    \snippet src_activeqt_container_qaxbase.cpp 10

    If the component's UUID is used the following patterns can be used
    to initialize the control on a remote machine, to initialize a
    licensed control or to connect to a running object:
    \list
    \li To initialize the control on a different machine use the following
    pattern:

    \snippet src_activeqt_container_qaxbase.cpp 11

    \li To initialize a licensed control use the following pattern:

    \snippet src_activeqt_container_qaxbase.cpp 12

    \li To connect to an already running object use the following pattern:

    \snippet src_activeqt_container_qaxbase.cpp 13

    \endlist
    The first two patterns can be combined, e.g. to initialize a licensed
    control on a remote machine:

    \snippet src_activeqt_container_qaxbase.cpp 14

    The control's read function always returns the control's UUID, if provided including the license
    key, and the name of the server, but not including the username, the domain or the password.

    \sa classContext
*/

/*!
    \fn void QAxBaseWidget::signal(const QString &name, int argc, void *argv)

    This generic signal gets emitted when the COM object issues the
    event \a name. \a argc is the number of parameters provided by the
    event (DISPPARAMS.cArgs), and \a argv is the pointer to the
    parameter values (DISPPARAMS.rgvarg). Note that the order of parameter
    values is turned around, ie. the last element of the array is the first
    parameter in the function.

    \snippet src_activeqt_container_qaxbase.cpp 20

    Use this signal if the event has parameters of unsupported data
    types. Otherwise, connect directly to the signal \a name.

    \sa QAxBaseObject::signal()
*/

/*!
    \fn void QAxBaseWidget::propertyChanged(const QString &name)

    If the COM object supports property notification, this signal gets
    emitted when the property called \a name is changed.

    \sa QAxBaseObject::propertyChanged()
*/

/*!
    \fn void QAxBaseWidget::exception(int code, const QString &source, const QString &desc, const QString &help)

    This signal is emitted when the COM object throws an exception while called using the OLE automation
    interface IDispatch. \a code, \a source, \a desc and \a help provide information about the exception as
    provided by the COM server and can be used to provide useful feedback to the end user. \a help includes
    the help file, and the help context ID in brackets, e.g. "filename [id]".

    \sa QAxBaseObject::exception()
*/

QAxBaseWidget::QAxBaseWidget(QWidgetPrivate &d, QWidget *parent, Qt::WindowFlags f)
    : QWidget(d, parent, f)
{
}

/*!
    \class QAxWidget
    \brief The QAxWidget class is a QWidget that wraps an ActiveX control.

    \inmodule QAxContainer

    A QAxWidget can be instantiated as an empty object, with the name
    of the ActiveX control it should wrap, or with an existing
    interface pointer to the ActiveX control. The ActiveX control's
    properties, methods and events which only use QAxBase
    supported data types, become available as Qt properties,
    slots and signals. The base class QAxBase provides an API to
    access the ActiveX directly through the \c IUnknown pointer.

    QAxWidget is a QWidget and can mostly be used as such, e.g. it can be
    organized in a widget hierarchy and layouts or act as an event filter.
    Standard widget properties, e.g. \link QWidget::enabled
    enabled \endlink are supported, but it depends on the ActiveX
    control to implement support for ambient properties like e.g.
    palette or font. QAxWidget tries to provide the necessary hints.

    However, you cannot reimplement Qt-specific event handlers like
    mousePressEvent or keyPressEvent and expect them to be called reliably.
    The embedded control covers the QAxWidget completely, and usually
    handles the user interface itself. Use control-specific APIs (i.e. listen
    to the signals of the control), or use standard COM techniques like
    window procedure subclassing.

    QAxWidget also inherits most of its ActiveX-related functionality
    from QAxBase, notably dynamicCall() and querySubObject().

    \warning
    You can subclass QAxWidget, but you cannot use the \c Q_OBJECT macro
    in the subclass (the generated moc-file will not compile), so you
    cannot add further signals, slots or properties. This limitation
    is due to the metaobject information generated in runtime. To work
    around this problem, aggregate the QAxWidget as a member of the
    QObject subclass.

    \sa QAxBase, QAxObject, QAxScript, {ActiveQt Framework}
*/

/*!
    Creates an empty QAxWidget widget and propagates \a parent
    and \a f to the QWidget constructor. To initialize a control,
    call setControl().
*/
QAxWidget::QAxWidget(QWidget *parent, Qt::WindowFlags f)
: QAxBaseWidget(*new QAxWidgetPrivate, parent, f)
{
    Q_D(QAxWidget);
    axBaseInit(d);
}

/*!
    Creates an QAxWidget widget and initializes the ActiveX control \a c.
    \a parent and \a f are propagated to the QWidget contructor.

    \sa setControl()
*/
QAxWidget::QAxWidget(const QString &c, QWidget *parent, Qt::WindowFlags f)
: QAxBaseWidget(*new QAxWidgetPrivate, parent, f)
{
    Q_D(QAxWidget);
    axBaseInit(d);
    setControl(c);
}

/*!
    Creates a QAxWidget that wraps the COM object referenced by \a iface.
    \a parent and \a f are propagated to the QWidget contructor.
*/
QAxWidget::QAxWidget(IUnknown *iface, QWidget *parent, Qt::WindowFlags f)
: QAxBaseWidget(*new QAxWidgetPrivate, parent, f)
{
    Q_D(QAxWidget);
    axBaseInit(d, iface);
}

/*!
    Shuts down the ActiveX control and destroys the QAxWidget widget,
    cleaning up all allocated resources.

    \sa clear()
*/
QAxWidget::~QAxWidget()
{
    Q_D(QAxWidget);
    if (d->container)
        d->container->reset(this);
    d->clear();
}

/*!
    \since 4.2

    Calls QAxBase::initialize(\a ptr), and embeds the control in this
    widget by calling createHostWindow(false) if successful.

    To initialize the control before it is activated, reimplement this
    function and add your initialization code before you call
    createHostWindow(true).

    Returns \c true on success, \c false otherwise.
*/
bool QAxWidget::initialize(IUnknown **ptr)
{
    if (!QAxBase::initialize(ptr))
        return false;

    return createHostWindow(false); // assume that control is not initialized
}

/*!
    Creates the client site for the ActiveX control, and returns true if
    the control could be embedded successfully, otherwise returns false.
    If \a initialized is true the control has already been initialized.

    This function is called by initialize(). If you reimplement initialize
    to customize the actual control instantiation, call this function in your
    reimplementation to have the control embedded by the default client side.
    Creates the client site for the ActiveX control, and returns true if
    the control could be embedded successfully, otherwise returns false.
*/
bool QAxWidget::createHostWindow(bool initialized)
{
    return createHostWindow(initialized, QByteArray());
}

/*!
    \since 4.4

    Creates the client site for the ActiveX control, and returns true if
    the control could be embedded successfully, otherwise returns false.
    If \a initialized is false the control will be initialized using the
    \a data. The control will be initialized through either IPersistStreamInit
    or IPersistStorage interface.

    If the control needs to be initialized using custom data, call this function
    in your reimplementation of initialize(). This function is not called by
    the  default implementation of initialize().
*/
bool QAxWidget::createHostWindow(bool initialized, const QByteArray &data)
{
    Q_D(QAxWidget);
    if (!d->container) // Potentially called repeatedly from QAxBase::metaObject(), QAxWidget::initialize()
        d->container = new QAxClientSite(this);

    d->container->activateObject(initialized, data);

    ATOM filter_ref = FindAtom(qaxatom);
    if (!filter_ref)
        QAbstractEventDispatcher::instance()->installNativeEventFilter(s_nativeEventFilter());
    AddAtom(qaxatom);

    if (parentWidget())
        QApplication::postEvent(parentWidget(), new QEvent(QEvent::LayoutRequest));

    return true;
}

/*!
    Reimplement this function when you want to implement additional
    COM interfaces for the client site of the ActiveX control, or when
    you want to provide alternative implementations of COM interfaces.
    Return a new object of a QAxAggregated subclass.

    The default implementation returns the null pointer.
*/
QAxAggregated *QAxWidget::createAggregate()
{
    return nullptr;
}

ulong QAxWidget::classContext() const
{
    return QAxBase::classContext();
}

void QAxWidget::setClassContext(ulong classContext)
{
    QAxBase::setClassContext(classContext);
}

QString QAxWidget::control() const
{
    return QAxBase::control();
}

bool QAxWidget::setControl(const QString &c)
{
    return QAxBase::setControl(c);
}

/*!
    Shuts down the ActiveX control.
    \sa resetControl()
*/
void QAxWidget::clear()
{
    resetControl();
}

/*!
    \reimp

    Shuts down the ActiveX control.
*/
void QAxWidget::resetControl()
{
    Q_D(QAxWidget);
    d->clear();
}

void QAxWidgetPrivate::clear()
{
    Q_Q(QAxWidget);
    if (q->isNull())
        return;
    if (!q->QAxBase::control().isEmpty()) {
        ATOM filter_ref = FindAtom(qaxatom);
        if (filter_ref)
            DeleteAtom(filter_ref);
        filter_ref = FindAtom(qaxatom);
        if (!filter_ref)
            QAbstractEventDispatcher::instance()->removeNativeEventFilter(s_nativeEventFilter());
    }

    if (container)
        container->deactivate();

    q->QAxBase::clear();
    q->setFocusPolicy(Qt::NoFocus);

    if (container) {
        container->releaseAll();
        container->Release();
    }
    container = nullptr;
}

/*!
    \since 4.1

    Requests the ActiveX control to perform the action \a verb. The
    possible verbs are returned by verbs().

    The function returns true if the object could perform the action, otherwise returns false.
*/
bool QAxWidget::doVerb(const QString &verb)
{
    Q_D(QAxWidget);
    if (!verbs().contains(verb))
        return false;

    return d->container->doVerb(indexOfVerb(verb)) == S_OK;
}

 /*!
    \fn QObject *QAxWidget::qObject() const
    \internal
*/

/*!
    \internal
*/
void QAxWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    QAxBasePrivate::qtStaticMetaCall(static_cast<QAxWidget *>(_o), _c, _id, _a);
}

const QMetaObject *QAxWidgetPrivate::fallbackMetaObject() const
{
    return &QAxWidget::staticMetaObject;
}

/*!
    \internal
*/
const QMetaObject *QAxWidget::metaObject() const
{
    return QAxBase::axBaseMetaObject();
}

const QMetaObject *QAxWidgetPrivate::parentMetaObject() const
{
    return &QAxBaseWidget::staticMetaObject;
}

/*!
    \internal
*/
void *QAxWidget::qt_metacast(const char *cname)
{
    if (!qstrcmp(cname, "QAxWidget")) return static_cast<void *>(this);
    if (!qstrcmp(cname, "QAxBase")) return static_cast<QAxBase *>(this);
    return QAxBaseWidget::qt_metacast(cname);
}

QObject* QAxWidgetPrivate::qObject() const
{
    Q_Q(const QAxWidget);
    return static_cast<QObject *>(const_cast<QAxWidget *>(q));
}

const char *QAxWidgetPrivate::className() const
{
    return "QAxWidget";
}

/*!
    \internal
*/
int QAxWidget::qt_metacall(QMetaObject::Call call, int id, void **v)
{
    Q_D(QAxWidget);
    id = QAxBaseWidget::qt_metacall(call, id, v);
    if (id < 0)
        return id;
    return d->qtMetaCall(call, id, v);
}

/*!
    \reimp
*/
QSize QAxWidget::sizeHint() const
{
    Q_D(const QAxWidget);
    if (d->container) {
        QSize sh = d->container->sizeHint();
        if (sh.isValid())
            return sh;
    }

    return QWidget::sizeHint();
}

/*!
    \reimp
*/
QSize QAxWidget::minimumSizeHint() const
{
    Q_D(const QAxWidget);
    if (d->container) {
        QSize sh = d->container->minimumSizeHint();
        if (sh.isValid())
            return sh;
    }

    return QWidget::minimumSizeHint();
}

/*!
    \reimp
*/
void QAxWidget::changeEvent(QEvent *e)
{
    Q_D(QAxWidget);
    if (isNull() || !d->container)
        return;

    switch (e->type()) {
    case QEvent::EnabledChange:
        d->container->emitAmbientPropertyChange(DISPID_AMBIENT_UIDEAD);
        break;
    case QEvent::FontChange:
        d->container->emitAmbientPropertyChange(DISPID_AMBIENT_FONT);
        break;
    case QEvent::PaletteChange:
        d->container->emitAmbientPropertyChange(DISPID_AMBIENT_BACKCOLOR);
        d->container->emitAmbientPropertyChange(DISPID_AMBIENT_FORECOLOR);
        break;
    case QEvent::ActivationChange:
        d->container->windowActivationChange();
        break;
    default:
        break;
    }
}

/*!
    \reimp
*/
void QAxWidget::resizeEvent(QResizeEvent *)
{
    Q_D(QAxWidget);
    if (d->container)
        d->container->resize(size());
}

/*!
    \reimp
*/
void QAxWidget::connectNotify(const QMetaMethod &)
{
    QAxBase::connectNotify();
}


/*!
    Reimplement this function to pass certain key events to the
    ActiveX control. \a message is the Window message identifier
    specifying the message type (ie. WM_KEYDOWN), and \a keycode is
    the virtual keycode (ie. VK_TAB).

    If the function returns true the key event is passed on to the
    ActiveX control, which then either processes the event or passes
    the event on to Qt.

    If the function returns false the processing of the key event is
    ignored by ActiveQt, ie. the ActiveX control might handle it or
    not.

    The default implementation returns true for the following cases:

    \table
    \header
    \li WM_SYSKEYDOWN
    \li WM_SYSKEYUP
    \li WM_KEYDOWN
    \row
    \li All keycodes
    \li VK_MENU
    \li VK_TAB, VK_DELETE and all non-arrow-keys in combination with VK_SHIFT,
       VK_CONTROL or VK_MENU
    \endtable

    This table is the result of experimenting with popular ActiveX controls,
    ie. Internet Explorer and Microsoft Office applications, but for some
    controls it might require modification.
*/
bool QAxWidget::translateKeyEvent(int message, int keycode) const
{
    bool translate = false;

    switch (message) {
    case WM_SYSKEYDOWN:
        translate = true;
        break;
    case WM_KEYDOWN:
        translate = keycode == VK_TAB
            || keycode == VK_DELETE;
        if (!translate) {
            int state = 0;
            if (GetKeyState(VK_SHIFT) < 0)
                state |= 0x01;
            if (GetKeyState(VK_CONTROL) < 0)
                state |= 0x02;
            if (GetKeyState(VK_MENU) < 0)
                state |= 0x04;
            if (state) {
                state = keycode < VK_LEFT || keycode > VK_DOWN;
            }
            translate = state;
        }
        break;
    case WM_SYSKEYUP:
        translate = keycode == VK_MENU;
        break;
    }

    return translate;
}

QT_END_NAMESPACE
