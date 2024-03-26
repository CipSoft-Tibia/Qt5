// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QApplication>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QDebug>
#include <QtVersion>

class ShortcutTester : public QWidget
{
public:
    ShortcutTester()
    {
        const QString title = QLatin1String(QT_VERSION_STR) + QLatin1Char(' ')
            + qApp->platformName();
        setWindowTitle(title);
        setupLayout();
    }

private:
    void setupLayout();
    void addToGrid(QWidget *w, int &row, int col);
    void addShortcutToGrid(const QKeySequence &k, int &row, int col);

   QGridLayout *m_gridLayout = new QGridLayout;
};

inline void ShortcutTester::addToGrid(QWidget *w, int &row, int col)
{
    m_gridLayout->addWidget(w, row++, col);
}

void ShortcutTester::addShortcutToGrid(const QKeySequence &k, int &row, int col)
{
    QPushButton *button = new QPushButton(k.toString());
    button->setShortcut(k);
    addToGrid(button, row, col);
}

void addShortcutToGrid(int key, int &row, int col);

void ShortcutTester::setupLayout()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *testPurpose = new QLabel();
    testPurpose->setWordWrap(true);
    testPurpose->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);
    testPurpose->setText("This test come in handy to verify shortcuts under different "
                         "keyboard layouts - qwerty, dvorak, non-latin (russian, arabic), etc.");
    layout->addWidget(testPurpose);

    layout->addLayout(m_gridLayout);

    int row = 0;
    int col = 0;

    const QKeyCombination keys1[] = {
        Qt::AltModifier | Qt::ShiftModifier | Qt::Key_G,
        Qt::AltModifier | Qt::Key_G,
        Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_R,
        Qt::ControlModifier | Qt::Key_R,
        Qt::ControlModifier | Qt::Key_Return, Qt::ControlModifier | Qt::Key_Enter,
        Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::Key_R,
        Qt::ShiftModifier | Qt::Key_5, Qt::ShiftModifier | Qt::Key_Percent,
        Qt::Key_Percent, Qt::Key_5, Qt::Key_Q
     };

    for (const auto k : keys1)
        addShortcutToGrid(k, row, col);

    row = 0;
    col++;

    const QKeyCombination keys2[] = {
        Qt::ControlModifier | Qt::Key_Percent,
        Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_5,
        Qt::ControlModifier | Qt::Key_5, Qt::AltModifier | Qt::Key_5,
        Qt::ControlModifier | Qt::Key_Plus,
        Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Plus,
        Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_Equal,
        Qt::ControlModifier | Qt::Key_Y, Qt::ShiftModifier | Qt::Key_Comma,
        Qt::ControlModifier | Qt::Key_Comma, Qt::ControlModifier | Qt::Key_Slash,
        Qt::ControlModifier | Qt::Key_Backslash
    };

    for (const auto k : keys2)
        addShortcutToGrid(k, row, col);

    row = 0;
    col++;

    const QKeyCombination keys3[] = {
        Qt::MetaModifier | Qt::ShiftModifier | Qt::Key_A,
        Qt::MetaModifier | Qt::Key_A,
        Qt::MetaModifier | Qt::Key_Q,
        Qt::MetaModifier | Qt::ShiftModifier | Qt::Key_5,
        Qt::ControlModifier | Qt::Key_BracketRight,
        Qt::ShiftModifier | Qt::Key_F3,
        Qt::ControlModifier | Qt::Key_F3,
    };

    for (const auto k : keys3)
        addShortcutToGrid(k, row, col);

     addShortcutToGrid(0x20AC, row, col); // EURO SIGN e.g. US (with euro on 5) on 3rd keyboard level
     addShortcutToGrid(int(Qt::ControlModifier) | 0x20AC, row, col);

    // with german (neo 2) layout on linux under ISO_Level3_Shift + ISO_Level5_Shift + I
    const QKeySequence greekPsi(QString(QStringLiteral("\u03A8")));
    addShortcutToGrid(greekPsi, row, col);

    row = 0;
    col++;

    addToGrid(new QLabel("Norwegian layout"), row, col);
    // LATIN SMALL LETTER O WITH STROKE
    QKeySequence norwegianO(QString(QStringLiteral("\u00F8")));
    addShortcutToGrid(norwegianO, row, col);

    addToGrid(new QLabel("Russian layout"), row, col);
    // CYRILLIC SMALL LETTER ZHE
    QKeySequence zhe(QString(QStringLiteral("\u0436")));
    addShortcutToGrid(zhe, row, col);

    // for sequence definitons see qplatformtheme.cpp
    addToGrid(new QLabel("QKeySequence::StandardKey(s)"), row, col);
    addShortcutToGrid(QKeySequence(QKeySequence::Open), row, col); // Qt::CTRL | Qt::Key_O
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << qVersion();

    ShortcutTester tester;
    tester.show();

    return a.exec();
}
