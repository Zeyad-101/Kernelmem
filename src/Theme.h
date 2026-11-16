// Theme.h — shared colours, fonts and small widget factories for the GUI.
#pragma once

#include <QColor>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QString>

namespace Theme {

// Palette: a RAM stick on a light bench.
// pcb = solder-mask green of the memory board, gold = edge contacts / highlights.
inline const QColor bg       {"#EEF0EA"};
inline const QColor surface  {"#FFFFFF"};
inline const QColor line     {"#D9DDD4"};
inline const QColor ink      {"#1A221E"};
inline const QColor muted    {"#66706A"};
inline const QColor pcb      {"#10392C"};
inline const QColor pcbLight {"#1A4D3C"};
inline const QColor gold     {"#D2A441"};
inline const QColor accent   {"#1F7A5A"};
inline const QColor fault    {"#C8463D"};
inline const QColor hit      {"#2E8B57"};

QColor procColor(int index);          // stable colour per PID / object index
QFont  mono(int px, bool bold = false);
QFont  ui(int px, int weight = QFont::Normal);
QString styleSheet();

// Small factories so every tab looks the same.
QFrame*      card(QWidget* parent = nullptr);
QLabel*      heading(const QString& text, QWidget* parent = nullptr);
QLabel*      caption(const QString& text, QWidget* parent = nullptr);
QPushButton* button(const QString& text, const QString& kind = QString(), QWidget* parent = nullptr);

} // namespace Theme

// Dark console-style event log used by every tab.
class LogView : public QListWidget {
public:
    explicit LogView(QWidget* parent = nullptr);
    void add(const QString& message, const QColor& tone = Theme::gold);
};

// A labelled big number ("FREE  67 KB").
class StatTile : public QFrame {
public:
    StatTile(const QString& label, QWidget* parent = nullptr);
    void setValue(const QString& value, const QColor& tone = Theme::ink);
private:
    QLabel* m_value;
};
