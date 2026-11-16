// MemoryBar.h — draws physical memory as a RAM stick: processes are chips,
// holes are bare hatched board, gold edge contacts along the bottom.
#pragma once

#include <QHash>
#include <QPair>
#include <QVariantAnimation>
#include <QVector>
#include <QWidget>
#include <functional>

struct Segment {
    QString key;          // stable identity, used to slide blocks during compaction
    QString label;        // "P3", "A", ... (empty for holes)
    int     size = 0;
    bool    free = true;
    QColor  color;
    QString badge;        // gold callout above the block (e.g. "First Fit picks this")
    bool    highlight = false;
    int     mark = 0;     // 0 none, 1 reachable (GC), 2 garbage (GC)
    bool    root = false; // GC root flag
};

class MemoryBar : public QWidget {
    Q_OBJECT
public:
    explicit MemoryBar(QWidget* parent = nullptr);

    void setSegments(const QVector<Segment>& segments, bool animate = true);
    void setArcs(const QVector<QPair<int, int>>& arcs);   // reference arrows (GC)
    void setTopSpace(int px);                             // room above board for badges / arcs
    void setClickable(std::function<bool(int)> fn);
    void setUnit(const QString& unit);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void segmentClicked(int index);

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    static constexpr int kBoardH   = 96;
    static constexpr int kContactH = 12;
    static constexpr int kRulerH   = 20;

    QRectF boardRect() const;
    QVector<QRectF> targetRects() const;
    QVector<QRectF> currentRects() const;
    int hitTest(const QPointF& pos) const;

    QVector<Segment> m_segs;
    QVector<QPair<int, int>> m_arcs;
    QHash<QString, QRectF> m_from;
    double m_t = 1.0;
    QVariantAnimation m_anim;
    int m_hover = -1;
    int m_top = 30;
    std::function<bool(int)> m_clickable;
    QString m_unit = "KB";
};
