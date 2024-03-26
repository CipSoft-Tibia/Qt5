// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtextbrowser.h"
#include "qtextedit_p.h"

#include <qstack.h>
#include <qapplication.h>
#include <private/qapplication_p.h>
#include <qevent.h>
#include <qdebug.h>
#include <qabstracttextdocumentlayout.h>
#include "private/qtextdocumentlayout_p.h"
#include <qpainter.h>
#include <qdir.h>
#if QT_CONFIG(whatsthis)
#include <qwhatsthis.h>
#endif
#include <qtextobject.h>
#include <qdesktopservices.h>
#include <qstringconverter.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static inline bool shouldEnableInputMethod(QTextBrowser *texbrowser)
{
#if defined (Q_OS_ANDROID)
    return !texbrowser->isReadOnly() || (texbrowser->textInteractionFlags() & Qt::TextSelectableByMouse);
#else
    return !texbrowser->isReadOnly();
#endif
}

Q_LOGGING_CATEGORY(lcBrowser, "qt.text.browser")

class QTextBrowserPrivate : public QTextEditPrivate
{
    Q_DECLARE_PUBLIC(QTextBrowser)
public:
    inline QTextBrowserPrivate()
        : textOrSourceChanged(false), forceLoadOnSourceChange(false), openExternalLinks(false),
          openLinks(true)
#ifdef QT_KEYPAD_NAVIGATION
        , lastKeypadScrollValue(-1)
#endif
    {}

    void init();

    struct HistoryEntry {
        inline HistoryEntry()
            : hpos(0), vpos(0), focusIndicatorPosition(-1),
              focusIndicatorAnchor(-1) {}
        QUrl url;
        QString title;
        int hpos;
        int vpos;
        int focusIndicatorPosition, focusIndicatorAnchor;
        QTextDocument::ResourceType type = QTextDocument::UnknownResource;
    };

    HistoryEntry history(int i) const
    {
        if (i <= 0)
            if (-i < stack.size())
                return stack[stack.size()+i-1];
            else
                return HistoryEntry();
        else
            if (i <= forwardStack.size())
                return forwardStack[forwardStack.size()-i];
            else
                return HistoryEntry();
    }


    HistoryEntry createHistoryEntry() const;
    void restoreHistoryEntry(const HistoryEntry &entry);

    QStack<HistoryEntry> stack;
    QStack<HistoryEntry> forwardStack;
    QUrl home;
    QUrl currentURL;

    QStringList searchPaths;

    /*flag necessary to give the linkClicked() signal some meaningful
      semantics when somebody connected to it calls setText() or
      setSource() */
    bool textOrSourceChanged;
    bool forceLoadOnSourceChange;

    bool openExternalLinks;
    bool openLinks;

    QTextDocument::ResourceType currentType;

#ifndef QT_NO_CURSOR
    QCursor oldCursor;
#endif

    QString findFile(const QUrl &name) const;

    inline void _q_documentModified()
    {
        textOrSourceChanged = true;
        forceLoadOnSourceChange = !currentURL.path().isEmpty();
    }

    void _q_activateAnchor(const QString &href);
    void _q_highlightLink(const QString &href);

    void setSource(const QUrl &url, QTextDocument::ResourceType type);

    // re-imlemented from QTextEditPrivate
    virtual QUrl resolveUrl(const QUrl &url) const override;
    inline QUrl resolveUrl(const QString &url) const
    { return resolveUrl(QUrl(url)); }

#ifdef QT_KEYPAD_NAVIGATION
    void keypadMove(bool next);
    QTextCursor prevFocus;
    int lastKeypadScrollValue;
#endif
    void emitHighlighted(const QUrl &url)
    {
        Q_Q(QTextBrowser);
        emit q->highlighted(url);
    }
};
Q_DECLARE_TYPEINFO(QTextBrowserPrivate::HistoryEntry, Q_RELOCATABLE_TYPE);

QString QTextBrowserPrivate::findFile(const QUrl &name) const
{
    QString fileName;
    if (name.scheme() == "qrc"_L1) {
        fileName = ":/"_L1 + name.path();
    } else if (name.scheme().isEmpty()) {
        fileName = name.path();
    } else {
#if defined(Q_OS_ANDROID)
        if (name.scheme() == "assets"_L1)
            fileName = "assets:"_L1 + name.path();
        else
#endif
            fileName = name.toLocalFile();
    }

    if (fileName.isEmpty())
        return fileName;

    if (QFileInfo(fileName).isAbsolute())
        return fileName;

    for (QString path : std::as_const(searchPaths)) {
        if (!path.endsWith(u'/'))
            path.append(u'/');
        path.append(fileName);
        if (QFileInfo(path).isReadable())
            return path;
    }

    return fileName;
}

QUrl QTextBrowserPrivate::resolveUrl(const QUrl &url) const
{
    if (!url.isRelative())
        return url;

    // For the second case QUrl can merge "#someanchor" with "foo.html"
    // correctly to "foo.html#someanchor"
    if (!(currentURL.isRelative()
          || (currentURL.scheme() == "file"_L1
              && !QFileInfo(currentURL.toLocalFile()).isAbsolute()))
          || (url.hasFragment() && url.path().isEmpty())) {
        return currentURL.resolved(url);
    }

    // this is our last resort when current url and new url are both relative
    // we try to resolve against the current working directory in the local
    // file system.
    QFileInfo fi(currentURL.toLocalFile());
    if (fi.exists()) {
        return QUrl::fromLocalFile(fi.absolutePath() + QDir::separator()).resolved(url);
    }

    return url;
}

void QTextBrowserPrivate::_q_activateAnchor(const QString &href)
{
    if (href.isEmpty())
        return;
    Q_Q(QTextBrowser);

#ifndef QT_NO_CURSOR
    viewport->setCursor(oldCursor);
#endif

    const QUrl url = resolveUrl(href);

    if (!openLinks) {
        emit q->anchorClicked(url);
        return;
    }

    textOrSourceChanged = false;

#ifndef QT_NO_DESKTOPSERVICES
    bool isFileScheme =
            url.scheme() == "file"_L1
#if defined(Q_OS_ANDROID)
            || url.scheme() == "assets"_L1
#endif
            || url.scheme() == "qrc"_L1;
    if ((openExternalLinks && !isFileScheme && !url.isRelative())
        || (url.isRelative() && !currentURL.isRelative() && !isFileScheme)) {
        QDesktopServices::openUrl(url);
        return;
    }
#endif

    emit q->anchorClicked(url);

    if (textOrSourceChanged)
        return;

    q->setSource(url);
}

void QTextBrowserPrivate::_q_highlightLink(const QString &anchor)
{
    if (anchor.isEmpty()) {
#ifndef QT_NO_CURSOR
        if (viewport->cursor().shape() != Qt::PointingHandCursor)
            oldCursor = viewport->cursor();
        viewport->setCursor(oldCursor);
#endif
        emitHighlighted(QUrl());
    } else {
#ifndef QT_NO_CURSOR
        viewport->setCursor(Qt::PointingHandCursor);
#endif

        const QUrl url = resolveUrl(anchor);
        emitHighlighted(url);
    }
}

void QTextBrowserPrivate::setSource(const QUrl &url, QTextDocument::ResourceType type)
{
    Q_Q(QTextBrowser);
#ifndef QT_NO_CURSOR
    if (q->isVisible())
        QGuiApplication::setOverrideCursor(Qt::WaitCursor);
#endif
    textOrSourceChanged = true;

    QString txt;

    bool doSetText = false;

    QUrl currentUrlWithoutFragment = currentURL;
    currentUrlWithoutFragment.setFragment(QString());
    QUrl newUrlWithoutFragment = currentURL.resolved(url);
    newUrlWithoutFragment.setFragment(QString());
    QString fileName = url.fileName();
    if (type == QTextDocument::UnknownResource) {
#if QT_CONFIG(textmarkdownreader)
        if (fileName.endsWith(".md"_L1) ||
                fileName.endsWith(".mkd"_L1) ||
                fileName.endsWith(".markdown"_L1))
            type = QTextDocument::MarkdownResource;
        else
#endif
            type = QTextDocument::HtmlResource;
    }
    currentType = type;

    if (url.isValid()
        && (newUrlWithoutFragment != currentUrlWithoutFragment || forceLoadOnSourceChange)) {
        QVariant data = q->loadResource(type, resolveUrl(url));
        if (data.userType() == QMetaType::QString) {
            txt = data.toString();
        } else if (data.userType() == QMetaType::QByteArray) {
            QByteArray ba = data.toByteArray();
            if (type == QTextDocument::HtmlResource) {
                auto decoder = QStringDecoder::decoderForHtml(ba);
                if (!decoder.isValid())
                    // fall back to utf8
                    decoder = QStringDecoder(QStringDecoder::Utf8);
                txt = decoder(ba);
            } else {
                txt = QString::fromUtf8(ba);
            }
        }
        if (Q_UNLIKELY(txt.isEmpty()))
            qWarning("QTextBrowser: No document for %s", url.toString().toLatin1().constData());

        if (q->isVisible()) {
            const QStringView firstTag = QStringView{txt}.left(txt.indexOf(u'>') + 1);
            if (firstTag.startsWith("<qt"_L1) && firstTag.contains("type"_L1) && firstTag.contains("detail"_L1)) {
#ifndef QT_NO_CURSOR
                QGuiApplication::restoreOverrideCursor();
#endif
#if QT_CONFIG(whatsthis)
                QWhatsThis::showText(QCursor::pos(), txt, q);
#endif
                return;
            }
        }

        currentURL = resolveUrl(url);
        doSetText = true;
    }

    if (!home.isValid())
        home = url;

    if (doSetText) {
        // Setting the base URL helps QTextDocument::resource() to find resources with relative paths.
        // But don't set it unless it contains the document's path, because QTextBrowserPrivate::resolveUrl()
        // can already deal with local files on the filesystem in case the base URL was not set.
        QUrl baseUrl = currentURL.adjusted(QUrl::RemoveFilename);
        if (!baseUrl.path().isEmpty())
            q->document()->setBaseUrl(baseUrl);
        q->document()->setMetaInformation(QTextDocument::DocumentUrl, currentURL.toString());
        qCDebug(lcBrowser) << "loading" << currentURL << "base" << q->document()->baseUrl() << "type" << type << txt.size() << "chars";
#if QT_CONFIG(textmarkdownreader)
        if (type == QTextDocument::MarkdownResource)
            q->QTextEdit::setMarkdown(txt);
        else
#endif
#ifndef QT_NO_TEXTHTMLPARSER
        q->QTextEdit::setHtml(txt);
#else
        q->QTextEdit::setPlainText(txt);
#endif

#ifdef QT_KEYPAD_NAVIGATION
        prevFocus.movePosition(QTextCursor::Start);
#endif
    }

    forceLoadOnSourceChange = false;

    if (!url.fragment().isEmpty()) {
        q->scrollToAnchor(url.fragment());
    } else {
        hbar->setValue(0);
        vbar->setValue(0);
    }
#ifdef QT_KEYPAD_NAVIGATION
    lastKeypadScrollValue = vbar->value();
    emitHighlighted(QUrl());
#endif

#ifndef QT_NO_CURSOR
    if (q->isVisible())
        QGuiApplication::restoreOverrideCursor();
#endif
    emit q->sourceChanged(url);
}

#ifdef QT_KEYPAD_NAVIGATION
void QTextBrowserPrivate::keypadMove(bool next)
{
    Q_Q(QTextBrowser);

    const int height = viewport->height();
    const int overlap = qBound(20, height / 5, 40); // XXX arbitrary, but a good balance
    const int visibleLinkAmount = overlap; // consistent, but maybe not the best choice (?)
    int yOffset = vbar->value();
    int scrollYOffset = qBound(0, next ? yOffset + height - overlap : yOffset - height + overlap, vbar->maximum());

    bool foundNextAnchor = false;
    bool focusIt = false;
    int focusedPos = -1;

    QTextCursor anchorToFocus;

    QRectF viewRect = QRectF(0, yOffset, control->size().width(), height);
    QRectF newViewRect = QRectF(0, scrollYOffset, control->size().width(), height);
    QRectF bothViewRects = viewRect.united(newViewRect);

    // If we don't have a previous anchor, pretend that we had the first/last character
    // on the screen selected.
    if (prevFocus.isNull()) {
        if (next)
            prevFocus = control->cursorForPosition(QPointF(0, yOffset));
        else
            prevFocus = control->cursorForPosition(QPointF(control->size().width(), yOffset + height));
    }

    // First, check to see if someone has moved the scroll bars independently
    if (lastKeypadScrollValue != yOffset) {
        // Someone (user or programmatically) has moved us, so we might
        // need to start looking from the current position instead of prevFocus

        bool findOnScreen = true;

        // If prevFocus is on screen at all, we just use it.
        if (prevFocus.hasSelection()) {
            QRectF prevRect = control->selectionRect(prevFocus);
            if (viewRect.intersects(prevRect))
                findOnScreen = false;
        }

        // Otherwise, we find a new anchor that's on screen.
        // Basically, create a cursor with the last/first character
        // on screen
        if (findOnScreen) {
            if (next)
                prevFocus = control->cursorForPosition(QPointF(0, yOffset));
            else
                prevFocus = control->cursorForPosition(QPointF(control->size().width(), yOffset + height));
        }
        foundNextAnchor = control->findNextPrevAnchor(prevFocus, next, anchorToFocus);
    } else if (prevFocus.hasSelection()) {
        // Check the pathological case that the current anchor is higher
        // than the screen, and just scroll through it in that case
        QRectF prevRect = control->selectionRect(prevFocus);
        if ((next && prevRect.bottom() > (yOffset + height)) ||
                (!next && prevRect.top() < yOffset)) {
            anchorToFocus = prevFocus;
            focusedPos = scrollYOffset;
            focusIt = true;
        } else {
            // This is the "normal" case - no scroll bar adjustments, no large anchors,
            // and no wrapping.
            foundNextAnchor = control->findNextPrevAnchor(prevFocus, next, anchorToFocus);
        }
    }

    // If not found yet, see if we need to wrap
    if (!focusIt && !foundNextAnchor) {
        if (next) {
            if (yOffset == vbar->maximum()) {
                prevFocus.movePosition(QTextCursor::Start);
                yOffset = scrollYOffset = 0;

                // Refresh the rectangles
                viewRect = QRectF(0, yOffset, control->size().width(), height);
                newViewRect = QRectF(0, scrollYOffset, control->size().width(), height);
                bothViewRects = viewRect.united(newViewRect);
            }
        } else {
            if (yOffset == 0) {
                prevFocus.movePosition(QTextCursor::End);
                yOffset = scrollYOffset = vbar->maximum();

                // Refresh the rectangles
                viewRect = QRectF(0, yOffset, control->size().width(), height);
                newViewRect = QRectF(0, scrollYOffset, control->size().width(), height);
                bothViewRects = viewRect.united(newViewRect);
            }
        }

        // Try looking now
        foundNextAnchor = control->findNextPrevAnchor(prevFocus, next, anchorToFocus);
    }

    // If we did actually find an anchor to use...
    if (foundNextAnchor) {
        QRectF desiredRect = control->selectionRect(anchorToFocus);

        // XXX This is an arbitrary heuristic
        // Decide to focus an anchor if it will be at least be
        // in the middle region of the screen after a scroll.
        // This can result in partial anchors with focus, but
        // insisting on links being completely visible before
        // selecting them causes disparities between links that
        // take up 90% of the screen height and those that take
        // up e.g. 110%
        // Obviously if a link is entirely visible, we still
        // focus it.
        if (bothViewRects.contains(desiredRect)
                || bothViewRects.adjusted(0, visibleLinkAmount, 0, -visibleLinkAmount).intersects(desiredRect)) {
            focusIt = true;

            // We aim to put the new link in the middle of the screen,
            // unless the link is larger than the screen (we just move to
            // display the first page of the link)
            if (desiredRect.height() > height) {
                if (next)
                    focusedPos = (int) desiredRect.top();
                else
                    focusedPos = (int) desiredRect.bottom() - height;
            } else
                focusedPos = (int) ((desiredRect.top() + desiredRect.bottom()) / 2 - (height / 2));

            // and clamp it to make sure we don't skip content.
            if (next)
                focusedPos = qBound(yOffset, focusedPos, scrollYOffset);
            else
                focusedPos = qBound(scrollYOffset, focusedPos, yOffset);
        }
    }

    // If we didn't get a new anchor, check if the old one is still on screen when we scroll
    // Note that big (larger than screen height) anchors also have some handling at the
    // start of this function.
    if (!focusIt && prevFocus.hasSelection()) {
        QRectF desiredRect = control->selectionRect(prevFocus);
        // XXX this may be better off also using the visibleLinkAmount value
        if (newViewRect.intersects(desiredRect)) {
            focusedPos = scrollYOffset;
            focusIt = true;
            anchorToFocus = prevFocus;
        }
    }

    // setTextCursor ensures that the cursor is visible. save & restore
    // the scroll bar values therefore
    const int savedXOffset = hbar->value();

    // Now actually process our decision
    if (focusIt && control->setFocusToAnchor(anchorToFocus)) {
        // Save the focus for next time
        prevFocus = control->textCursor();

        // Scroll
        vbar->setValue(focusedPos);
        lastKeypadScrollValue = focusedPos;
        hbar->setValue(savedXOffset);

        // Ensure that the new selection is highlighted.
        const QString href = control->anchorAtCursor();
        QUrl url = resolveUrl(href);
        emitHighlighted(url);
    } else {
        // Scroll
        vbar->setValue(scrollYOffset);
        lastKeypadScrollValue = scrollYOffset;

        // now make sure we don't have a focused anchor
        QTextCursor cursor = control->textCursor();
        cursor.clearSelection();

        control->setTextCursor(cursor);

        hbar->setValue(savedXOffset);
        vbar->setValue(scrollYOffset);

        emitHighlighted(QUrl());
    }
}
#endif

QTextBrowserPrivate::HistoryEntry QTextBrowserPrivate::createHistoryEntry() const
{
    HistoryEntry entry;
    entry.url = q_func()->source();
    entry.type = q_func()->sourceType();
    entry.title = q_func()->documentTitle();
    entry.hpos = hbar->value();
    entry.vpos = vbar->value();

    const QTextCursor cursor = control->textCursor();
    if (control->cursorIsFocusIndicator()
        && cursor.hasSelection()) {

        entry.focusIndicatorPosition = cursor.position();
        entry.focusIndicatorAnchor = cursor.anchor();
    }
    return entry;
}

void QTextBrowserPrivate::restoreHistoryEntry(const HistoryEntry &entry)
{
    setSource(entry.url, entry.type);
    hbar->setValue(entry.hpos);
    vbar->setValue(entry.vpos);
    if (entry.focusIndicatorAnchor != -1 && entry.focusIndicatorPosition != -1) {
        QTextCursor cursor(control->document());
        cursor.setPosition(entry.focusIndicatorAnchor);
        cursor.setPosition(entry.focusIndicatorPosition, QTextCursor::KeepAnchor);
        control->setTextCursor(cursor);
        control->setCursorIsFocusIndicator(true);
    }
#ifdef QT_KEYPAD_NAVIGATION
    lastKeypadScrollValue = vbar->value();
    prevFocus = control->textCursor();

    Q_Q(QTextBrowser);
    const QString href = prevFocus.charFormat().anchorHref();
    QUrl url = resolveUrl(href);
    emitHighlighted(url);
#endif
}

/*!
    \class QTextBrowser
    \brief The QTextBrowser class provides a rich text browser with hypertext navigation.

    \ingroup richtext-processing
    \inmodule QtWidgets

    This class extends QTextEdit (in read-only mode), adding some navigation
    functionality so that users can follow links in hypertext documents.

    If you want to provide your users with an editable rich text editor,
    use QTextEdit. If you want a text browser without hypertext navigation
    use QTextEdit, and use QTextEdit::setReadOnly() to disable
    editing. If you just need to display a small piece of rich text
    use QLabel.

    \section1 Document Source and Contents

    The contents of QTextEdit are set with setHtml() or setPlainText(),
    but QTextBrowser also implements the setSource() function, making it
    possible to use a named document as the source text. The name is looked
    up in a list of search paths and in the directory of the current document
    factory.

    If a document name ends with
    an anchor (for example, "\c #anchor"), the text browser automatically
    scrolls to that position (using scrollToAnchor()). When the user clicks
    on a hyperlink, the browser will call setSource() itself with the link's
    \c href value as argument. You can track the current source by connecting
    to the sourceChanged() signal.

    \section1 Navigation

    QTextBrowser provides backward() and forward() slots which you can
    use to implement Back and Forward buttons. The home() slot sets
    the text to the very first document displayed. The anchorClicked()
    signal is emitted when the user clicks an anchor. To override the
    default navigation behavior of the browser, call the setSource()
    function to supply new document text in a slot connected to this
    signal.

    If you want to load documents stored in the Qt resource system use
    \c{qrc} as the scheme in the URL to load. For example, for the document
    resource path \c{:/docs/index.html} use \c{qrc:/docs/index.html} as
    the URL with setSource().

    \sa QTextEdit, QTextDocument
*/

/*!
    \property QTextBrowser::modified
    \brief whether the contents of the text browser have been modified
*/

/*!
    \property QTextBrowser::readOnly
    \brief whether the text browser is read-only

    By default, this property is \c true.
*/

/*!
    \property QTextBrowser::undoRedoEnabled
    \brief whether the text browser supports undo/redo operations

    By default, this property is \c false.
*/

void QTextBrowserPrivate::init()
{
    Q_Q(QTextBrowser);
    control->setTextInteractionFlags(Qt::TextBrowserInteraction);
#ifndef QT_NO_CURSOR
    viewport->setCursor(oldCursor);
#endif
    q->setAttribute(Qt::WA_InputMethodEnabled, shouldEnableInputMethod(q));
    q->setUndoRedoEnabled(false);
    viewport->setMouseTracking(true);
    QObject::connect(q->document(), SIGNAL(contentsChanged()), q, SLOT(_q_documentModified()));
    QObject::connect(control, SIGNAL(linkActivated(QString)),
                     q, SLOT(_q_activateAnchor(QString)));
    QObject::connect(control, SIGNAL(linkHovered(QString)),
                     q, SLOT(_q_highlightLink(QString)));
}

/*!
    Constructs an empty QTextBrowser with parent \a parent.
*/
QTextBrowser::QTextBrowser(QWidget *parent)
    : QTextEdit(*new QTextBrowserPrivate, parent)
{
    Q_D(QTextBrowser);
    d->init();
}


/*!
    \internal
*/
QTextBrowser::~QTextBrowser()
{
}

/*!
    \property QTextBrowser::source
    \brief the name of the displayed document.

    This is a an invalid url if no document is displayed or if the
    source is unknown.

    When setting this property QTextBrowser tries to find a document
    with the specified name in the paths of the searchPaths property
    and directory of the current source, unless the value is an absolute
    file path. It also checks for optional anchors and scrolls the document
    accordingly

    If the first tag in the document is \c{<qt type=detail>}, the
    document is displayed as a popup rather than as new document in
    the browser window itself. Otherwise, the document is displayed
    normally in the text browser with the text set to the contents of
    the named document with \l QTextDocument::setHtml() or
    \l QTextDocument::setMarkdown(), depending on whether the filename ends
    with any of the known Markdown file extensions.

    If you would like to avoid automatic type detection
    and specify the type explicitly, call setSource() rather than
    setting this property.

    By default, this property contains an empty URL.
*/
QUrl QTextBrowser::source() const
{
    Q_D(const QTextBrowser);
    if (d->stack.isEmpty())
        return QUrl();
    else
        return d->stack.top().url;
}

/*!
    \property QTextBrowser::sourceType
    \brief the type of the displayed document

    This is QTextDocument::UnknownResource if no document is displayed or if
    the type of the source is unknown. Otherwise it holds the type that was
    detected, or the type that was specified when setSource() was called.
*/
QTextDocument::ResourceType QTextBrowser::sourceType() const
{
    Q_D(const QTextBrowser);
    if (d->stack.isEmpty())
        return QTextDocument::UnknownResource;
    else
        return d->stack.top().type;
}

/*!
    \property QTextBrowser::searchPaths
    \brief the search paths used by the text browser to find supporting
    content

    QTextBrowser uses this list to locate images and documents.

    By default, this property contains an empty string list.
*/

QStringList QTextBrowser::searchPaths() const
{
    Q_D(const QTextBrowser);
    return d->searchPaths;
}

void QTextBrowser::setSearchPaths(const QStringList &paths)
{
    Q_D(QTextBrowser);
    d->searchPaths = paths;
}

/*!
    Reloads the current set source.
*/
void QTextBrowser::reload()
{
    Q_D(QTextBrowser);
    QUrl s = d->currentURL;
    d->currentURL = QUrl();
    setSource(s, d->currentType);
}

/*!
    Attempts to load the document at the given \a url with the specified \a type.

    If \a type is \l {QTextDocument::UnknownResource}{UnknownResource}
    (the default), the document type will be detected: that is, if the url ends
    with an extension of \c{.md}, \c{.mkd} or \c{.markdown}, the document will be
    loaded via \l QTextDocument::setMarkdown(); otherwise it will be loaded via
    \l QTextDocument::setHtml(). This detection can be bypassed by specifying
    the \a type explicitly.
*/
void QTextBrowser::setSource(const QUrl &url, QTextDocument::ResourceType type)
{
    doSetSource(url, type);
}

/*!
    Attempts to load the document at the given \a url with the specified \a type.

    setSource() calls doSetSource.  In Qt 5, setSource(const QUrl &url) was virtual.
    In Qt 6, doSetSource() is virtual instead, so that it can be overridden in subclasses.
*/
void QTextBrowser::doSetSource(const QUrl &url, QTextDocument::ResourceType type)
{
    Q_D(QTextBrowser);

    const QTextBrowserPrivate::HistoryEntry historyEntry = d->createHistoryEntry();

    d->setSource(url, type);

    if (!url.isValid())
        return;

    // the same url you are already watching?
    if (!d->stack.isEmpty() && d->stack.top().url == url)
        return;

    if (!d->stack.isEmpty())
        d->stack.top() = historyEntry;

    QTextBrowserPrivate::HistoryEntry entry;
    entry.url = url;
    entry.type = d->currentType;
    entry.title = documentTitle();
    entry.hpos = 0;
    entry.vpos = 0;
    d->stack.push(entry);

    emit backwardAvailable(d->stack.size() > 1);

    if (!d->forwardStack.isEmpty() && d->forwardStack.top().url == url) {
        d->forwardStack.pop();
        emit forwardAvailable(d->forwardStack.size() > 0);
    } else {
        d->forwardStack.clear();
        emit forwardAvailable(false);
    }

    emit historyChanged();
}

/*!
    \fn void QTextBrowser::backwardAvailable(bool available)

    This signal is emitted when the availability of backward()
    changes. \a available is false when the user is at home();
    otherwise it is true.
*/

/*!
    \fn void QTextBrowser::forwardAvailable(bool available)

    This signal is emitted when the availability of forward() changes.
    \a available is true after the user navigates backward() and false
    when the user navigates or goes forward().
*/

/*!
    \fn void QTextBrowser::historyChanged()
    \since 4.4

    This signal is emitted when the history changes.

    \sa historyTitle(), historyUrl()
*/

/*!
    \fn void QTextBrowser::sourceChanged(const QUrl &src)

    This signal is emitted when the source has changed, \a src
    being the new source.

    Source changes happen both programmatically when calling
    setSource(), forward(), backward() or home() or when the user
    clicks on links or presses the equivalent key sequences.
*/

/*!  \fn void QTextBrowser::highlighted(const QUrl &link)

    This signal is emitted when the user has selected but not
    activated an anchor in the document. The URL referred to by the
    anchor is passed in \a link.
*/

/*!
    \fn void QTextBrowser::anchorClicked(const QUrl &link)

    This signal is emitted when the user clicks an anchor. The
    URL referred to by the anchor is passed in \a link.

    Note that the browser will automatically handle navigation to the
    location specified by \a link unless the openLinks property
    is set to false or you call setSource() in a slot connected.
    This mechanism is used to override the default navigation features of the browser.
*/

/*!
    Changes the document displayed to the previous document in the
    list of documents built by navigating links. Does nothing if there
    is no previous document.

    \sa forward(), backwardAvailable()
*/
void QTextBrowser::backward()
{
    Q_D(QTextBrowser);
    if (d->stack.size() <= 1)
        return;

    // Update the history entry
    d->forwardStack.push(d->createHistoryEntry());
    d->stack.pop(); // throw away the old version of the current entry
    d->restoreHistoryEntry(d->stack.top()); // previous entry
    emit backwardAvailable(d->stack.size() > 1);
    emit forwardAvailable(true);
    emit historyChanged();
}

/*!
    Changes the document displayed to the next document in the list of
    documents built by navigating links. Does nothing if there is no
    next document.

    \sa backward(), forwardAvailable()
*/
void QTextBrowser::forward()
{
    Q_D(QTextBrowser);
    if (d->forwardStack.isEmpty())
        return;
    if (!d->stack.isEmpty()) {
        // Update the history entry
        d->stack.top() = d->createHistoryEntry();
    }
    d->stack.push(d->forwardStack.pop());
    d->restoreHistoryEntry(d->stack.top());
    emit backwardAvailable(true);
    emit forwardAvailable(!d->forwardStack.isEmpty());
    emit historyChanged();
}

/*!
    Changes the document displayed to be the first document from
    the history.
*/
void QTextBrowser::home()
{
    Q_D(QTextBrowser);
    if (d->home.isValid())
        setSource(d->home);
}

/*!
    The event \a ev is used to provide the following keyboard shortcuts:
    \table
    \header \li Keypress            \li Action
    \row \li Alt+Left Arrow  \li \l backward()
    \row \li Alt+Right Arrow \li \l forward()
    \row \li Alt+Up Arrow    \li \l home()
    \endtable
*/
void QTextBrowser::keyPressEvent(QKeyEvent *ev)
{
#ifdef QT_KEYPAD_NAVIGATION
    Q_D(QTextBrowser);
    switch (ev->key()) {
    case Qt::Key_Select:
        if (QApplicationPrivate::keypadNavigationEnabled()) {
            if (!hasEditFocus()) {
                setEditFocus(true);
                return;
            } else {
                QTextCursor cursor = d->control->textCursor();
                QTextCharFormat charFmt = cursor.charFormat();
                if (!cursor.hasSelection() || charFmt.anchorHref().isEmpty()) {
                    ev->accept();
                    return;
                }
            }
        }
        break;
    case Qt::Key_Back:
        if (QApplicationPrivate::keypadNavigationEnabled()) {
            if (hasEditFocus()) {
                setEditFocus(false);
                ev->accept();
                return;
            }
        }
        QTextEdit::keyPressEvent(ev);
        return;
    default:
        if (QApplicationPrivate::keypadNavigationEnabled() && !hasEditFocus()) {
            ev->ignore();
            return;
        }
    }
#endif

    if (ev->modifiers() & Qt::AltModifier) {
        switch (ev->key()) {
        case Qt::Key_Right:
            forward();
            ev->accept();
            return;
        case Qt::Key_Left:
            backward();
            ev->accept();
            return;
        case Qt::Key_Up:
            home();
            ev->accept();
            return;
        }
    }
#ifdef QT_KEYPAD_NAVIGATION
    else {
        if (ev->key() == Qt::Key_Up) {
            d->keypadMove(false);
            return;
        } else if (ev->key() == Qt::Key_Down) {
            d->keypadMove(true);
            return;
        }
    }
#endif
    QTextEdit::keyPressEvent(ev);
}

/*!
    \reimp
*/
void QTextBrowser::mouseMoveEvent(QMouseEvent *e)
{
    QTextEdit::mouseMoveEvent(e);
}

/*!
    \reimp
*/
void QTextBrowser::mousePressEvent(QMouseEvent *e)
{
    QTextEdit::mousePressEvent(e);
}

/*!
    \reimp
*/
void QTextBrowser::mouseReleaseEvent(QMouseEvent *e)
{
    QTextEdit::mouseReleaseEvent(e);
}

/*!
    \reimp
*/
void QTextBrowser::focusOutEvent(QFocusEvent *ev)
{
#ifndef QT_NO_CURSOR
    Q_D(QTextBrowser);
    d->viewport->setCursor((!(d->control->textInteractionFlags() & Qt::TextEditable)) ? d->oldCursor : Qt::IBeamCursor);
#endif
    QTextEdit::focusOutEvent(ev);
}

/*!
    \reimp
*/
bool QTextBrowser::focusNextPrevChild(bool next)
{
    Q_D(QTextBrowser);
    if (d->control->setFocusToNextOrPreviousAnchor(next)) {
#ifdef QT_KEYPAD_NAVIGATION
        // Might need to synthesize a highlight event.
        if (d->prevFocus != d->control->textCursor() && d->control->textCursor().hasSelection()) {
            const QString href = d->control->anchorAtCursor();
            QUrl url = d->resolveUrl(href);
            emitHighlighted(url);
        }
        d->prevFocus = d->control->textCursor();
#endif
        return true;
    } else {
#ifdef QT_KEYPAD_NAVIGATION
        // We assume we have no highlight now.
        emitHighlighted(QUrl());
#endif
    }
    return QTextEdit::focusNextPrevChild(next);
}

/*!
  \reimp
*/
void QTextBrowser::paintEvent(QPaintEvent *e)
{
    Q_D(QTextBrowser);
    QPainter p(d->viewport);
    d->paint(&p, e);
}

/*!
    This function is called when the document is loaded and for
    each image in the document. The \a type indicates the type of resource
    to be loaded. An invalid QVariant is returned if the resource cannot be
    loaded.

    The default implementation ignores \a type and tries to locate
    the resources by interpreting \a name as a file name. If it is
    not an absolute path it tries to find the file in the paths of
    the \l searchPaths property and in the same directory as the
    current source. On success, the result is a QVariant that stores
    a QByteArray with the contents of the file.

    If you reimplement this function, you can return other QVariant
    types. The table below shows which variant types are supported
    depending on the resource type:

    \table
    \header \li ResourceType  \li QMetaType::Type
    \row    \li QTextDocument::HtmlResource  \li QString or QByteArray
    \row    \li QTextDocument::ImageResource \li QImage, QPixmap or QByteArray
    \row    \li QTextDocument::StyleSheetResource \li QString or QByteArray
    \row    \li QTextDocument::MarkdownResource \li QString or QByteArray
    \endtable
*/
QVariant QTextBrowser::loadResource(int /*type*/, const QUrl &name)
{
    Q_D(QTextBrowser);

    QByteArray data;
    QString fileName = d->findFile(d->resolveUrl(name));
    if (fileName.isEmpty())
        return QVariant();
    QFile f(fileName);
    if (f.open(QFile::ReadOnly)) {
        data = f.readAll();
        f.close();
    } else {
        return QVariant();
    }

    return data;
}

/*!
    \since 4.2

    Returns \c true if the text browser can go backward in the document history
    using backward().

    \sa backwardAvailable(), backward()
*/
bool QTextBrowser::isBackwardAvailable() const
{
    Q_D(const QTextBrowser);
    return d->stack.size() > 1;
}

/*!
    \since 4.2

    Returns \c true if the text browser can go forward in the document history
    using forward().

    \sa forwardAvailable(), forward()
*/
bool QTextBrowser::isForwardAvailable() const
{
    Q_D(const QTextBrowser);
    return !d->forwardStack.isEmpty();
}

/*!
    \since 4.2

    Clears the history of visited documents and disables the forward and
    backward navigation.

    \sa backward(), forward()
*/
void QTextBrowser::clearHistory()
{
    Q_D(QTextBrowser);
    d->forwardStack.clear();
    if (!d->stack.isEmpty()) {
        QTextBrowserPrivate::HistoryEntry historyEntry = d->stack.top();
        d->stack.clear();
        d->stack.push(historyEntry);
        d->home = historyEntry.url;
    }
    emit forwardAvailable(false);
    emit backwardAvailable(false);
    emit historyChanged();
}

/*!
   Returns the url of the HistoryItem.

    \table
    \header \li Input            \li Return
    \row \li \a{i} < 0  \li \l backward() history
    \row \li\a{i} == 0 \li current, see QTextBrowser::source()
    \row \li \a{i} > 0  \li \l forward() history
    \endtable

    \since 4.4
*/
QUrl QTextBrowser::historyUrl(int i) const
{
    Q_D(const QTextBrowser);
    return d->history(i).url;
}

/*!
    Returns the documentTitle() of the HistoryItem.

    \table
    \header \li Input            \li Return
    \row \li \a{i} < 0  \li \l backward() history
    \row \li \a{i} == 0 \li current, see QTextBrowser::source()
    \row \li \a{i} > 0  \li \l forward() history
    \endtable

    \snippet code/src_gui_widgets_qtextbrowser.cpp 0

    \since 4.4
*/
QString QTextBrowser::historyTitle(int i) const
{
    Q_D(const QTextBrowser);
    return d->history(i).title;
}


/*!
    Returns the number of locations forward in the history.

    \since 4.4
*/
int QTextBrowser::forwardHistoryCount() const
{
    Q_D(const QTextBrowser);
    return d->forwardStack.size();
}

/*!
    Returns the number of locations backward in the history.

    \since 4.4
*/
int QTextBrowser::backwardHistoryCount() const
{
    Q_D(const QTextBrowser);
    return d->stack.size()-1;
}

/*!
    \property QTextBrowser::openExternalLinks
    \since 4.2

    Specifies whether QTextBrowser should automatically open links to external
    sources using QDesktopServices::openUrl() instead of emitting the
    anchorClicked signal. Links are considered external if their scheme is
    neither file or qrc.

    The default value is false.
*/
bool QTextBrowser::openExternalLinks() const
{
    Q_D(const QTextBrowser);
    return d->openExternalLinks;
}

void QTextBrowser::setOpenExternalLinks(bool open)
{
    Q_D(QTextBrowser);
    d->openExternalLinks = open;
}

/*!
   \property QTextBrowser::openLinks
   \since 4.3

   This property specifies whether QTextBrowser should automatically open links the user tries to
   activate by mouse or keyboard.

   Regardless of the value of this property the anchorClicked signal is always emitted.

   The default value is true.
*/

bool QTextBrowser::openLinks() const
{
    Q_D(const QTextBrowser);
    return d->openLinks;
}

void QTextBrowser::setOpenLinks(bool open)
{
    Q_D(QTextBrowser);
    d->openLinks = open;
}

/*! \reimp */
bool QTextBrowser::event(QEvent *e)
{
    return QTextEdit::event(e);
}

QT_END_NAMESPACE

#include "moc_qtextbrowser.cpp"
