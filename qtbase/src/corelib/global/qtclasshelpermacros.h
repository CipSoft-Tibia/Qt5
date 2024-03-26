// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTCLASSHELPERMACROS_H
#define QTCLASSHELPERMACROS_H

#include <QtCore/qtconfigmacros.h>

#if 0
#pragma qt_class(QtClassHelperMacros)
#pragma qt_sync_stop_processing
#endif

QT_BEGIN_NAMESPACE

#if defined(__cplusplus)

/*
   Some classes do not permit copies to be made of an object. These
   classes contains a private copy constructor and assignment
   operator to disable copying (the compiler gives an error message).
*/
#define Q_DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;

#define Q_DISABLE_COPY_MOVE(Class) \
    Q_DISABLE_COPY(Class) \
    Class(Class &&) = delete; \
    Class &operator=(Class &&) = delete;

/*
    Implementing a move assignment operator using an established
    technique (move-and-swap, pure swap) is just boilerplate.
    Here's a couple of *private* macros for convenience.

    To know which one to use:

    * if you don't have a move constructor (*) => use pure swap;
    * if you have a move constructor, then
      * if your class holds just memory (no file handles, no user-defined
        datatypes, etc.) => use pure swap;
      * use move and swap.

    The preference should always go for the move-and-swap one, as it
    will deterministically destroy the data previously held in *this,
    and not "dump" it in the moved-from object (which may then be alive
    for longer).

    The requirement for either macro is the presence of a member swap(),
    which any value class that defines its own special member functions
    should have anyhow.

    (*) Many value classes in Qt do not have move constructors; mostly,
    the implicitly shared classes using QSharedDataPointer and friends.
    The reason is mostly historical: those classes require either an
    out-of-line move constructor, which we could not provide before we
    made C++11 mandatory (and that we don't like anyhow), or
    an out-of-line dtor for the Q(E)DSP<Private> member (cf. QPixmap).

    If you can however add a move constructor to a class lacking it,
    consider doing so, then reevaluate which macro to choose.
*/
#define QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(Class) \
    Class &operator=(Class &&other) noexcept { \
        Class moved(std::move(other)); \
        swap(moved); \
        return *this; \
    }

#define QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(Class) \
    Class &operator=(Class &&other) noexcept { \
        swap(other); \
        return *this; \
    }

template <typename T> inline T *qGetPtrHelper(T *ptr) noexcept { return ptr; }
template <typename Ptr> inline auto qGetPtrHelper(Ptr &ptr) noexcept -> decltype(ptr.get())
{ static_assert(noexcept(ptr.get()), "Smart d pointers for Q_DECLARE_PRIVATE must have noexcept get()"); return ptr.get(); }

class QObject;
class QObjectPrivate;
namespace QtPrivate {
    template <typename ObjPrivate> void assertObjectType(QObjectPrivate *d);
    inline const QObject *getQObject(const QObjectPrivate *d);
}

#define Q_DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() noexcept \
    { Q_CAST_IGNORE_ALIGN(return reinterpret_cast<Class##Private *>(qGetPtrHelper(d_ptr));) } \
    inline const Class##Private* d_func() const noexcept \
    { Q_CAST_IGNORE_ALIGN(return reinterpret_cast<const Class##Private *>(qGetPtrHelper(d_ptr));) } \
    friend class Class##Private;

#define Q_DECLARE_PRIVATE_D(Dptr, Class) \
    inline Class##Private* d_func() noexcept \
    { Q_CAST_IGNORE_ALIGN(return reinterpret_cast<Class##Private *>(qGetPtrHelper(Dptr));) } \
    inline const Class##Private* d_func() const noexcept \
    { Q_CAST_IGNORE_ALIGN(return reinterpret_cast<const Class##Private *>(qGetPtrHelper(Dptr));) } \
    friend class Class##Private;

#define Q_DECLARE_PUBLIC(Class)                                    \
    inline Class* q_func() noexcept { return static_cast<Class *>(q_ptr); } \
    inline const Class* q_func() const noexcept { return static_cast<const Class *>(q_ptr); } \
    friend class Class; \
    friend const QObject *QtPrivate::getQObject(const QObjectPrivate *d); \
    template <typename ObjPrivate> friend void QtPrivate::assertObjectType(QObjectPrivate *d);

#define Q_D(Class) Class##Private * const d = d_func()
#define Q_Q(Class) Class * const q = q_func()

/*
   Specialize a shared type with:

     Q_DECLARE_SHARED(type)

   where 'type' is the name of the type to specialize.  NOTE: shared
   types must define a member-swap, and be defined in the same
   namespace as Qt for this to work.
*/

#define Q_DECLARE_SHARED(TYPE) \
Q_DECLARE_TYPEINFO(TYPE, Q_RELOCATABLE_TYPE); \
inline void swap(TYPE &value1, TYPE &value2) \
    noexcept(noexcept(value1.swap(value2))) \
{ value1.swap(value2); }

#endif // __cplusplus

QT_END_NAMESPACE

#endif // QTCLASSHELPERMACROS_H
