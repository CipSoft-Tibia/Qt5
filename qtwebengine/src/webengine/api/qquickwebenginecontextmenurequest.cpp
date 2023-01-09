/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickwebenginecontextmenurequest_p.h"
#include "web_contents_adapter_client.h"

QT_BEGIN_NAMESPACE

// Match MediaType enum
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaTypeNone,
                   QQuickWebEngineContextMenuRequest::MediaTypeNone)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaTypeImage,
                   QQuickWebEngineContextMenuRequest::MediaTypeImage)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaTypeAudio,
                   QQuickWebEngineContextMenuRequest::MediaTypeAudio)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaTypeVideo,
                   QQuickWebEngineContextMenuRequest::MediaTypeVideo)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaTypeCanvas,
                   QQuickWebEngineContextMenuRequest::MediaTypeCanvas)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaTypeFile,
                   QQuickWebEngineContextMenuRequest::MediaTypeFile)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaTypePlugin,
                   QQuickWebEngineContextMenuRequest::MediaTypePlugin)

// Match MediaFlag enum
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaInError,
                   QQuickWebEngineContextMenuRequest::MediaInError)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaPaused,
                   QQuickWebEngineContextMenuRequest::MediaPaused)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaMuted,
                   QQuickWebEngineContextMenuRequest::MediaMuted)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaLoop,
                   QQuickWebEngineContextMenuRequest::MediaLoop)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaCanSave,
                   QQuickWebEngineContextMenuRequest::MediaCanSave)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaHasAudio,
                   QQuickWebEngineContextMenuRequest::MediaHasAudio)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaCanToggleControls,
                   QQuickWebEngineContextMenuRequest::MediaCanToggleControls)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaControls,
                   QQuickWebEngineContextMenuRequest::MediaControls)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaCanPrint,
                   QQuickWebEngineContextMenuRequest::MediaCanPrint)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::MediaCanRotate,
                   QQuickWebEngineContextMenuRequest::MediaCanRotate)

// Match EditFlag enum
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::CanUndo,
                   QQuickWebEngineContextMenuRequest::CanUndo)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::CanRedo,
                   QQuickWebEngineContextMenuRequest::CanRedo)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::CanCut,
                   QQuickWebEngineContextMenuRequest::CanCut)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::CanCopy,
                   QQuickWebEngineContextMenuRequest::CanCopy)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::CanPaste,
                   QQuickWebEngineContextMenuRequest::CanPaste)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::CanDelete,
                   QQuickWebEngineContextMenuRequest::CanDelete)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::CanSelectAll,
                   QQuickWebEngineContextMenuRequest::CanSelectAll)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::CanTranslate,
                   QQuickWebEngineContextMenuRequest::CanTranslate)
ASSERT_ENUMS_MATCH(QtWebEngineCore::WebEngineContextMenuData::CanEditRichly,
                   QQuickWebEngineContextMenuRequest::CanEditRichly)

/*!
    \qmltype ContextMenuRequest
    \instantiates QQuickWebEngineContextMenuRequest
    \inqmlmodule QtWebEngine
    \since QtWebEngine 1.4

    \brief A request for showing a context menu.

    A ContextMenuRequest is passed as an argument of the
    WebEngineView::contextMenuRequested signal. It provides further
    information about the context of the request. The position of the
    request origin can be found via the \l x and \l y properties.

    The \l accepted property of the request indicates whether the request
    is handled by the user code or the default context menu should
    be displayed.

    The following code uses a custom menu to handle the request:

    \code
    WebEngineView {
        id: view
        // ...
        onContextMenuRequested: function(request) {
            request.accepted = true;
            myMenu.x = request.x;
            myMenu.y = request.y;
            myMenu.trigger.connect(view.triggerWebAction);
            myMenu.popup();
        }
        // ...
    }
    \endcode
*/

QQuickWebEngineContextMenuRequest::QQuickWebEngineContextMenuRequest(
        const QtWebEngineCore::WebEngineContextMenuData &data, QObject *parent):
    QObject(parent)
  , m_data(new QtWebEngineCore::WebEngineContextMenuData(data))
  , m_accepted(false)
{
}

QQuickWebEngineContextMenuRequest::~QQuickWebEngineContextMenuRequest()
{
}

/*!
    \qmlproperty int ContextMenuRequest::x
    \readonly

    The x coordinate of the user action from where the context
    menu request originates.
*/

int QQuickWebEngineContextMenuRequest::x() const
{
    return m_data->position().x();
}

/*!
    \qmlproperty int ContextMenuRequest::y
    \readonly

    The y coordinate of the user action from where the context
    menu request originates.
*/

int QQuickWebEngineContextMenuRequest::y() const
{
    return m_data->position().y();
}

/*!
    \qmlproperty string ContextMenuRequest::selectedText
    \readonly

    The selected text the context menu was created for.
*/

QString QQuickWebEngineContextMenuRequest::selectedText() const
{
    return m_data->selectedText();
}

/*!
    \qmlproperty string ContextMenuRequest::linkText
    \readonly

    The text of the link if the context menu was requested for a link.
*/

QString QQuickWebEngineContextMenuRequest::linkText() const
{
    return m_data->linkText();
}

/*!
    \qmlproperty url ContextMenuRequest::linkUrl
    \readonly

    The URL of the link if the selected web page content is a link.
    It is not guaranteed to be a valid URL.
*/

QUrl QQuickWebEngineContextMenuRequest::linkUrl() const
{
    return m_data->unfilteredLinkUrl();
}

/*!
    \qmlproperty url ContextMenuRequest::mediaUrl
    \readonly

    The URL of media if the selected web content is a media element.
*/

QUrl QQuickWebEngineContextMenuRequest::mediaUrl() const
{
    return m_data->mediaUrl();
}

/*!
    \qmlproperty enumeration ContextMenuRequest::mediaType
    \readonly

    The type of the media element or \c MediaTypeNone if
    the selected web page content is not a media element.

    \value  ContextMenuRequest.MediaTypeNone
            Not a media.
    \value  ContextMenuRequest.MediaTypeImage
            An image.
    \value  ContextMenuRequest.MediaTypeVideo
            A video.
    \value  ContextMenuRequest.MediaTypeAudio
            An audio element.
    \value  ContextMenuRequest.MediaTypeCanvas
            A canvas.
    \value  ContextMenuRequest.MediaTypeFile
            A file.
    \value  ContextMenuRequest.MediaTypePlugin
            A plugin.
*/

QQuickWebEngineContextMenuRequest::MediaType QQuickWebEngineContextMenuRequest::mediaType() const
{
    return static_cast<QQuickWebEngineContextMenuRequest::MediaType>(m_data->mediaType());
}

/*!
    \qmlproperty bool ContextMenuRequest::isContentEditable
    \readonly

    Indicates whether the selected web content is editable.
*/

bool QQuickWebEngineContextMenuRequest::isContentEditable() const
{
    return m_data->isEditable();
}

/*!
    \qmlproperty string ContextMenuRequest::misspelledWord
    \readonly

    If the context is a word considered misspelled by the spell-checker,
    returns the misspelled word.
*/

QString QQuickWebEngineContextMenuRequest::misspelledWord() const
{
    return m_data->misspelledWord();
}

/*!
    \qmlproperty stringlist ContextMenuRequest::spellCheckerSuggestions
    \readonly

    If the context is a word considered misspelled by the spell-checker,
    returns a list of suggested replacements.
*/

QStringList QQuickWebEngineContextMenuRequest::spellCheckerSuggestions() const
{
    return m_data->spellCheckerSuggestions();
}

/*!
    \qmlproperty bool ContextMenuRequest::accepted

    Indicates whether the context menu request has been
    handled by the signal handler.

    If the property is \c false after any signal handlers
    for WebEngineView::contextMenuRequested have been executed,
    a default context menu will be shown.
    To prevent this, set \c{request.accepted} to \c true.

    The default is \c false.

    \note The default content of the context menu depends on the
    web element for which the request was actually generated.
*/

bool QQuickWebEngineContextMenuRequest::isAccepted() const
{
    return m_accepted;
}

void QQuickWebEngineContextMenuRequest::setAccepted(bool accepted)
{
    m_accepted = accepted;
}

/*!
    \qmlproperty flags ContextMenuRequest::mediaFlags
    \readonly
    \since QtWebEngine 1.7

    The current media element's status and its available operations.
    \c MediaNone if the selected web page content is not a media element.

    \value  ContextMenuRequest.MediaInError
            An error occurred.
    \value  ContextMenuRequest.MediaPaused
            Media is paused.
    \value  ContextMenuRequest.MediaMuted
            Media is muted.
    \value  ContextMenuRequest.MediaLoop
            Media can be looped.
    \value  ContextMenuRequest.MediaCanSave
            Media can be saved.
    \value  ContextMenuRequest.MediaHasAudio
            Media has audio.
    \value  ContextMenuRequest.MediaCanToggleControls
            Media can show controls.
    \value  ContextMenuRequest.MediaControls
            Media controls are shown.
    \value  ContextMenuRequest.MediaCanPrint
            Media is printable.
    \value  ContextMenuRequest.MediaCanRotate
            Media is rotatable.
*/
QQuickWebEngineContextMenuRequest::MediaFlags QQuickWebEngineContextMenuRequest::mediaFlags() const
{
    return static_cast<QQuickWebEngineContextMenuRequest::MediaFlags>(m_data->mediaFlags());
}

/*!
    \qmlproperty flags ContextMenuRequest::editFlags
    \readonly
    \since QtWebEngine 1.7

    The available edit operations in the current context or \c CanDoNone if no actions are available.

    \value  ContextMenuRequest.CanUndo
            Undo is available.
    \value  ContextMenuRequest.CanRedo
            Redo is available.
    \value  ContextMenuRequest.CanCut
            Cut is available.
    \value  ContextMenuRequest.CanCopy
            Copy is available.
    \value  ContextMenuRequest.CanPaste
            Paste is available.
    \value  ContextMenuRequest.CanDelete
            Delete is available.
    \value  ContextMenuRequest.CanSelectAll
            Select All is available.
    \value  ContextMenuRequest.CanTranslate
            Translate is available.
    \value  ContextMenuRequest.CanEditRichly
            Context is richly editable.
*/
QQuickWebEngineContextMenuRequest::EditFlags QQuickWebEngineContextMenuRequest::editFlags() const
{
    return static_cast<QQuickWebEngineContextMenuRequest::EditFlags>(m_data->editFlags());
}

QT_END_NAMESPACE
