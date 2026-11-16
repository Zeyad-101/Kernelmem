// AllocationTab.h — First / Best / Worst Fit, release + coalescing, compaction.
#pragma once

#include <QWidget>
#include "../backend/AllocationAlgorithm.h"

class QComboBox;
class QSpinBox;
class QLabel;
class MemoryBar;
class LogView;
class StatTile;

class AllocationTab : public QWidget {
public:
    explicit AllocationTab(QWidget* parent = nullptr);

private:
    void refresh(bool animate = true);
    void allocate();
    void release(int index);
    int  algo() const;   // 1 First, 2 Best, 3 Worst (backend convention)

    MemoryManager m_mm;  // backend, untouched

    QSpinBox*  m_size;
    QComboBox* m_algo;
    MemoryBar* m_bar;
    QLabel*    m_hint;
    StatTile*  m_free;
    StatTile*  m_largest;
    StatTile*  m_frag;
    LogView*   m_log;
};
