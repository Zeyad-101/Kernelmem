// ProcessTab.h — processManagement (activity, termination, idle cleanup)
// wired to its own MemoryManager so terminated processes free their memory.
#pragma once

#include <QHash>
#include <QTimer>
#include <random>
#include <vector>
#include <QWidget>
#include "../backend/AllocationAlgorithm.h"
#include "../backend/ProcessManagementAlgorithm.h"

class QCheckBox;
class QLabel;
class QLineEdit;
class QSpinBox;
class QTableWidget;
class MemoryBar;
class LogView;

class ProcessTab : public QWidget {
public:
    explicit ProcessTab(QWidget* parent = nullptr);

private:
    void bootstrap();
    void resetAll();
    void compact();
    int  allocateRandom(int size);   // returns backend PID or -1
    void addProcess();
    void terminate(const QString& name);
    void touch(const QString& name);
    void runCleanup(bool manual);
    void refreshMemory(bool animate = true);
    void rebuildTable();
    void tick();
    int  runningIndex(const QString& name) const;

    processManagement m_pm;          // backend, untouched
    MemoryManager     m_mm;          // backend, untouched (separate instance from the Allocation tab)
    struct Record { int pid; int memPid; };   // parallel to m_pm.processes
    std::vector<Record> m_rec;
    int m_nextPid = 1;
    std::mt19937 m_rng{std::random_device{}()};

    QLineEdit*    m_name;
    QSpinBox*     m_size;
    QLabel*       m_error;
    MemoryBar*    m_bar;
    QTableWidget* m_table;
    QCheckBox*    m_auto;
    QSpinBox*     m_idleMin;
    LogView*      m_log;
    QTimer        m_timer;
};
