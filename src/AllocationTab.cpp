#include "AllocationTab.h"
#include "MemoryBar.h"
#include "Theme.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

AllocationTab::AllocationTab(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 16, 0, 0);
    root->setSpacing(14);

    // --- Controls -----------------------------------------------------------------
    auto* controls = Theme::card();
    auto* cl = new QHBoxLayout(controls);
    cl->setContentsMargins(18, 14, 18, 14);
    cl->setSpacing(10);

    m_size = new QSpinBox;
    m_size->setRange(1, 999);
    m_size->setValue(20);
    m_size->setSuffix(" KB");
    m_size->setMinimumWidth(100);

    m_algo = new QComboBox;
    m_algo->addItems({"First Fit", "Best Fit", "Worst Fit"});
    m_algo->setMinimumWidth(120);

    auto* allocBtn   = Theme::button("Allocate", "primary");
    auto* compactBtn = Theme::button("Compact memory");
    auto* resetBtn   = Theme::button("Reset");

    auto* sizeLbl = Theme::caption("Request");
    auto* algoLbl = Theme::caption("Algorithm");
    cl->addWidget(sizeLbl);
    cl->addWidget(m_size);
    cl->addSpacing(6);
    cl->addWidget(algoLbl);
    cl->addWidget(m_algo);
    cl->addWidget(allocBtn);
    cl->addStretch();
    cl->addWidget(compactBtn);
    cl->addWidget(resetBtn);
    root->addWidget(controls);

    // --- Memory map -------------------------------------------------------------
    auto* mapCard = Theme::card();
    auto* ml = new QVBoxLayout(mapCard);
    ml->setContentsMargins(18, 14, 18, 14);
    ml->setSpacing(8);
    auto* head = new QHBoxLayout;
    head->addWidget(Theme::heading("Physical memory"));
    head->addStretch();
    head->addWidget(Theme::caption("Click a process to free it. Neighbouring holes merge on release."));
    ml->addLayout(head);

    m_bar = new MemoryBar;
    m_bar->setTopSpace(36);
    m_bar->setClickable([this](int i) { return i < int(m_mm.getBlocks().size()) && m_mm.getBlocks()[i].pid != -1; });
    ml->addWidget(m_bar);

    m_hint = Theme::caption("");
    ml->addWidget(m_hint);

    auto* stats = new QHBoxLayout;
    stats->setSpacing(28);
    m_free    = new StatTile("Free");
    m_largest = new StatTile("Largest hole");
    m_frag    = new StatTile("Fragmentation");
    stats->addWidget(m_free);
    stats->addWidget(m_largest);
    stats->addWidget(m_frag);
    stats->addStretch();
    ml->addLayout(stats);
    root->addWidget(mapCard);

    // --- Log --------------------------------------------------------------------
    auto* logCard = Theme::card();
    auto* ll = new QVBoxLayout(logCard);
    ll->setContentsMargins(18, 14, 18, 14);
    ll->addWidget(Theme::heading("Activity"));
    m_log = new LogView;
    ll->addWidget(m_log);
    root->addWidget(logCard, 1);

    // --- Wiring -----------------------------------------------------------------
    connect(allocBtn, &QPushButton::clicked, this, [this] { allocate(); });
    connect(m_size, QOverload<int>::of(&QSpinBox::valueChanged), this, [this] { refresh(false); });
    connect(m_algo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { refresh(false); });
    connect(m_bar, &MemoryBar::segmentClicked, this, [this](int i) { release(i); });
    connect(compactBtn, &QPushButton::clicked, this, [this] {
        m_mm.compact();
        m_log->add("Compaction: processes moved to the start, free space merged into one hole.", Theme::gold);
        refresh();
    });
    connect(resetBtn, &QPushButton::clicked, this, [this] {
        m_mm.loadDefaultPreset();
        m_log->add("Memory reset to the default fragmented layout.", Theme::muted);
        refresh();
    });

    m_log->add("Loaded default layout: P1–P4 with 4 holes.", Theme::muted);
    refresh(false);
}

int AllocationTab::algo() const { return m_algo->currentIndex() + 1; }

void AllocationTab::allocate()
{
    std::string status;
    const bool ok = m_mm.allocate(m_size->value(), algo(), &status);
    m_log->add(QString("%1 · %2").arg(m_algo->currentText(), QString::fromStdString(status)),
               ok ? Theme::hit : Theme::fault);
    refresh();
}

void AllocationTab::release(int index)
{
    const auto& blocks = m_mm.getBlocks();
    if (index < 0 || index >= int(blocks.size()) || blocks[index].pid == -1) return;
    std::string status;
    m_mm.release(blocks[index].pid, &status);
    m_log->add(QString::fromStdString(status) + " Holes coalesced.", Theme::fault);
    refresh();
}

void AllocationTab::refresh(bool animate)
{
    const auto& blocks = m_mm.getBlocks();
    const int pick = m_mm.findHole(m_size->value(), algo());

    QVector<Segment> segs;
    int addr = 0;
    for (int i = 0; i < int(blocks.size()); ++i) {
        const Block& b = blocks[i];
        Segment s;
        s.size = b.size;
        s.free = (b.pid == -1);
        if (s.free) {
            s.key = QString("free@%1/%2").arg(addr).arg(b.size);
        } else {
            s.key = QString("p%1").arg(b.pid);
            s.label = QString("P%1").arg(b.pid);
            s.color = Theme::procColor(b.pid - 1);
        }
        if (i == pick) {
            s.highlight = true;
            s.badge = QString("%1 → here").arg(m_algo->currentText());
        }
        segs.push_back(s);
        addr += b.size;
    }
    m_bar->setSegments(segs, animate);

    int freeMem = 0, largest = 0, frag = 0;
    m_mm.getMetrics(freeMem, largest, frag);
    m_free->setValue(QString("%1 KB").arg(freeMem));
    m_largest->setValue(QString("%1 KB").arg(largest));
    m_frag->setValue(QString("%1%").arg(frag), frag >= 50 ? Theme::fault : (frag > 0 ? QColor("#B7791F") : Theme::hit));

    const int req = m_size->value();
    if (pick >= 0)
        m_hint->setText(QString("%1 places %2 KB in the hole of %3 KB (leftover %4).")
                            .arg(m_algo->currentText()).arg(req).arg(blocks[pick].size).arg(blocks[pick].size - req));
    else if (freeMem >= req)
        m_hint->setText(QString("No single hole fits %1 KB, but %2 KB is free in total. Compact memory, then allocate.")
                            .arg(req).arg(freeMem));
    else
        m_hint->setText(QString("Not enough free memory for %1 KB (%2 KB free).").arg(req).arg(freeMem));
}
