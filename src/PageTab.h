// PageTab.h — FIFO / LRU / Clock page replacement, step-by-step frame table.
#pragma once

#include <QWidget>
#include <vector>
#include "../backend/PageReplacementAlgorithm.h"

class QComboBox;
class QSpinBox;
class QLineEdit;
class QLabel;
class QPushButton;
class LogView;
class StatTile;

// Painted frame table: one column per reference, one row per frame.
class PageGrid : public QWidget {
public:
    explicit PageGrid(QWidget* parent = nullptr);
    void setData(const std::vector<int>& pages, int frames, const std::vector<PageStepRecord>& history);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::vector<int> m_pages;
    std::vector<PageStepRecord> m_hist;
    int m_frames = 3;
};

// Fault count of all three algorithms on the same input.
class CompareChart : public QWidget {
public:
    explicit CompareChart(QWidget* parent = nullptr);
    void setData(const QVector<QPair<QString, int>>& faults, int total, const QString& current);
    QSize sizeHint() const override { return QSize(320, 112); }
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QVector<QPair<QString, int>> m_data;
    int m_total = 1;
    QString m_current;
};

class PageTab : public QWidget {
public:
    explicit PageTab(QWidget* parent = nullptr);

private:
    void restart();
    void stepOnce();
    void runAll();
    void refresh();
    void updateCompare();
    std::vector<int> parsePages() const;

    PageReplacementSim m_sim;   // backend, untouched
    std::vector<int> m_pages;

    QLineEdit*   m_refs;
    QSpinBox*    m_frames;
    QComboBox*   m_algo;
    QPushButton* m_stepBtn;
    QPushButton* m_runBtn;
    PageGrid*    m_grid;
    StatTile*    m_step;
    StatTile*    m_hits;
    StatTile*    m_faults;
    StatTile*    m_ratio;
    CompareChart* m_compare;
    QLabel*      m_error;
};
