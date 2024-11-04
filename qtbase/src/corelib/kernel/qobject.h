// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOBJECT_H
#define QOBJECT_H

#ifndef QT_NO_QOBJECT

#include <QtCore/qobjectdefs.h>
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#ifdef QT_INCLUDE_COMPAT
#include <QtCore/qcoreevent.h>
#endif
#include <QtCore/qscopedpointer.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qobject_impl.h>
#include <QtCore/qbindingstorage.h>

#include <chrono>

QT_BEGIN_NAMESPACE


template <typename T> class QBindable;
class QEvent;
class QTimerEvent;
class QChildEvent;
struct QMetaObject;
class QVariant;
class QObjectPrivate;
class QObject;
class QThread;
class QWidget;
class QAccessibleWidget;
#if QT_CONFIG(regularexpression)
class QRegularExpression;
#endif
struct QDynamicMetaObjectData;

typedef QList<QObject*> QObjectList;

#if QT_CORE_REMOVED_SINCE(6, 7)
Q_CORE_EXPORT void qt_qFindChildren_helper(const QObject *parent, const QString &name,
                                           const QMetaObject &mo, QList<void *> *list, Qt::FindChildOptions options);
#endif
Q_CORE_EXPORT void qt_qFindChildren_helper(const QObject *parent, QAnyStringView name,
                                           const QMetaObject &mo, QList<void *> *list,
                                           Qt::FindChildOptions options);
#if QT_CORE_REMOVED_SINCE(6, 7)
Q_CORE_EXPORT void qt_qFindChildren_helper(const QObject *parent, const QMetaObject &mo,
                                           QList<void *> *list, Qt::FindChildOptions options);
#endif
Q_CORE_EXPORT void qt_qFindChildren_helper(const QObject *parent, const QRegularExpression &re,
                                           const QMetaObject &mo, QList<void *> *list, Qt::FindChildOptions options);
#if QT_CORE_REMOVED_SINCE(6, 7)
Q_CORE_EXPORT QObject *qt_qFindChild_helper(const QObject *parent, const QString &name, const QMetaObject &mo, Qt::FindChildOptions options);
#endif
Q_CORE_EXPORT QObject *qt_qFindChild_helper(const QObject *parent, QAnyStringView name,
                                            const QMetaObject &mo, Qt::FindChildOptions options);

class Q_CORE_EXPORT QObjectData
{
    Q_DISABLE_COPY(QObjectData)
public:
    QObjectData() = default;
    virtual ~QObjectData() = 0;
    QObject *q_ptr;
    QObject *parent;
    QObjectList children;

    uint isWidget : 1;
    uint blockSig : 1;
    uint wasDeleted : 1;
    uint isDeletingChildren : 1;
    uint sendChildEvents : 1;
    uint receiveChildEvents : 1;
    uint isWindow : 1; // for QWindow
    uint deleteLaterCalled : 1;
    uint isQuickItem : 1;
    uint willBeWidget : 1; // for handling widget-specific bits in QObject's ctor
    uint wasWidget : 1; // for properly cleaning up in QObject's dtor
    uint receiveParentEvents: 1;
    uint unused : 20;
    QAtomicInt postedEvents;
    QDynamicMetaObjectData *metaObject;
    QBindingStorage bindingStorage;

    // ### Qt7: Make this return a const QMetaObject *. You should not mess with
    //          the metaobjects of existing objects.
    QMetaObject *dynamicMetaObject() const;

#ifdef QT_DEBUG
    enum { CheckForParentChildLoopsWarnDepth = 4096 };
#endif
};

class Q_CORE_EXPORT QObject
{
    Q_OBJECT

    Q_PROPERTY(QString objectName READ objectName WRITE setObjectName NOTIFY objectNameChanged
               BINDABLE bindableObjectName)
    Q_DECLARE_PRIVATE(QObject)

public:
    Q_INVOKABLE explicit QObject(QObject *parent = nullptr);
    virtual ~QObject();

    virtual bool event(QEvent *event);
    virtual bool eventFilter(QObject *watched, QEvent *event);

#if defined(QT_NO_TRANSLATION) || defined(Q_QDOC)
    static QString tr(const char *sourceText, const char * = nullptr, int = -1)
        { return QString::fromUtf8(sourceText); }
#endif // QT_NO_TRANSLATION

    QString objectName() const;
#if QT_CORE_REMOVED_SINCE(6, 4)
    void setObjectName(const QString &name);
#endif
    Q_WEAK_OVERLOAD
    void setObjectName(const QString &name) { doSetObjectName(name); }
    void setObjectName(QAnyStringView name);
    QBindable<QString> bindableObjectName();

    inline bool isWidgetType() const { return d_ptr->isWidget; }
    inline bool isWindowType() const { return d_ptr->isWindow; }
    inline bool isQuickItemType() const { return d_ptr->isQuickItem; }

    inline bool signalsBlocked() const noexcept { return d_ptr->blockSig; }
    bool blockSignals(bool b) noexcept;

    QThread *thread() const;
#if QT_CORE_REMOVED_SINCE(6, 7)
    void moveToThread(QThread *thread);
#endif
    bool moveToThread(QThread *thread QT6_DECL_NEW_OVERLOAD_TAIL);

    int startTimer(int interval, Qt::TimerType timerType = Qt::CoarseTimer);
    int startTimer(std::chrono::milliseconds time, Qt::TimerType timerType = Qt::CoarseTimer);
    void killTimer(int id);

    template<typename T>
    T findChild(QAnyStringView aName, Qt::FindChildOptions options = Qt::FindChildrenRecursively) const
    {
        typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
        static_assert(QtPrivate::HasQ_OBJECT_Macro<ObjType>::Value,
                          "No Q_OBJECT in the class passed to QObject::findChild");
        return static_cast<T>(qt_qFindChild_helper(this, aName, ObjType::staticMetaObject, options));
    }

    template<typename T>
    QList<T> findChildren(QAnyStringView aName, Qt::FindChildOptions options = Qt::FindChildrenRecursively) const
    {
        typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
        static_assert(QtPrivate::HasQ_OBJECT_Macro<ObjType>::Value,
                          "No Q_OBJECT in the class passed to QObject::findChildren");
        QList<T> list;
        qt_qFindChildren_helper(this, aName, ObjType::staticMetaObject,
                                reinterpret_cast<QList<void *> *>(&list), options);
        return list;
    }

    template<typename T>
    T findChild(Qt::FindChildOptions options = Qt::FindChildrenRecursively) const
    {
        return findChild<T>({}, options);
    }

    template<typename T>
    QList<T> findChildren(Qt::FindChildOptions options = Qt::FindChildrenRecursively) const
    {
        return findChildren<T>(QAnyStringView{}, options);
    }

#if QT_CONFIG(regularexpression)
    template<typename T>
    inline QList<T> findChildren(const QRegularExpression &re, Qt::FindChildOptions options = Qt::FindChildrenRecursively) const
    {
        typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
        static_assert(QtPrivate::HasQ_OBJECT_Macro<ObjType>::Value,
                          "No Q_OBJECT in the class passed to QObject::findChildren");
        QList<T> list;
        qt_qFindChildren_helper(this, re, ObjType::staticMetaObject,
                                reinterpret_cast<QList<void *> *>(&list), options);
        return list;
    }
#endif // QT_CONFIG(regularexpression)

    inline const QObjectList &children() const { return d_ptr->children; }

    void setParent(QObject *parent);
    void installEventFilter(QObject *filterObj);
    void removeEventFilter(QObject *obj);

    static QMetaObject::Connection connect(const QObject *sender, const char *signal,
                        const QObject *receiver, const char *member, Qt::ConnectionType = Qt::AutoConnection);

    static QMetaObject::Connection connect(const QObject *sender, const QMetaMethod &signal,
                        const QObject *receiver, const QMetaMethod &method,
                        Qt::ConnectionType type = Qt::AutoConnection);

    inline QMetaObject::Connection connect(const QObject *sender, const char *signal,
                        const char *member, Qt::ConnectionType type = Qt::AutoConnection) const;

#ifdef Q_QDOC
    template<typename PointerToMemberFunction>
    static QMetaObject::Connection connect(const QObject *sender, PointerToMemberFunction signal, const QObject *receiver, PointerToMemberFunction method, Qt::ConnectionType type = Qt::AutoConnection);
    template<typename PointerToMemberFunction, typename Functor>
    static QMetaObject::Connection connect(const QObject *sender, PointerToMemberFunction signal, Functor functor);
    template<typename PointerToMemberFunction, typename Functor>
    static QMetaObject::Connection connect(const QObject *sender, PointerToMemberFunction signal, const QObject *context, Functor functor, Qt::ConnectionType type = Qt::AutoConnection);
#else
    //connect with context
    template <typename Func1, typename Func2>
    static inline QMetaObject::Connection
        connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                const typename QtPrivate::ContextTypeForFunctor<Func2>::ContextType *context, Func2 &&slot,
                Qt::ConnectionType type = Qt::AutoConnection)
    {
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        typedef QtPrivate::FunctionPointer<std::decay_t<Func2>> SlotType;

        if constexpr (SlotType::ArgumentCount != -1) {
            static_assert((QtPrivate::AreArgumentsCompatible<typename SlotType::ReturnType, typename SignalType::ReturnType>::value),
                            "Return type of the slot is not compatible with the return type of the signal.");
        } else {
            constexpr int FunctorArgumentCount = QtPrivate::ComputeFunctorArgumentCount<std::decay_t<Func2>, typename SignalType::Arguments>::Value;
            [[maybe_unused]]
            constexpr int SlotArgumentCount = (FunctorArgumentCount >= 0) ? FunctorArgumentCount : 0;
            typedef typename QtPrivate::FunctorReturnType<std::decay_t<Func2>, typename QtPrivate::List_Left<typename SignalType::Arguments, SlotArgumentCount>::Value>::Value SlotReturnType;

            static_assert((QtPrivate::AreArgumentsCompatible<SlotReturnType, typename SignalType::ReturnType>::value),
                            "Return type of the slot is not compatible with the return type of the signal.");
        }

        static_assert(QtPrivate::HasQ_OBJECT_Macro<typename SignalType::Object>::Value,
                          "No Q_OBJECT in the class with the signal");

        //compilation error if the arguments does not match.
        static_assert(int(SignalType::ArgumentCount) >= int(SlotType::ArgumentCount),
                          "The slot requires more arguments than the signal provides.");

        const int *types = nullptr;
        if (type == Qt::QueuedConnection || type == Qt::BlockingQueuedConnection)
            types = QtPrivate::ConnectionTypes<typename SignalType::Arguments>::types();

        void **pSlot = nullptr;
        if constexpr (std::is_member_function_pointer_v<std::decay_t<Func2>>) {
            pSlot = const_cast<void **>(reinterpret_cast<void *const *>(&slot));
        } else {
            Q_ASSERT_X((type & Qt::UniqueConnection) == 0, "",
                       "QObject::connect: Unique connection requires the slot to be a pointer to "
                       "a member function of a QObject subclass.");
        }

        return connectImpl(sender, reinterpret_cast<void **>(&signal), context, pSlot,
                           QtPrivate::makeCallableObject<Func1>(std::forward<Func2>(slot)),
                           type, types, &SignalType::Object::staticMetaObject);
    }

#ifndef QT_NO_CONTEXTLESS_CONNECT
    //connect without context
    template <typename Func1, typename Func2>
    static inline QMetaObject::Connection
        connect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal, Func2 &&slot)
    {
        return connect(sender, signal, sender, std::forward<Func2>(slot), Qt::DirectConnection);
    }
#endif // QT_NO_CONTEXTLESS_CONNECT
#endif //Q_QDOC

    static bool disconnect(const QObject *sender, const char *signal,
                           const QObject *receiver, const char *member);
    static bool disconnect(const QObject *sender, const QMetaMethod &signal,
                           const QObject *receiver, const QMetaMethod &member);
    inline bool disconnect(const char *signal = nullptr,
                           const QObject *receiver = nullptr, const char *member = nullptr) const
        { return disconnect(this, signal, receiver, member); }
    inline bool disconnect(const QObject *receiver, const char *member = nullptr) const
        { return disconnect(this, nullptr, receiver, member); }
    static bool disconnect(const QMetaObject::Connection &);

#ifdef Q_QDOC
    template<typename PointerToMemberFunction>
    static bool disconnect(const QObject *sender, PointerToMemberFunction signal, const QObject *receiver, PointerToMemberFunction method);
#else
    template <typename Func1, typename Func2>
    static inline bool disconnect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                  const typename QtPrivate::FunctionPointer<Func2>::Object *receiver, Func2 slot)
    {
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        typedef QtPrivate::FunctionPointer<Func2> SlotType;

        static_assert(QtPrivate::HasQ_OBJECT_Macro<typename SignalType::Object>::Value,
                          "No Q_OBJECT in the class with the signal");

        //compilation error if the arguments does not match.
        static_assert((QtPrivate::CheckCompatibleArguments<typename SignalType::Arguments, typename SlotType::Arguments>::value),
                          "Signal and slot arguments are not compatible.");

        return disconnectImpl(sender, reinterpret_cast<void **>(&signal), receiver, reinterpret_cast<void **>(&slot),
                              &SignalType::Object::staticMetaObject);
    }
    template <typename Func1>
    static inline bool disconnect(const typename QtPrivate::FunctionPointer<Func1>::Object *sender, Func1 signal,
                                  const QObject *receiver, void **zero)
    {
        // This is the overload for when one wish to disconnect a signal from any slot. (slot=nullptr)
        // Since the function template parameter cannot be deduced from '0', we use a
        // dummy void ** parameter that must be equal to 0
        Q_ASSERT(!zero);
        typedef QtPrivate::FunctionPointer<Func1> SignalType;
        return disconnectImpl(sender, reinterpret_cast<void **>(&signal), receiver, zero,
                              &SignalType::Object::staticMetaObject);
    }
#endif //Q_QDOC

    void dumpObjectTree() const;
    void dumpObjectInfo() const;

    QT_CORE_INLINE_SINCE(6, 6)
    bool setProperty(const char *name, const QVariant &value);
    inline bool setProperty(const char *name, QVariant &&value);
    QVariant property(const char *name) const;
    QList<QByteArray> dynamicPropertyNames() const;
    QBindingStorage *bindingStorage() { return &d_ptr->bindingStorage; }
    const QBindingStorage *bindingStorage() const { return &d_ptr->bindingStorage; }

Q_SIGNALS:
    void destroyed(QObject * = nullptr);
    void objectNameChanged(const QString &objectName, QPrivateSignal);

public:
    inline QObject *parent() const { return d_ptr->parent; }

    inline bool inherits(const char *classname) const
    {
        return const_cast<QObject *>(this)->qt_metacast(classname) != nullptr;
    }

public Q_SLOTS:
    void deleteLater();

protected:
    QObject *sender() const;
    int senderSignalIndex() const;
    int receivers(const char *signal) const;
    bool isSignalConnected(const QMetaMethod &signal) const;

    virtual void timerEvent(QTimerEvent *event);
    virtual void childEvent(QChildEvent *event);
    virtual void customEvent(QEvent *event);

    virtual void connectNotify(const QMetaMethod &signal);
    virtual void disconnectNotify(const QMetaMethod &signal);

protected:
    QObject(QObjectPrivate &dd, QObject *parent = nullptr);

protected:
    QScopedPointer<QObjectData> d_ptr;

    friend struct QMetaObject;
    friend struct QMetaObjectPrivate;
    friend class QMetaCallEvent;
    friend class QApplication;
    friend class QApplicationPrivate;
    friend class QCoreApplication;
    friend class QCoreApplicationPrivate;
    friend class QWidget;
    friend class QAccessibleWidget;
    friend class QThreadData;

private:
    void doSetObjectName(const QString &name);
    bool doSetProperty(const char *name, const QVariant *lvalue, QVariant *rvalue);

    Q_DISABLE_COPY(QObject)

private:
    static QMetaObject::Connection connectImpl(const QObject *sender, void **signal,
                                               const QObject *receiver, void **slotPtr,
                                               QtPrivate::QSlotObjectBase *slot, Qt::ConnectionType type,
                                               const int *types, const QMetaObject *senderMetaObject);

    static bool disconnectImpl(const QObject *sender, void **signal, const QObject *receiver, void **slot,
                               const QMetaObject *senderMetaObject);

};

inline QMetaObject::Connection QObject::connect(const QObject *asender, const char *asignal,
                                            const char *amember, Qt::ConnectionType atype) const
{ return connect(asender, asignal, this, amember, atype); }

#if QT_CORE_INLINE_IMPL_SINCE(6, 6)
bool QObject::setProperty(const char *name, const QVariant &value)
{
    return doSetProperty(name, &value, nullptr);
}
#endif // inline since 6.6
bool QObject::setProperty(const char *name, QVariant &&value)
{
    return doSetProperty(name, &value, &value);
}

template <class T>
inline T qobject_cast(QObject *object)
{
    static_assert(std::is_pointer_v<T>,
                  "qobject_cast requires to cast towards a pointer type");
    typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
    static_assert(QtPrivate::HasQ_OBJECT_Macro<ObjType>::Value,
                    "qobject_cast requires the type to have a Q_OBJECT macro");
    return static_cast<T>(ObjType::staticMetaObject.cast(object));
}

template <class T>
inline T qobject_cast(const QObject *object)
{
    static_assert(std::is_pointer_v<T>,
                  "qobject_cast requires to cast towards a pointer type");
    static_assert(std::is_const_v<std::remove_pointer_t<T>>,
                  "qobject_cast cannot cast away constness (use const_cast)");
    typedef typename std::remove_cv<typename std::remove_pointer<T>::type>::type ObjType;
    static_assert(QtPrivate::HasQ_OBJECT_Macro<ObjType>::Value,
                      "qobject_cast requires the type to have a Q_OBJECT macro");
    return static_cast<T>(ObjType::staticMetaObject.cast(object));
}


template <class T> constexpr const char * qobject_interface_iid() = delete;
template <class T> inline T *
qobject_iid_cast(QObject *object, const char *IId = qobject_interface_iid<T *>())
{
    return reinterpret_cast<T *>((object ? object->qt_metacast(IId) : nullptr));
}
template <class T> inline std::enable_if_t<std::is_const<T>::value, T *>
qobject_iid_cast(const QObject *object)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    QObject *o = const_cast<QObject *>(object);
    return qobject_iid_cast<std::remove_cv_t<T>>(o);
}

#if defined(Q_QDOC)
#  define Q_DECLARE_INTERFACE(IFace, IId)
#elif !defined(Q_MOC_RUN)
#  define Q_DECLARE_INTERFACE(IFace, IId) \
    template <> constexpr const char *qobject_interface_iid<IFace *>() \
    { return IId; } \
    template <> inline IFace *qobject_cast<IFace *>(QObject *object) \
    { return qobject_iid_cast<IFace>(object); } \
    template <> inline const IFace *qobject_cast<const IFace *>(const QObject *object) \
    { return qobject_iid_cast<const IFace>(object); }
#endif // Q_MOC_RUN

inline const QBindingStorage *qGetBindingStorage(const QObject *o)
{
    return o->bindingStorage();
}
inline QBindingStorage *qGetBindingStorage(QObject *o)
{
    return o->bindingStorage();
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QObject *);
#endif

class QSignalBlocker
{
public:
    Q_NODISCARD_CTOR
    inline explicit QSignalBlocker(QObject *o) noexcept;
    Q_NODISCARD_CTOR
    inline explicit QSignalBlocker(QObject &o) noexcept;
    inline ~QSignalBlocker();

    Q_NODISCARD_CTOR
    inline QSignalBlocker(QSignalBlocker &&other) noexcept;
    inline QSignalBlocker &operator=(QSignalBlocker &&other) noexcept;

    inline void reblock() noexcept;
    inline void unblock() noexcept;
    inline void dismiss() noexcept;

private:
    Q_DISABLE_COPY(QSignalBlocker)
    QObject *m_o;
    bool m_blocked;
    bool m_inhibited;
};

QSignalBlocker::QSignalBlocker(QObject *o) noexcept
    : m_o(o),
      m_blocked(o && o->blockSignals(true)),
      m_inhibited(false)
{}

QSignalBlocker::QSignalBlocker(QObject &o) noexcept
    : m_o(&o),
      m_blocked(o.blockSignals(true)),
      m_inhibited(false)
{}

QSignalBlocker::QSignalBlocker(QSignalBlocker &&other) noexcept
    : m_o(other.m_o),
      m_blocked(other.m_blocked),
      m_inhibited(other.m_inhibited)
{
    other.m_o = nullptr;
}

QSignalBlocker &QSignalBlocker::operator=(QSignalBlocker &&other) noexcept
{
    if (this != &other) {
        // if both *this and other block the same object's signals:
        // unblock *this iff our dtor would unblock, but other's wouldn't
        if (m_o != other.m_o || (!m_inhibited && other.m_inhibited))
            unblock();
        m_o = other.m_o;
        m_blocked = other.m_blocked;
        m_inhibited = other.m_inhibited;
        // disable other:
        other.m_o = nullptr;
    }
    return *this;
}

QSignalBlocker::~QSignalBlocker()
{
    if (m_o && !m_inhibited)
        m_o->blockSignals(m_blocked);
}

void QSignalBlocker::reblock() noexcept
{
    if (m_o)
        m_o->blockSignals(true);
    m_inhibited = false;
}

void QSignalBlocker::unblock() noexcept
{
    if (m_o)
        m_o->blockSignals(m_blocked);
    m_inhibited = true;
}

void QSignalBlocker::dismiss() noexcept
{
    m_o = nullptr;
}

namespace QtPrivate {
    inline QObject & deref_for_methodcall(QObject &o) { return  o; }
    inline QObject & deref_for_methodcall(QObject *o) { return *o; }
}
#define Q_SET_OBJECT_NAME(obj) QT_PREPEND_NAMESPACE(QtPrivate)::deref_for_methodcall(obj).setObjectName(QLatin1StringView(#obj))

QT_END_NAMESPACE

#endif

#endif // QOBJECT_H
