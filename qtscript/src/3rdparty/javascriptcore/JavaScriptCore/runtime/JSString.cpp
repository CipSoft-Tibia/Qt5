/*
 *  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2004, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "JSString.h"

#include "JSGlobalObject.h"
#include "JSObject.h"
#include "Operations.h"
#include "StringObject.h"
#include "StringPrototype.h"

namespace JSC {

void JSString::Rope::destructNonRecursive()
{
    Vector<Rope*, 32> workQueue;
    Rope* rope = this;

    while (true) {
        unsigned length = rope->ropeLength();
        for (unsigned i = 0; i < length; ++i) {
            Fiber& fiber = rope->fibers(i);
            if (fiber.isString())
                fiber.string()->deref();
            else {
                Rope* nextRope = fiber.rope();
                if (nextRope->hasOneRef())
                    workQueue.append(nextRope);
                else
                    nextRope->deref();
            }
        }
        if (rope != this)
            fastFree(rope);

        if (workQueue.isEmpty())
            return;

        rope = workQueue.last();
        workQueue.removeLast();
    }
}

JSString::Rope::~Rope()
{
    destructNonRecursive();
}

// Overview: this methods converts a JSString from holding a string in rope form
// down to a simple UString representation.  It does so by building up the string
// backwards, since we want to avoid recursion, we expect that the tree structure
// representing the rope is likely imbalanced with more nodes down the left side
// (since appending to the string is likely more common) - and as such resolving
// in this fashion should minimize work queue size.  (If we built the queue forwards
// we would likely have to place all of the constituent UString::Reps into the
// Vector before performing any concatenation, but by working backwards we likely
// only fill the queue with the number of substrings at any given level in a
// rope-of-ropes.)
void JSString::resolveRope(ExecState* exec) const
{
    ASSERT(isRope());

    // Allocate the buffer to hold the final string, position initially points to the end.
    UChar* buffer;
    if (PassRefPtr<UStringImpl> newImpl = UStringImpl::tryCreateUninitialized(m_stringLength, buffer))
        m_value = newImpl;
    else {
        for (unsigned i = 0; i < m_ropeLength; ++i) {
            m_fibers[i].deref();
            m_fibers[i] = static_cast<void*>(0);
        }
        m_ropeLength = 0;
        ASSERT(!isRope());
        ASSERT(m_value == UString());
        throwOutOfMemoryError(exec);
        return;
    }
    UChar* position = buffer + m_stringLength;

    // Start with the current Rope.
    Vector<Rope::Fiber, 32> workQueue;
    Rope::Fiber currentFiber;
    for (unsigned i = 0; i < (m_ropeLength - 1); ++i)
        workQueue.append(m_fibers[i]);
    currentFiber = m_fibers[m_ropeLength - 1];
    while (true) {
        if (currentFiber.isRope()) {
            Rope* rope = currentFiber.rope();
            // Copy the contents of the current rope into the workQueue, with the last item in 'currentFiber'
            // (we will be working backwards over the rope).
            unsigned ropeLengthMinusOne = rope->ropeLength() - 1;
            for (unsigned i = 0; i < ropeLengthMinusOne; ++i)
                workQueue.append(rope->fibers(i));
            currentFiber = rope->fibers(ropeLengthMinusOne);
        } else {
            UString::Rep* string = currentFiber.string();
            unsigned length = string->size();
            position -= length;
            UStringImpl::copyChars(position, string->data(), length);

            // Was this the last item in the work queue?
            if (workQueue.isEmpty()) {
                // Create a string from the UChar buffer, clear the rope RefPtr.
                ASSERT(buffer == position);
                for (unsigned i = 0; i < m_ropeLength; ++i) {
                    m_fibers[i].deref();
                    m_fibers[i] = static_cast<void*>(0);
                }
                m_ropeLength = 0;

                ASSERT(!isRope());
                return;
            }

            // No! - set the next item up to process.
            currentFiber = workQueue.last();
            workQueue.removeLast();
        }
    }
}

JSValue JSString::toPrimitive(ExecState*, PreferredPrimitiveType) const
{
    return const_cast<JSString*>(this);
}

bool JSString::getPrimitiveNumber(ExecState* exec, double& number, JSValue& result)
{
    result = this;
    number = value(exec).toDouble();
    return false;
}

bool JSString::toBoolean(ExecState*) const
{
    return m_stringLength;
}

double JSString::toNumber(ExecState* exec) const
{
    return value(exec).toDouble();
}

UString JSString::toString(ExecState* exec) const
{
    return value(exec);
}

UString JSString::toThisString(ExecState* exec) const
{
    return value(exec);
}

JSString* JSString::toThisJSString(ExecState*)
{
    return this;
}

inline StringObject* StringObject::create(ExecState* exec, JSString* string)
{
    return new (exec) StringObject(exec->lexicalGlobalObject()->stringObjectStructure(), string);
}

JSObject* JSString::toObject(ExecState* exec) const
{
    return StringObject::create(exec, const_cast<JSString*>(this));
}

JSObject* JSString::toThisObject(ExecState* exec) const
{
    return StringObject::create(exec, const_cast<JSString*>(this));
}

bool JSString::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    // The semantics here are really getPropertySlot, not getOwnPropertySlot.
    // This function should only be called by JSValue::get.
    if (getStringPropertySlot(exec, propertyName, slot))
        return true;
    if (propertyName == exec->propertyNames().underscoreProto) {
        slot.setValue(exec->lexicalGlobalObject()->stringPrototype());
        return true;
    }
    slot.setBase(this);
    JSObject* object;
    for (JSValue prototype = exec->lexicalGlobalObject()->stringPrototype(); !prototype.isNull(); prototype = object->prototype()) {
        object = asObject(prototype);
        if (object->getOwnPropertySlot(exec, propertyName, slot))
            return true;
    }
    slot.setUndefined();
    return true;
}

bool JSString::getStringPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (propertyName == exec->propertyNames().length) {
        descriptor.setDescriptor(jsNumber(exec, m_stringLength), DontEnum | DontDelete | ReadOnly);
        return true;
    }
    
    bool isStrictUInt32;
    unsigned i = propertyName.toStrictUInt32(&isStrictUInt32);
    if (isStrictUInt32 && i < m_stringLength) {
        descriptor.setDescriptor(jsSingleCharacterSubstring(exec, value(exec), i), DontDelete | ReadOnly);
        return true;
    }
    
    return false;
}

bool JSString::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (getStringPropertyDescriptor(exec, propertyName, descriptor))
        return true;
    if (propertyName != exec->propertyNames().underscoreProto)
        return false;
    descriptor.setDescriptor(exec->lexicalGlobalObject()->stringPrototype(), DontEnum);
    return true;
}

bool JSString::getOwnPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    // The semantics here are really getPropertySlot, not getOwnPropertySlot.
    // This function should only be called by JSValue::get.
    if (getStringPropertySlot(exec, propertyName, slot))
        return true;
    return JSString::getOwnPropertySlot(exec, Identifier::from(exec, propertyName), slot);
}

} // namespace JSC
