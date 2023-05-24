// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "bmbase_p.h"

#include <QLoggingCategory>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcLottieQtBodymovinParser, "qt.lottieqt.bodymovin.parser");
Q_LOGGING_CATEGORY(lcLottieQtBodymovinUpdate, "qt.lottieqt.bodymovin.update");
Q_LOGGING_CATEGORY(lcLottieQtBodymovinRender, "qt.lottieqt.bodymovin.render");

BMBase::BMBase(const BMBase &other)
{
    m_definition = other.m_definition;
    m_type = other.m_type;
    m_hidden = other.m_hidden;
    m_name = other.m_name;
    m_autoOrient = other.m_autoOrient;
    for (BMBase *child : other.m_children) {
        BMBase *clone = child->clone();
        clone->setParent(this);
        appendChild(clone);
    }
}

BMBase::~BMBase()
{
    qDeleteAll(m_children);
}

BMBase *BMBase::clone() const
{
    return new BMBase(*this);
}

QString BMBase::name() const
{
    return m_name;
}

void BMBase::setName(const QString &name)
{
    m_name = name;
}

bool BMBase::setProperty(BMLiteral::PropertyType propertyName, QVariant value)
{
    for (BMBase *child : std::as_const(m_children)) {
        bool changed = child->setProperty(propertyName, value);
        if (changed)
            return true;
    }
    return false;
}

int BMBase::type() const
{
    return m_type;
}

void BMBase::setType(int type)
{
    m_type = type;
}

void BMBase::prependChild(BMBase *child)
{
    m_children.push_front(child);
}

void BMBase::appendChild(BMBase *child)
{
    m_children.push_back(child);
}

BMBase *BMBase::findChild(const QString &childName)
{
    if (name() == childName)
        return this;

    BMBase *found = nullptr;
    for (BMBase *child : std::as_const(m_children)) {
        found = child->findChild(childName);
        if (found)
            break;
    }
    return found;
}

void BMBase::updateProperties(int frame)
{
    if (m_hidden)
        return;

    for (BMBase *child : std::as_const(m_children))
        child->updateProperties(frame);
}

void BMBase::render(LottieRenderer &renderer) const
{
    if (m_hidden)
        return;

    renderer.saveState();
    for (BMBase *child : std::as_const(m_children)) {
        if (child->m_hidden)
            continue;
        child->render(renderer);
    }
    renderer.restoreState();
}

void BMBase::resolveTopRoot()
{
    if (!m_topRoot) {
        BMBase *p = this;
        while (p) {
            m_topRoot = p;
            p = p->parent();
        }
    }
    Q_ASSERT(m_topRoot);
}

BMBase *BMBase::topRoot() const
{
    return m_topRoot;
}

void BMBase::parse(const QJsonObject &definition)
{
    qCDebug(lcLottieQtBodymovinParser) << "BMBase::parse()";

    m_definition = definition;

    m_hidden = definition.value(QLatin1String("hd")).toBool(false);
    m_name = definition.value(QLatin1String("nm")).toString();
    m_matchName = definition.value(QLatin1String("mn")).toString();
    m_autoOrient = definition.value(QLatin1String("ao")).toBool();

    if (m_autoOrient)
        qCWarning(lcLottieQtBodymovinParser)
                << "Element has auto-orientation set, but it is not supported";
}

const QJsonObject &BMBase::definition() const
{
    return m_definition;
}

bool BMBase::active(int frame) const
{
    Q_UNUSED(frame);
    return !m_hidden;
}

bool BMBase::hidden() const
{
    return m_hidden;
}

void BMBase::setParent(BMBase *parent)
{
    m_parent = parent;
}


const QJsonObject BMBase::resolveExpression(const QJsonObject &definition)
{
    QString expr = definition.value(QLatin1String("x")).toString();

    // If there is no expression, return the original object definition
    if (expr.isEmpty())
        return definition;

    // Find out layer handle
    resolveTopRoot();

    QRegularExpression re(QStringLiteral("effect\\(\\'(.*?)\\'\\)\\(\\'(.*?)\\'\\)"));
    QRegularExpressionMatch match = re.match(expr);
    if (!match.hasMatch())
        return definition;

    QString effect = match.captured(1);
    QString elementName = match.captured(2);

    QJsonObject retVal = definition;

    if (BMBase *source = m_topRoot->findChild(effect)) {
        if (source->children().size())
            retVal = source->children().at(0)->definition().value(QLatin1String("v")).toObject();
        else
            retVal = source->definition().value(QLatin1String("v")).toObject();
        if (source->children().size() > 1)
            qCWarning(lcLottieQtBodymovinParser) << "Effect source points"
                                                "to a group that has"
                                                "many children. The"
                                                "first is be picked";
    } else {
        qCWarning(lcLottieQtBodymovinParser) << "Failed to find specified effect" << effect;
    }

    // Let users of the json know that it is originated from expression,
    // so they can adjust their behavior accordingly
    retVal.insert(QLatin1String("fromExpression"), true);

    return retVal;
}

QT_END_NAMESPACE
