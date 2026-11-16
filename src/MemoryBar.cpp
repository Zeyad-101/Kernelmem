#include "MemoryBar.h"
#include "Theme.h"

#include <QEasingCurve>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>
#include <cmath>

namespace {
QPointF eventPos(QMouseEvent* e)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return e->position();
#else
    return e->localPos();
#endif
}

QRectF lerp(const QRectF& a, const QRectF& b, double t)
{
    return QRectF(a.x() + (b.x() - a.x()) * t, a.y() + (b.y() - a.y()) * t,
                  a.width() + (b.width() - a.width()) * t, a.height() + (b.height() - a.height()) * t);
}
} // namespace

MemoryBar::MemoryBar(QWidget* parent) : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_anim.setStartValue(0.0);
    m_anim.setEndValue(1.0);
    m_anim.setDuration(420);
    connect(&m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_t = QEasingCurve(QEasingCurve::OutCubic).valueForProgress(v.toDouble());
        update();
    });
}

void MemoryBar::setSegments(const QVector<Segment>& segments, bool animate)
{
    if (animate && !m_segs.isEmpty() && isVisible()) {
        const QVector<QRectF> cur = currentRects();
        m_from.clear();
        for (int i = 0; i < m_segs.size(); ++i)
            if (!m_segs[i].key.isEmpty())
                m_from.insert(m_segs[i].key, cur[i]);
        m_segs = segments;
        m_t = 0.0;
        m_anim.stop();
        m_anim.start();
    } else {
        m_segs = segments;
        m_t = 1.0;
    }
    m_hover = -1;
    update();
}

void MemoryBar::setArcs(const QVector<QPair<int, int>>& arcs) { m_arcs = arcs; update(); }
void MemoryBar::setTopSpace(int px) { m_top = px; updateGeometry(); update(); }
void MemoryBar::setClickable(std::function<bool(int)> fn) { m_clickable = std::move(fn); }
void MemoryBar::setUnit(const QString& unit) { m_unit = unit; update(); }

QSize MemoryBar::sizeHint() const { return QSize(900, m_top + kBoardH + kContactH + kRulerH); }
QSize MemoryBar::minimumSizeHint() const { return QSize(420, m_top + kBoardH + kContactH + kRulerH); }

QRectF MemoryBar::boardRect() const
{
    return QRectF(1, m_top, width() - 2, kBoardH);
}

QVector<QRectF> MemoryBar::targetRects() const
{
    QVector<QRectF> rects;
    const QRectF inner = boardRect().adjusted(12, 14, -12, -14);
    int total = 0;
    for (const auto& s : m_segs) total += s.size;
    if (total <= 0) return rects;

    const double gap = 4.0;
    const double avail = inner.width() - gap * (m_segs.size() - 1);
    double x = inner.left();
    for (const auto& s : m_segs) {
        const double w = std::max(2.0, avail * s.size / total);
        rects.push_back(QRectF(x, inner.top(), w, inner.height()));
        x += w + gap;
    }
    return rects;
}

QVector<QRectF> MemoryBar::currentRects() const
{
    QVector<QRectF> target = targetRects();
    if (m_t >= 1.0) return target;
    for (int i = 0; i < target.size(); ++i) {
        const QRectF& t = target[i];
        QRectF from(t.center().x(), t.top(), 0, t.height());
        if (!m_segs[i].key.isEmpty() && m_from.contains(m_segs[i].key))
            from = m_from.value(m_segs[i].key);
        target[i] = lerp(from, t, m_t);
    }
    return target;
}

int MemoryBar::hitTest(const QPointF& pos) const
{
    const QVector<QRectF> rects = currentRects();
    for (int i = 0; i < rects.size(); ++i)
        if (rects[i].adjusted(-2, -6, 2, 6).contains(pos))
            return i;
    return -1;
}

void MemoryBar::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF board = boardRect();

    // --- Board with edge-connector tab and key notch --------------------------------
    QPainterPath body;
    body.addRoundedRect(board, 10, 10);
    QRectF tab(board.left() + 8, board.bottom() - 4, board.width() - 16, kContactH + 4);
    body.addRoundedRect(tab, 4, 4);
    body.setFillRule(Qt::WindingFill);
    p.fillPath(body.simplified(), Theme::pcb);

    // faint copper traces
    p.setPen(QPen(QColor(255, 255, 255, 14), 1));
    for (int k = 1; k <= 3; ++k) {
        const double y = board.top() + 6 + k * (board.height() - 12) / 4.0;
        p.drawLine(QPointF(board.left() + 6, y), QPointF(board.right() - 6, y));
    }

    // gold contacts
    const double notchX = board.left() + board.width() * 0.38;
    p.setPen(Qt::NoPen);
    p.setBrush(Theme::gold);
    for (double x = tab.left() + 8; x < tab.right() - 8; x += 9) {
        if (std::abs(x - notchX) < 10) continue;
        p.drawRoundedRect(QRectF(x, board.bottom() + 1, 5, kContactH - 3), 1.2, 1.2);
    }
    p.setBrush(Theme::surface);
    p.drawRoundedRect(QRectF(notchX - 5, board.bottom() + 2, 10, kContactH + 4), 3, 3);

    if (m_segs.isEmpty()) return;

    const QVector<QRectF> rects = currentRects();

    // --- Address ruler ----------------------------------------------------------------
    {
        p.setFont(Theme::mono(10));
        p.setPen(Theme::muted);
        const double y = board.bottom() + kContactH + 14;
        int addr = 0;
        double lastX = -1e9;
        const QVector<QRectF> tgt = targetRects();
        for (int i = 0; i <= tgt.size(); ++i) {
            const double x = (i < tgt.size()) ? tgt[i].left() : tgt.last().right();
            const QString txt = QString::number(addr);
            const double w = p.fontMetrics().horizontalAdvance(txt);
            const bool last = (i == tgt.size());
            if (x - lastX > w + 10 || last) {
                double tx = x - w / 2;
                if (last) tx = std::min(tx, double(width()) - w - 2);
                tx = std::max(tx, 0.0);
                p.drawText(QPointF(tx, y), txt);
                lastX = x;
            }
            if (i < m_segs.size()) addr += m_segs[i].size;
        }
    }

    // --- Blocks -----------------------------------------------------------------------
    for (int i = 0; i < m_segs.size(); ++i) {
        const Segment& s = m_segs[i];
        const QRectF r = rects[i];
        if (r.width() < 1) continue;
        const bool hover = (i == m_hover);

        if (s.free) {
            p.setPen(Qt::NoPen);
            p.setBrush(hover ? Theme::pcbLight.lighter(118) : Theme::pcbLight);
            p.drawRoundedRect(r, 5, 5);
            p.setBrush(QBrush(QColor(255, 255, 255, 26), Qt::BDiagPattern));
            p.drawRoundedRect(r, 5, 5);
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor(255, 255, 255, 70), 1, Qt::DashLine));
            p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 5, 5);

            if (r.width() > 22) {
                p.setPen(QColor("#CFE3D8"));
                p.setFont(Theme::mono(12, true));
                p.drawText(r, Qt::AlignCenter, QString::number(s.size));
            }
        } else {
            QColor fill = s.color;
            if (hover) fill = fill.lighter(112);

            // chip legs
            if (r.width() > 24) {
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(210, 164, 65, 150));
                for (double x = r.left() + 6; x < r.right() - 6; x += 8) {
                    p.drawRect(QRectF(x, r.top() - 4, 3, 4));
                    p.drawRect(QRectF(x, r.bottom(), 3, 4));
                }
            }
            p.setPen(QPen(fill.darker(135), 1));
            p.setBrush(fill);
            p.drawRoundedRect(r, 5, 5);
            p.setPen(QPen(QColor(255, 255, 255, 70), 1));
            p.drawLine(QPointF(r.left() + 5, r.top() + 1.5), QPointF(r.right() - 5, r.top() + 1.5));

            if (r.width() > 18) {
                const QColor text = fill.lightness() > 150 ? Theme::ink : QColor("#FFFFFF");
                p.setPen(text);
                p.setFont(Theme::ui(13, QFont::Bold));
                QFontMetrics fm(p.font());
                const QString name = fm.elidedText(s.label, Qt::ElideRight, int(r.width() - 6));
                p.drawText(QRectF(r.left(), r.center().y() - 17, r.width(), 18), Qt::AlignCenter, name);
                p.setFont(Theme::mono(11));
                p.drawText(QRectF(r.left(), r.center().y() + 1, r.width(), 16), Qt::AlignCenter,
                           QString::number(s.size));
            }

            if (s.mark == 2) { // garbage
                p.setBrush(QColor(16, 57, 44, 130));
                p.setPen(QPen(Theme::fault, 2, Qt::DashLine));
                p.drawRoundedRect(r, 5, 5);
                p.setPen(QColor("#FFD9D5"));
                p.setFont(Theme::ui(9, QFont::Bold));
                p.drawText(r.adjusted(0, 0, 0, -4), Qt::AlignHCenter | Qt::AlignBottom, "GARBAGE");
            }
        }

        if (s.mark == 1 || s.highlight) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(Theme::gold, 2.5));
            p.drawRoundedRect(r.adjusted(-3, -3, 3, 3), 7, 7);
        }

        if (s.root) {
            p.setFont(Theme::ui(9, QFont::Bold));
            const QString t = "ROOT";
            const double w = p.fontMetrics().horizontalAdvance(t) + 10;
            QRectF tag(r.left() + 3, r.top() - 9, w, 14);
            p.setPen(Qt::NoPen);
            p.setBrush(Theme::gold);
            p.drawRoundedRect(tag, 4, 4);
            p.setPen(Theme::pcb);
            p.drawText(tag, Qt::AlignCenter, t);
        }
    }

    // --- Reference arcs (GC) ------------------------------------------------------------
    for (const auto& arc : m_arcs) {
        if (arc.first < 0 || arc.second < 0 || arc.first >= rects.size() || arc.second >= rects.size()) continue;
        if (arc.first == arc.second) continue;
        const QRectF a = rects[arc.first], b = rects[arc.second];
        const double dir = (b.center().x() > a.center().x()) ? 1 : -1;
        const QPointF p0(a.center().x() + dir * std::min(10.0, a.width() / 4), board.top() + 2);
        const QPointF p1(b.center().x() - dir * std::min(10.0, b.width() / 4), board.top() + 2);
        double h = std::min(double(m_top - 8), 24 + std::abs(p1.x() - p0.x()) * 0.28);
        if (dir < 0) h *= 0.55;   // leftward arcs sit lower so A->B and B->A never overlap
        QPainterPath path(p0);
        path.cubicTo(p0.x(), p0.y() - h, p1.x(), p1.y() - h, p1.x(), p1.y() - 3);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Theme::accent, 2, Qt::SolidLine, Qt::RoundCap));
        p.drawPath(path);
        QPolygonF head;
        head << QPointF(p1.x(), p1.y()) << QPointF(p1.x() - 5, p1.y() - 9) << QPointF(p1.x() + 5, p1.y() - 9);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::accent);
        p.drawPolygon(head);
    }

    // --- Badges (algorithm pick, etc.) ---------------------------------------------------
    for (int i = 0; i < m_segs.size(); ++i) {
        if (m_segs[i].badge.isEmpty()) continue;
        const QRectF r = rects[i];
        p.setFont(Theme::ui(11, QFont::Bold));
        const double w = p.fontMetrics().horizontalAdvance(m_segs[i].badge) + 18;
        double x = r.center().x() - w / 2;
        x = std::max(2.0, std::min(x, width() - w - 2));
        QRectF pill(x, board.top() - 28, w, 20);
        p.setPen(Qt::NoPen);
        p.setBrush(Theme::gold);
        p.drawRoundedRect(pill, 10, 10);
        QPolygonF tip;
        tip << QPointF(r.center().x() - 5, pill.bottom() - 1) << QPointF(r.center().x() + 5, pill.bottom() - 1)
            << QPointF(r.center().x(), pill.bottom() + 5);
        p.drawPolygon(tip);
        p.setPen(Theme::pcb);
        p.drawText(pill, Qt::AlignCenter, m_segs[i].badge);
    }
}

void MemoryBar::mouseMoveEvent(QMouseEvent* e)
{
    const int idx = hitTest(eventPos(e));
    if (idx != m_hover) {
        m_hover = idx;
        update();
    }
    const bool canClick = idx >= 0 && m_clickable && m_clickable(idx);
    setCursor(canClick ? Qt::PointingHandCursor : Qt::ArrowCursor);

    if (idx >= 0) {
        int start = 0;
        for (int i = 0; i < idx; ++i) start += m_segs[i].size;
        const Segment& s = m_segs[idx];
        const QString what = s.free ? QString("Free hole") : s.label;
        QString tip = QString("<b>%1</b><br>%2 %3 &nbsp;·&nbsp; %4 – %5")
                          .arg(what.toHtmlEscaped()).arg(s.size).arg(m_unit).arg(start).arg(start + s.size);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QToolTip::showText(e->globalPosition().toPoint(), tip, this);
#else
        QToolTip::showText(e->globalPos(), tip, this);
#endif
    } else {
        QToolTip::hideText();
    }
}

void MemoryBar::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) return;
    const int idx = hitTest(eventPos(e));
    if (idx >= 0 && (!m_clickable || m_clickable(idx)))
        emit segmentClicked(idx);
}

void MemoryBar::leaveEvent(QEvent*)
{
    m_hover = -1;
    update();
}
