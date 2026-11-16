#include "PageTab.h"
#include "Theme.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
constexpr int kLabelW  = 76;
constexpr int kCellW   = 54;
constexpr int kCellH   = 46;
constexpr int kHeaderH = 34;
constexpr int kFooterH = 28;
constexpr int kGap     = 4;
} // namespace

// ============================================================================
// PageGrid
// ============================================================================
PageGrid::PageGrid(QWidget* parent) : QWidget(parent) {}

void PageGrid::setData(const std::vector<int>& pages, int frames, const std::vector<PageStepRecord>& history)
{
    m_pages = pages;
    m_frames = frames;
    m_hist = history;
    setFixedSize(sizeHint());
    update();
}

QSize PageGrid::sizeHint() const
{
    const int cols = std::max<int>(1, int(m_pages.size()));
    return QSize(kLabelW + cols * (kCellW + kGap) + 8,
                 kHeaderH + m_frames * (kCellH + kGap) + kFooterH + 8);
}

void PageGrid::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int cols = int(m_pages.size());
    const int done = int(m_hist.size());
    const int current = done - 1;

    auto colX = [](int c) { return kLabelW + c * (kCellW + kGap); };
    auto rowY = [](int r) { return kHeaderH + r * (kCellH + kGap); };

    // current column band
    if (current >= 0) {
        QRectF band(colX(current) - 2, 0, kCellW + 4, height() - 2);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(210, 164, 65, 46));
        p.drawRoundedRect(band, 8, 8);
    }

    // row labels
    p.setFont(Theme::ui(12, QFont::DemiBold));
    p.setPen(Theme::muted);
    for (int r = 0; r < m_frames; ++r)
        p.drawText(QRectF(0, rowY(r), kLabelW - 10, kCellH), Qt::AlignVCenter | Qt::AlignRight,
                   QString("Frame %1").arg(r + 1));
    p.drawText(QRectF(0, 0, kLabelW - 10, kHeaderH - 6), Qt::AlignVCenter | Qt::AlignRight, "Page");

    for (int c = 0; c < cols; ++c) {
        const bool past = c < done;

        // header: the page being referenced
        p.setFont(Theme::mono(15, true));
        p.setPen(past ? Theme::ink : QColor("#B4BBB5"));
        p.drawText(QRectF(colX(c), 0, kCellW, kHeaderH - 6), Qt::AlignCenter, QString::number(m_pages[c]));

        for (int r = 0; r < m_frames; ++r) {
            QRectF cell(colX(c), rowY(r), kCellW, kCellH);
            if (!past) {
                p.setPen(QPen(Theme::line, 1, Qt::DashLine));
                p.setBrush(Qt::NoBrush);
                p.drawRoundedRect(cell.adjusted(0.5, 0.5, -0.5, -0.5), 6, 6);
                continue;
            }
            const PageStepRecord& rec = m_hist[c];
            const int value = (r < int(rec.frames.size())) ? rec.frames[r] : -1;
            const bool replaced = (!rec.isHit && rec.replacedFrame == r);
            const bool hitCell  = (rec.isHit && value == rec.page);

            QColor fill = Theme::surface, border = Theme::line, text = Theme::ink;
            double bw = 1;
            if (replaced) { fill = Theme::accent; border = Theme::fault; text = Qt::white; bw = 2.5; }
            else if (hitCell) { fill = QColor(46, 139, 87, 40); border = Theme::hit; bw = 2.5; }
            else if (value == -1) { fill = QColor("#F6F7F4"); }

            p.setPen(QPen(border, bw));
            p.setBrush(fill);
            p.drawRoundedRect(cell.adjusted(bw / 2, bw / 2, -bw / 2, -bw / 2), 6, 6);

            if (value != -1) {
                p.setPen(text);
                p.setFont(Theme::mono(15, replaced || hitCell));
                p.drawText(cell, Qt::AlignCenter, QString::number(value));
            }
            // Clock reference bit
            if (!rec.refBits.empty() && value != -1 && r < int(rec.refBits.size())) {
                p.setFont(Theme::mono(9, true));
                p.setPen(replaced ? QColor(255, 255, 255, 200) : (rec.refBits[r] ? Theme::gold.darker(120) : Theme::muted));
                p.drawText(cell.adjusted(0, 2, -5, 0), Qt::AlignTop | Qt::AlignRight, QString::number(rec.refBits[r]));
            }
        }

        if (past) {
            const bool isHit = m_hist[c].isHit;
            p.setFont(Theme::ui(11, QFont::Bold));
            p.setPen(isHit ? Theme::hit : Theme::fault);
            p.drawText(QRectF(colX(c) - 4, rowY(m_frames), kCellW + 8, kFooterH), Qt::AlignCenter,
                       isHit ? "Hit" : "Fault");
        }
    }
}

// ============================================================================
// CompareChart
// ============================================================================
CompareChart::CompareChart(QWidget* parent) : QWidget(parent)
{
    setMinimumHeight(112);
}

void CompareChart::setData(const QVector<QPair<QString, int>>& faults, int total, const QString& current)
{
    m_data = faults;
    m_total = std::max(1, total);
    m_current = current;
    update();
}

void CompareChart::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const int rowH = 30, labelW = 56, valueW = 84;
    int best = INT_MAX;
    for (const auto& d : m_data) best = std::min(best, d.second);

    for (int i = 0; i < m_data.size(); ++i) {
        const auto& d = m_data[i];
        const double y = 6 + i * (rowH + 6);
        const bool cur = (d.first == m_current);

        p.setFont(Theme::ui(12, cur ? QFont::Bold : QFont::DemiBold));
        p.setPen(cur ? Theme::ink : Theme::muted);
        p.drawText(QRectF(0, y, labelW, rowH), Qt::AlignVCenter | Qt::AlignLeft, d.first);

        const QRectF track(labelW, y + 8, width() - labelW - valueW, rowH - 16);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#EEF0EA"));
        p.drawRoundedRect(track, 7, 7);
        QRectF bar = track;
        bar.setWidth(track.width() * d.second / m_total);
        p.setBrush(cur ? Theme::accent : QColor("#9DBDB0"));
        if (bar.width() > 0) p.drawRoundedRect(bar, 7, 7);

        p.setFont(Theme::mono(12, cur));
        p.setPen(cur ? Theme::ink : Theme::muted);
        QString v = QString("%1 faults").arg(d.second);
        p.drawText(QRectF(width() - valueW + 8, y, valueW - 8, rowH), Qt::AlignVCenter | Qt::AlignLeft, v);
        if (d.second == best) {
            p.setBrush(Theme::gold);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(labelW - 10, y + rowH / 2.0), 4, 4);
        }
    }
}

// ============================================================================
// PageTab
// ============================================================================
PageTab::PageTab(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 16, 0, 0);
    root->setSpacing(14);

    // --- Controls -----------------------------------------------------------------
    auto* controls = Theme::card();
    auto* cl = new QHBoxLayout(controls);
    cl->setContentsMargins(18, 14, 18, 14);
    cl->setSpacing(10);

    m_refs = new QLineEdit("7 0 1 2 0 3 0 4 2 3 0 3 2");
    m_refs->setFont(Theme::mono(13));
    m_refs->setPlaceholderText("e.g. 7 0 1 2 0 3 0 4");
    m_frames = new QSpinBox;
    m_frames->setRange(1, 8);
    m_frames->setValue(3);
    m_algo = new QComboBox;
    m_algo->addItems({"FIFO", "LRU", "Clock"});
    m_stepBtn = Theme::button("Step", "primary");
    m_runBtn  = Theme::button("Run all");
    auto* resetBtn = Theme::button("Reset");

    cl->addWidget(Theme::caption("Reference string"));
    cl->addWidget(m_refs, 1);
    cl->addWidget(Theme::caption("Frames"));
    cl->addWidget(m_frames);
    cl->addWidget(Theme::caption("Algorithm"));
    cl->addWidget(m_algo);
    cl->addSpacing(6);
    cl->addWidget(m_stepBtn);
    cl->addWidget(m_runBtn);
    cl->addWidget(resetBtn);
    root->addWidget(controls);

    m_error = new QLabel;
    m_error->setObjectName("error");
    m_error->hide();
    root->addWidget(m_error);

    // --- Grid -------------------------------------------------------------------
    auto* gridCard = Theme::card();
    auto* gl = new QVBoxLayout(gridCard);
    gl->setContentsMargins(18, 14, 18, 14);
    gl->setSpacing(8);
    auto* head = new QHBoxLayout;
    head->addWidget(Theme::heading("Frames over time"));
    head->addStretch();
    head->addWidget(Theme::caption("Green cell: page already loaded (hit). Red border: frame chosen as victim. "
                                   "Small digit in Clock: reference bit."));
    gl->addLayout(head);

    m_grid = new PageGrid;
    auto* scroll = new QScrollArea;
    scroll->setWidget(m_grid);
    scroll->setWidgetResizable(false);
    scroll->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setMinimumHeight(260);
    scroll->viewport()->setAutoFillBackground(false);
    scroll->setStyleSheet("QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; }");
    gl->addWidget(scroll, 1);
    root->addWidget(gridCard, 1);

    // --- Stats + compare --------------------------------------------------------
    auto* bottom = new QHBoxLayout;
    bottom->setSpacing(14);

    auto* statCard = Theme::card();
    auto* sl = new QHBoxLayout(statCard);
    sl->setContentsMargins(18, 16, 18, 16);
    sl->setSpacing(28);
    m_step   = new StatTile("Step");
    m_hits   = new StatTile("Hits");
    m_faults = new StatTile("Faults");
    m_ratio  = new StatTile("Hit ratio");
    sl->addWidget(m_step);
    sl->addWidget(m_hits);
    sl->addWidget(m_faults);
    sl->addWidget(m_ratio);
    sl->addStretch();
    bottom->addWidget(statCard, 3);

    auto* cmpCard = Theme::card();
    auto* cml = new QVBoxLayout(cmpCard);
    cml->setContentsMargins(18, 14, 18, 14);
    cml->setSpacing(4);
    cml->addWidget(Theme::heading("Same string, every algorithm"));
    m_compare = new CompareChart;
    cml->addWidget(m_compare);
    bottom->addWidget(cmpCard, 2);
    root->addLayout(bottom);

    // --- Wiring -----------------------------------------------------------------
    connect(m_stepBtn, &QPushButton::clicked, this, [this] { stepOnce(); });
    connect(m_runBtn, &QPushButton::clicked, this, [this] { runAll(); });
    connect(resetBtn, &QPushButton::clicked, this, [this] { restart(); });
    connect(m_refs, &QLineEdit::editingFinished, this, [this] { restart(); });
    connect(m_frames, QOverload<int>::of(&QSpinBox::valueChanged), this, [this] { restart(); });
    connect(m_algo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { restart(); });

    restart();
}

std::vector<int> PageTab::parsePages() const
{
    std::vector<int> pages;
    const QString text = m_refs->text().trimmed();
    // "7 0 1 2" / "7,0,1,2" -> tokens; "70120" (no separators) -> one page per digit
    const bool hasSep = text.contains(QRegularExpression("[\\s,;]"));
    if (hasSep) {
        for (const QString& t : text.split(QRegularExpression("[\\s,;]+"), Qt::SkipEmptyParts)) {
            bool ok = false;
            const int v = t.toInt(&ok);
            if (!ok || v < 0) return {};
            pages.push_back(v);
        }
    } else {
        for (QChar ch : text) {
            if (!ch.isDigit()) return {};
            pages.push_back(ch.digitValue());
        }
    }
    return pages;
}

void PageTab::restart()
{
    m_pages = parsePages();
    if (m_pages.empty()) {
        m_error->setText("Enter page numbers (0 or more) separated by spaces or commas, e.g. 7 0 1 2 0 3.");
        m_error->show();
    } else {
        m_error->hide();
    }
    m_sim.init(m_frames->value(), m_algo->currentText().toStdString());
    updateCompare();
    refresh();
}

void PageTab::stepOnce()
{
    const size_t done = m_sim.getHistory().size();
    if (done < m_pages.size()) m_sim.step(m_pages[done]);
    refresh();
}

void PageTab::runAll()
{
    const size_t done = m_sim.getHistory().size();
    for (size_t i = done; i < m_pages.size(); ++i) m_sim.step(m_pages[i]);
    refresh();
}

void PageTab::refresh()
{
    const auto& hist = m_sim.getHistory();
    m_grid->setData(m_pages, m_sim.getFrameCount(), hist);

    const int done = int(hist.size());
    const int hits = m_sim.getHitCount(), faults = m_sim.getFaultCount();
    m_step->setValue(QString("%1/%2").arg(done).arg(m_pages.size()));
    m_hits->setValue(QString::number(hits), Theme::hit);
    m_faults->setValue(QString::number(faults), Theme::fault);
    m_ratio->setValue(done ? QString("%1%").arg(qRound(100.0 * hits / done)) : "—");

    const bool finished = done >= int(m_pages.size());
    m_stepBtn->setEnabled(!finished);
    m_runBtn->setEnabled(!finished);
}

void PageTab::updateCompare()
{
    QVector<QPair<QString, int>> data;
    for (const char* name : {"FIFO", "LRU", "Clock"}) {
        PageReplacementSim sim;   // a separate backend instance per algorithm
        sim.init(m_frames->value(), name);
        sim.runAll(m_pages);
        data.push_back({QString(name), sim.getFaultCount()});
    }
    m_compare->setData(data, std::max<int>(1, int(m_pages.size())), m_algo->currentText());
}
