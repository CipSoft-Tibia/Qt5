// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qv4referenceobject_p.h>

QT_BEGIN_NAMESPACE

DEFINE_OBJECT_VTABLE(QV4::ReferenceObject);

/*!
  \class QV4::ReferenceObject
  \brief An object that keeps track of the provenance of its owned
  value, allowing to reflect mutations on the original instance.

  \internal

  \section1 Copied Types and Mutations

  In QML, certain types are conceptually passed by value.
  Instances of those types are always copied when they are accessed
  or passed around.

  Let those be "Copied Type"s.

  For example, suppose that \c{foo} is an instance of a Copied Type
  that has a member \c{bar} that has some value \c{X}.

  Consider the following example:

  \qml
      import QtQuick

      Item {
          Component.onCompleted: {
             foo.bar = Y
             console.log(foo.bar)
          }
      }
  \endqml

  Where \c{Y} is some value that can inhabit \c{foo.bar} and whose
  stringified representation is distinguishable from \c{X}.

  One might expect that a stringified representation of \c{Y} should be logged.
  Nonetheless, as \c{foo} is a Copied Type, accessing it creates a copy.
  The access to the \c{bar} member and its further mutation is
  performed on the copy that was created, and thus is not retained by
  the object that \c{foo} refers to.

  If \c{copy} is an operation that performs a deep-copy of an object
  and returns it, the above snippet can be considered implicitly
  equivalent to the following:

  \qml
      import QtQuick

      Item {
          Component.onCompleted: {
             copy(foo).bar = Y
             console.log(copy(foo).bar)
          }
      }
  \endqml

  This can generally be surprising as it stands in contrast to the
  effect that the above assignment would have if \c{foo} wasn't a
  Copied Type. Similarly, it stands in contrast to what one could
  expect from the outcome of the same assignment in a Javascript
  environment, where the mutation might be expected to generally be
  visible in later steps no matter the type of \c{foo}.

  A ReferenceObject can be used to avoid this inconsistency by
  wrapping a value and providing a "write-back" mechanism that can
  reflect mutations back on the original value.

  Furthermore, a ReferenceObject can be used to load the data from
  the original value to ensure that the two values remain in sync, as
  the value might have been mutated while the copy is still alive,
  conceptually allowing for an "inverted write-back".

  \section1 Setting Up a ReferenceObject

  ReferenceObject is intended to be extended by inheritance.

  An object that is used to wrap a value that is copied around but
  has a provenance that requires a write-back, can inherit from
  ReferenceObject to plug into the write-back behavior.

  The heap part of the object should subclass
  QV4::Heap::ReferenceObject while the object part should subclass
  QV4::ReferenceObject.

  When initializing the heap part of the subclass,
  QV4::Heap::ReferenceObject::init should be called to set up the
  write-back mechanism.

  The write-back mechanism stores a reference to an object and,
  potentially, a property index to write-back at.

  Furthermore, a series of flags can be used to condition the
  behavior of the write-back.

  For example:

  \code
     void QV4::Heap::Foo::init(Heap::Object *object) {
         ReferenceObject::init(object, 1, ReferenceObject::Flag::CanWriteBack);
         // Some further initialization code
         ...
     }
  \endcode

  The snippet is an example of some sub-class \c{Foo} of
  ReferenceObject setting up for a write-back to the property at index
  1 of some object \c{object}.

  Generally, a sub-class of ReferenceObject will be used to wrap one
  or more Copied Types and provide a certain behavior.

  In certain situations there is no need to setup a write-back.
  For example, we might have certain cases where there is no original
  value to be wrapped while still in need of providing an object of
  the sub-classing type.

  One example of such a behavior is that of returning an instance from
  a C++ method to QML.

  When that is the case, the following initialization can be provided:

  \code
     void QV4::Heap::Foo::init(Heap::Object *object) {
         ReferenceObject::init(nullptr, -1, NoFlag);
         // Some further initialization code
         ...
     }
  \endcode

  \section1 Providing the Required Infrastructure for a Default Write-back

  Generally, to use the base implementation of write and read backs,
  as provided by QV4::ReferenceObject::readReference and
  QV4::ReferenceObject::writeBack, a sub-class should provide the
  following interface:

  \code
      void *storagePointer();
      const void *storagePointer();


       QVariant toVariant() const;
       bool setVariant(const QVariant &variant);
  \endcode

  The two overloads of \c{storagePointer} should provide access to
  the internal backing storage of the ReferenceObject.

  Generally, a ReferenceObject will be wrapping some instance of a C++
  type, owning a copy of the original instance used as a storage for
  writes and reads.
  An implementation of \c{storagePointer} should give access to this
  backing storage, which is used to know what value to write-back and
  where to write when reading back from the original value.

  For example, a Sequence type that wraps a QVariantList will own its
  own instance of a QVariantList. The implementation for
  \c{storagePointer} should give access to that owned instance.

  \c{toVariant} should provide a QVariant wrapped representation of
  the internal storage that the ReferenceObject uses.
  This is used during the write-back of a ReferenceObject whose
  original value was a QVariant backed instance.

  Do remember that instances of a ReferenceObject that are backing
  QVariant values should further pass the
  QV4::Heap::ReferenceObject::Flag::IsVariant flag at initialization
  time.

  On the opposite side, \c{setVariant} should switch the value that
  the ReferenceObject stores with the provided variant.
  This is used when a QVariant backed ReferenceObject performs a read
  of its original value, to allow for synchronization.

  It is still possible to use ReferenceObject without necessarily
  providing the above interface.
  QV4::DateObject is an example of a ReferenceObject sub-class that
  provides its own writeBack implementation and doesn't abide by the
  above.

  \section1 Performing a Write-back

  With a sub-class of ReferenceObject that was set-up as above, a
  write-back can be performed by calling
  QV4::ReferenceObject::writeBack.

  For example, some ReferenceObject subclass \c{Foo} might be
  backing, say, some map-like object.

  Internally, insertion of an element might pass by some method, say
  \c{insertElement}. An implementation might, for example, look like
  the following:

  \code
    bool Foo::insertElement(const QString& key, const Value& value) {
        // Insert the element if possible
        ...
        QV4::ReferenceObject::writeBack(d());
        ...
        // Some further handling
    }
  \endcode

  The call to writeBack will ensure that the newly inserted element
  will be reflected on the original value where the object originates
  from.

  Here \c{d()}, with \c{Foo} being a sub-class of ReferenceObject and
  thus of Object, is the heap part of \c{Foo} that should provide the
  interface specificed above and owns the actual storage of the stored
  value.

  \section1 Synchronizing with the Original Value

  QV4::ReferenceObject::readReference provides a way to obtain the
  current state of the value that the ReferenceObject refers to.

  When this read is performed, the obtained value will be stored back
  into the backing storage for the ReferenceObject.

  This allows the ReferenceObject to lazily load the latest data on
  demand and correctly reflect the original value.

  For example, say that \c{Foo} is a sub-class of ReferenceObject that
  is used to back array-like values.

  \c{Foo} should provide the usual \c{virtualGetLength} method to
  support the \c{length} property that we expect an array to have.

  In a possible implementation of \c{virtualGetLength}, \c{Foo} should
  ensure that readReference is called before providing a value for the
  length, to ensure that the latest data is available.

  For example:

  \code
    qint64 Foo::virtualGetLength(const Managed *m)
    {
        const Foo *foo = static_cast<const Foo *>(m);
        foo->readReference(d());
        // Return the length from the owned storage
    }
  \endcode

  \section1 Limiting Write-backs Based on Source Location

  \note We generally consider location-aware write-backs to be a
  mistake and expect to generally avoid further uses of them.
  Due to backward compatibility promises they cannot be universally
  enforced, possibly creating discrepancies in certain behaviors.
  If at some point possible, the feature might be backtracked on and
  removed, albeit this has shown to be difficult due to certain
  existing cross-dependencies.

  Consider the following code:

  \qml
      import QtQuick

      Text {
          Component.onCompleted: {
              var f = font // font is a value type and f is a value type reference
              f.bold = true;
              otherText.font = f
          }
      }
  \endqml

  Remembering that \c{font} is a Copied Type in QtQuick, \c{f} will be a
  Copied Type reference, internally backed by a ReferenceObject.
  Changing the \c{bold} property of \c{f} would internally perform a
  write-back which will reflect on the original \c{font} property of
  the \c{Text} component, as we would expect from the definition we
  gave above.

  Nonetheless, we could generally expect that the intention behind the
  code wasn't to update the original \c{font} property, but to pass a
  slightly modified version of its value to \c{otherText}.

  To avoid introducing this kind of possibly unintended behavior while
  still supporting a solution to the original problem, ReferenceObject
  allows limiting the availability of write-backs to a specific source
  location.

  For example, by limiting the availability of write-backs to the
  single statement where a Copied Type reference is created, we can
  address the most common cases of the original issue while avoiding
  the unintuitive behavior of the above.

  To enable source location enforcement,
  QV4::Heap::ReferenceObject::Flag::EnforcesLocation should be set
  when the ReferenceObject is initialized.

  A reference location should be set by calling
  QV4::Heap::ReferenceObject::setLocation.
  For example, during initialization, one might be set to limit
  write-backs to the currently processed statement, where the
  ReferenceObject was created, as follows:

  \code
  void Heap::Sequence::Foo::init(..., Heap::ReferenceObject::Flags flags) {
      ...
      ReferenceObject::init(..., flags & ReferenceObject::Flag::EnforcesLocation);
      ...
      if (CppStackFrame *frame = internalClass->engine->currentStackFrame)
          setLocation(frame->v4Function, frame->statementNumber());
      ...
  }
  \endcode

  Do note that calls to QV4::ReferenceObject::writeBack and
  QV4::ReferenceObject::readReference do not directly take into
  account location enforcement.

  This should generally be handled by the sub-class.
  QV4::Heap::ReferenceObject::isAttachedToProperty can be used to
  recognize whether the reference is still suitable for write-backs in
  a location-enforcement-aware way.
 */

QT_END_NAMESPACE
