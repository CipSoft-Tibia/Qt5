// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qplatformdefs.h>
#include "qitemeditorfactory.h"
#include "qitemeditorfactory_p.h"

#if QT_CONFIG(combobox)
#include <qcombobox.h>
#endif
#if QT_CONFIG(datetimeedit)
#include <qdatetimeedit.h>
#endif
#if QT_CONFIG(label)
#include <qlabel.h>
#endif
#if QT_CONFIG(lineedit)
#include <qlineedit.h>
#endif
#if QT_CONFIG(spinbox)
#include <qspinbox.h>
#endif
#include <qstyle.h>
#include <qstyleoption.h>
#include <limits.h>
#include <float.h>
#include <qapplication.h>
#include <qdebug.h>

#include <vector>
#include <algorithm>
QT_BEGIN_NAMESPACE


#if QT_CONFIG(combobox)

class QBooleanComboBox : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(bool value READ value WRITE setValue USER true)

public:
    QBooleanComboBox(QWidget *parent);
    void setValue(bool);
    bool value() const;
};

#endif // QT_CONFIG(combobox)


#if QT_CONFIG(spinbox)

class QUIntSpinBox : public QSpinBox
{
    Q_OBJECT
    Q_PROPERTY(uint value READ uintValue WRITE setUIntValue NOTIFY uintValueChanged USER true)
public:
    explicit QUIntSpinBox(QWidget *parent = nullptr)
      : QSpinBox(parent)
    {
        connect(this, SIGNAL(valueChanged(int)), SIGNAL(uintValueChanged()));
    }

    uint uintValue()
    {
        return value();
    }

    void setUIntValue(uint value_)
    {
        return setValue(value_);
    }

Q_SIGNALS:
    void uintValueChanged();
};

#endif // QT_CONFIG(spinbox)

/*!
    \class QItemEditorFactory
    \brief The QItemEditorFactory class provides widgets for editing item data
    in views and delegates.
    \since 4.2
    \ingroup model-view
    \inmodule QtWidgets

    When editing data in an item view, editors are created and
    displayed by a delegate. QStyledItemDelegate, which is the delegate by
    default installed on Qt's item views, uses a QItemEditorFactory to
    create editors for it. A default unique instance provided by
    QItemEditorFactory is used by all item delegates.  If you set a
    new default factory with setDefaultFactory(), the new factory will
    be used by existing and new delegates.

    A factory keeps a collection of QItemEditorCreatorBase
    instances, which are specialized editors that produce editors
    for one particular QVariant data type (All Qt models store
    their data in \l{QVariant}s).

    \section1 Standard Editing Widgets

    The standard factory implementation provides editors for a variety of data
    types. These are created whenever a delegate needs to provide an editor for
    data supplied by a model. The following table shows the relationship between
    types and the standard editors provided.

    \table
    \header \li Type \li Editor Widget
    \row    \li bool \li QComboBox
    \row    \li double \li QDoubleSpinBox
    \row    \li int \li{1,2} QSpinBox
    \row    \li unsigned int
    \row    \li QDate \li QDateEdit
    \row    \li QDateTime \li QDateTimeEdit
    \row    \li QPixmap \li QLabel
    \row    \li QString \li QLineEdit
    \row    \li QTime \li QTimeEdit
    \endtable

    Additional editors can be registered with the registerEditor() function.

    \sa QStyledItemDelegate, {Model/View Programming}, {Color Editor Factory Example}
*/

/*!
    \fn QItemEditorFactory::QItemEditorFactory()

    Constructs a new item editor factory.
*/

/*!
    Creates an editor widget with the given \a parent for the specified \a userType of data,
    and returns it as a QWidget.

    \sa registerEditor()
*/
QWidget *QItemEditorFactory::createEditor(int userType, QWidget *parent) const
{
    QItemEditorCreatorBase *creator = creatorMap.value(userType, 0);
    if (!creator) {
        const QItemEditorFactory *dfactory = defaultFactory();
        return dfactory == this ? nullptr : dfactory->createEditor(userType, parent);
    }
    return creator->createWidget(parent);
}

/*!
    Returns the property name used to access data for the given \a userType of data.
*/
QByteArray QItemEditorFactory::valuePropertyName(int userType) const
{
    QItemEditorCreatorBase *creator = creatorMap.value(userType, 0);
    if (!creator) {
        const QItemEditorFactory *dfactory = defaultFactory();
        return dfactory == this ? QByteArray() : dfactory->valuePropertyName(userType);
    }
    return creator->valuePropertyName();
}

/*!
    Destroys the item editor factory.
*/
QItemEditorFactory::~QItemEditorFactory()
{
    //we make sure we delete all the QItemEditorCreatorBase
    //this has to be done only once, hence the sort-unique idiom
    std::vector<QItemEditorCreatorBase*> creators(creatorMap.cbegin(), creatorMap.cend());
    std::sort(creators.begin(), creators.end());
    const auto it = std::unique(creators.begin(), creators.end());
    qDeleteAll(creators.begin(), it);
}

/*!
    Registers an item editor creator specified by \a creator for the given \a userType of data.

    \b{Note:} The factory takes ownership of the item editor creator and will destroy
    it if a new creator for the same type is registered later.

    \sa createEditor()
*/
void QItemEditorFactory::registerEditor(int userType, QItemEditorCreatorBase *creator)
{
    const auto it = creatorMap.constFind(userType);
    if (it != creatorMap.cend()) {
        QItemEditorCreatorBase *oldCreator = it.value();
        Q_ASSERT(oldCreator);
        creatorMap.erase(it);
        if (std::find(creatorMap.cbegin(), creatorMap.cend(), oldCreator) == creatorMap.cend())
            delete oldCreator; // if it is no more in use we can delete it
    }

    creatorMap[userType] = creator;
}

class QDefaultItemEditorFactory : public QItemEditorFactory
{
public:
    inline QDefaultItemEditorFactory() {}
    QWidget *createEditor(int userType, QWidget *parent) const override;
    QByteArray valuePropertyName(int) const override;
};

QWidget *QDefaultItemEditorFactory::createEditor(int userType, QWidget *parent) const
{
    switch (userType) {
#if QT_CONFIG(combobox)
    case QMetaType::Bool: {
        QBooleanComboBox *cb = new QBooleanComboBox(parent);
        cb->setFrame(false);
        cb->setSizePolicy(QSizePolicy::Ignored, cb->sizePolicy().verticalPolicy());
        return cb; }
#endif
#if QT_CONFIG(spinbox)
    case QMetaType::UInt: {
        QSpinBox *sb = new QUIntSpinBox(parent);
        sb->setFrame(false);
        sb->setMinimum(0);
        sb->setMaximum(INT_MAX);
        sb->setSizePolicy(QSizePolicy::Ignored, sb->sizePolicy().verticalPolicy());
        return sb; }
    case QMetaType::Int: {
        QSpinBox *sb = new QSpinBox(parent);
        sb->setFrame(false);
        sb->setMinimum(INT_MIN);
        sb->setMaximum(INT_MAX);
        sb->setSizePolicy(QSizePolicy::Ignored, sb->sizePolicy().verticalPolicy());
        return sb; }
#endif
#if QT_CONFIG(datetimeedit)
    case QMetaType::QDate: {
        QDateTimeEdit *ed = new QDateEdit(parent);
        ed->setFrame(false);
        return ed; }
    case QMetaType::QTime: {
        QDateTimeEdit *ed = new QTimeEdit(parent);
        ed->setFrame(false);
        return ed; }
    case QMetaType::QDateTime: {
        QDateTimeEdit *ed = new QDateTimeEdit(parent);
        ed->setFrame(false);
        return ed; }
#endif
#if QT_CONFIG(label)
    case QMetaType::QPixmap:
        return new QLabel(parent);
#endif
#if QT_CONFIG(spinbox)
    case QMetaType::Double: {
        QDoubleSpinBox *sb = new QDoubleSpinBox(parent);
        sb->setFrame(false);
        sb->setMinimum(-DBL_MAX);
        sb->setMaximum(DBL_MAX);
        sb->setSizePolicy(QSizePolicy::Ignored, sb->sizePolicy().verticalPolicy());
        return sb; }
#endif
#if QT_CONFIG(lineedit)
    case QMetaType::QString:
    default: {
        // the default editor is a lineedit
        QExpandingLineEdit *le = new QExpandingLineEdit(parent);
        le->setFrame(le->style()->styleHint(QStyle::SH_ItemView_DrawDelegateFrame, nullptr, le));
        if (!le->style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, nullptr, le))
            le->setWidgetOwnsGeometry(true);
        return le; }
#else
    default:
        break;
#endif
    }
    return nullptr;
}

QByteArray QDefaultItemEditorFactory::valuePropertyName(int userType) const
{
    switch (userType) {
#if QT_CONFIG(combobox)
    case QMetaType::Bool:
        return "currentIndex";
#endif
#if QT_CONFIG(spinbox)
    case QMetaType::UInt:
    case QMetaType::Int:
    case QMetaType::Double:
        return "value";
#endif
#if QT_CONFIG(datetimeedit)
    case QMetaType::QDate:
        return "date";
    case QMetaType::QTime:
        return "time";
    case QMetaType::QDateTime:
        return "dateTime";
#endif
    case QMetaType::QString:
    default:
        // the default editor is a lineedit
        return "text";
    }
}

static QItemEditorFactory *q_default_factory = nullptr;
struct QDefaultFactoryCleaner
{
    inline QDefaultFactoryCleaner() {}
    ~QDefaultFactoryCleaner() { delete q_default_factory; q_default_factory = nullptr; }
};

/*!
    Returns the default item editor factory.

    \sa setDefaultFactory()
*/
const QItemEditorFactory *QItemEditorFactory::defaultFactory()
{
    static const QDefaultItemEditorFactory factory;
    if (q_default_factory)
        return q_default_factory;
    return &factory;
}

/*!
    Sets the default item editor factory to the given \a factory.
    Both new and existing delegates will use the new factory.

    \sa defaultFactory()
*/
void QItemEditorFactory::setDefaultFactory(QItemEditorFactory *factory)
{
    static const QDefaultFactoryCleaner cleaner;
    delete q_default_factory;
    q_default_factory = factory;
}

/*!
    \class QItemEditorCreatorBase
    \brief The QItemEditorCreatorBase class provides an abstract base class that
    must be subclassed when implementing new item editor creators.
    \since 4.2
    \ingroup model-view
    \inmodule QtWidgets

    QItemEditorCreatorBase objects are specialized widget factories that
    provide editor widgets for one particular QVariant data type. They
    are used by QItemEditorFactory to create editors for
    \l{QStyledItemDelegate}s. Creator bases must be registered with
    QItemEditorFactory::registerEditor().

    An editor should provide a user property for the data it edits.
    QItemDelagates can then access the property using Qt's
    \l{Meta-Object System}{meta-object system} to set and retrieve the
    editing data. A property is set as the user property with the USER
    keyword:

    \snippet code/src_gui_itemviews_qitemeditorfactory.cpp 0

    If the editor does not provide a user property, it must return the
    name of the property from valuePropertyName(); delegates will then
    use the name to access the property. If a user property exists,
    item delegates will not call valuePropertyName().

    QStandardItemEditorCreator is a convenience template class that can be used
    to register widgets without the need to subclass QItemEditorCreatorBase.

    \sa QStandardItemEditorCreator, QItemEditorFactory,
    {Model/View Programming}, {Color Editor Factory Example}
*/

/*!
    \fn QItemEditorCreatorBase::~QItemEditorCreatorBase()

    Destroys the editor creator object.
*/
QItemEditorCreatorBase::~QItemEditorCreatorBase()
{

}

/*!
    \fn QWidget *QItemEditorCreatorBase::createWidget(QWidget *parent) const

    Returns an editor widget with the given \a parent.

    When implementing this function in subclasses of this class, you must
    construct and return new editor widgets with the parent widget specified.
*/

/*!
    \fn QByteArray QItemEditorCreatorBase::valuePropertyName() const

    Returns the name of the property used to get and set values in the creator's
    editor widgets.

    When implementing this function in subclasses, you must ensure that the
    editor widget's property specified by this function can accept the type
    the creator is registered for. For example, a creator which constructs
    QCheckBox widgets to edit boolean values would return the
    \l{QCheckBox::checkable}{checkable} property name from this function,
    and must be registered in the item editor factory for the QMetaType::Bool
    type.

    Note: Since Qt 4.2 the item delegates query the user property of widgets,
    and only call this function if the widget has no user property. You can
    override this behavior by reimplementing QAbstractItemDelegate::setModelData()
    and QAbstractItemDelegate::setEditorData().

    \sa QMetaObject::userProperty(), QItemEditorFactory::registerEditor()
*/

/*!
    \class QItemEditorCreator
    \brief The QItemEditorCreator class makes it possible to create
           item editor creator bases without subclassing
           QItemEditorCreatorBase.

    \since 4.2
    \ingroup model-view
    \inmodule QtWidgets

    QItemEditorCreator is a convenience template class. It uses
    the template class to create editors for QItemEditorFactory.
    This way, it is not necessary to subclass
    QItemEditorCreatorBase.

    \snippet code/src_gui_itemviews_qitemeditorfactory.cpp 1

    The constructor takes the name of the property that contains the
    editing data. QStyledItemDelegate can then access the property by name
    when it sets and retrieves editing data. Only use this class if
    your editor does not define a user property (using the USER
    keyword in the Q_PROPERTY macro).  If the widget has a user
    property, you should use QStandardItemEditorCreator instead.

    \sa QItemEditorCreatorBase, QStandardItemEditorCreator,
        QItemEditorFactory, {Color Editor Factory Example}
*/

/*!
    \fn template <class T> QItemEditorCreator<T>::QItemEditorCreator(const QByteArray &valuePropertyName)

    Constructs an editor creator object using \a valuePropertyName
    as the name of the property to be used for editing. The
    property name is used by QStyledItemDelegate when setting and
    getting editor data.

    Note that the \a valuePropertyName is only used if the editor
    widget does not have a user property defined.
*/

/*!
    \fn template <class T> QWidget *QItemEditorCreator<T>::createWidget(QWidget *parent) const
    \reimp
*/

/*!
    \fn template <class T> QByteArray QItemEditorCreator<T>::valuePropertyName() const
    \reimp
*/

/*!
    \class QStandardItemEditorCreator

    \brief The QStandardItemEditorCreator class provides the
    possibility to register widgets without having to subclass
    QItemEditorCreatorBase.

    \since 4.2
    \ingroup model-view
    \inmodule QtWidgets

    This convenience template class makes it possible to register widgets without
    having to subclass QItemEditorCreatorBase.

    Example:

    \snippet code/src_gui_itemviews_qitemeditorfactory.cpp 2

    Setting the \c editorFactory created above in an item delegate via
    QStyledItemDelegate::setItemEditorFactory() makes sure that all values of type
    QMetaType::QDateTime will be edited in \c{MyFancyDateTimeEdit}.

    The editor must provide a user property that will contain the
    editing data. The property is used by \l{QStyledItemDelegate}s to set
    and retrieve the data (using Qt's \l{Meta-Object
    System}{meta-object system}). You set the user property with
    the USER keyword:

    \snippet code/src_gui_itemviews_qitemeditorfactory.cpp 3

    \sa QItemEditorCreatorBase, QItemEditorCreator,
        QItemEditorFactory, QStyledItemDelegate, {Color Editor Factory Example}
*/

/*!
    \fn template <class T> QStandardItemEditorCreator<T>::QStandardItemEditorCreator()

    Constructs an editor creator object.
*/

/*!
    \fn template <class T> QWidget *QStandardItemEditorCreator<T>::createWidget(QWidget *parent) const
    \reimp
*/

/*!
    \fn template <class T> QByteArray QStandardItemEditorCreator<T>::valuePropertyName() const
    \reimp
*/

#if QT_CONFIG(lineedit)

QExpandingLineEdit::QExpandingLineEdit(QWidget *parent)
    : QLineEdit(parent), originalWidth(-1), widgetOwnsGeometry(false)
{
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(resizeToContents()));
    updateMinimumWidth();
}

void QExpandingLineEdit::changeEvent(QEvent *e)
{
    switch (e->type())
    {
    case QEvent::FontChange:
    case QEvent::StyleChange:
    case QEvent::ContentsRectChange:
        updateMinimumWidth();
        break;
    default:
        break;
    }

    QLineEdit::changeEvent(e);
}

void QExpandingLineEdit::updateMinimumWidth()
{
    const QMargins tm = textMargins();
    const QMargins cm = contentsMargins();
    const int width = tm.left() + tm.right() + cm.left() + cm.right() + 4 /*horizontalMargin in qlineedit.cpp*/;

    QStyleOptionFrame opt;
    initStyleOption(&opt);

    int minWidth = style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(width, 0), this).width();
    setMinimumWidth(minWidth);
}

void QExpandingLineEdit::resizeToContents()
{
    int oldWidth = width();
    if (originalWidth == -1)
        originalWidth = oldWidth;
    if (QWidget *parent = parentWidget()) {
        QPoint position = pos();
        int hintWidth = minimumWidth() + fontMetrics().horizontalAdvance(displayText());
        int parentWidth = parent->width();
        int maxWidth = isRightToLeft() ? position.x() + oldWidth : parentWidth - position.x();
        int newWidth = qBound(qMin(originalWidth, maxWidth), hintWidth, maxWidth);
        if (widgetOwnsGeometry)
            setMaximumWidth(newWidth);
        if (isRightToLeft())
            move(position.x() - newWidth + oldWidth, position.y());
        resize(newWidth, height());
    }
}

#endif // QT_CONFIG(lineedit)

#if QT_CONFIG(combobox)

QBooleanComboBox::QBooleanComboBox(QWidget *parent)
    : QComboBox(parent)
{
    addItem(QComboBox::tr("False"));
    addItem(QComboBox::tr("True"));
}

void QBooleanComboBox::setValue(bool value)
{
    setCurrentIndex(value ? 1 : 0);
}

bool QBooleanComboBox::value() const
{
    return (currentIndex() == 1);
}

#endif // QT_CONFIG(combobox)

QT_END_NAMESPACE

#if QT_CONFIG(lineedit) || QT_CONFIG(combobox)
#include "qitemeditorfactory.moc"
#endif

#include "moc_qitemeditorfactory_p.cpp"
