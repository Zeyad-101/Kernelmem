// GcTab.h — mark & sweep garbage collector on the memoryManger backend.
#pragma once

#include <QStringList>
#include <QWidget>
#include "../backend/MemoryManagementAlgorithm.h"

class QComboBox;
class QSpinBox;
class QPushButton;
class QLabel;
class MemoryBar;
class LogView;

class GcTab : public QWidget {
public:
    explicit GcTab(QWidget* parent = nullptr);

private:
    void loadPreset();
    void refresh(bool animate = true);
    void invalidateMarks();
    void newObject();
    void logBackend(const QStringList& lines, const QColor& tone);
    QStringList liveNames() const;

    memoryManger m_gc;          // backend, untouched
    bool m_marked = false;      // true between "Mark" and "Sweep"
    int  m_nextName = 0;

    MemoryBar*   m_bar;
    QComboBox*   m_from;
    QComboBox*   m_to;
    QComboBox*   m_rootPick;
    QSpinBox*    m_newSize;
    QPushButton* m_sweepBtn;
    QLabel*      m_phase;
    LogView*     m_log;
};
