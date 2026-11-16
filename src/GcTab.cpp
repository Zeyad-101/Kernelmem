#include "GcTab.h"
#include "MemoryBar.h"
#include "Theme.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <iostream>
#include <sstream>

namespace {
// The backend reports through std::cout; capture it so the messages show in the log.
class CoutCapture {
public:
    CoutCapture() : m_old(std::cout.rdbuf(m_buf.rdbuf())) {}
    ~CoutCapture() { std::cout.rdbuf(m_old); }
    QStringList lines() const
    {
        return QString::fromStdString(m_buf.str()).split('\n', Qt::SkipEmptyParts);
    }
private:
    std::stringstream m_buf;
    std::streambuf* m_old;
};

QString heapName(int i)
{
    return i < 26 ? QString(QChar('A' + i)) : QString("O%1").arg(i);
}
} // namespace

GcTab::GcTab(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 16, 0, 0);
    root->setSpacing(14);

    // --- Heap view ----------------------------------------------------------------
    auto* mapCard = Theme::card();
    auto* ml = new QVBoxLayout(mapCard);
    ml->setContentsMargins(18, 14, 18, 14);
    ml->setSpacing(6);
    auto* head = new QHBoxLayout;
    head->addWidget(Theme::heading("Heap"));
    head->addStretch();
    m_phase = new QLabel;
    m_phase->setFont(Theme::ui(12, QFont::Bold));
    head->addWidget(m_phase);
    ml->addLayout(head);
    ml->addWidget(Theme::caption("Arrows are references. Click an object to make it a root (or remove the root). "
                                 "Mark walks from the roots; anything not reached is garbage."));

    m_bar = new MemoryBar;
    m_bar->setTopSpace(96);
    m_bar->setClickable([this](int i) { return i < int(m_gc.memory.size()) && !m_gc.memory[i].isFree; });
    ml->addWidget(m_bar);
    root->addWidget(mapCard);

    // --- Controls + log -----------------------------------------------------------
    auto* bottom = new QHBoxLayout;
    bottom->setSpacing(14);

    auto* ctrlCard = Theme::card();
    auto* g = new QGridLayout(ctrlCard);
    g->setContentsMargins(18, 14, 18, 14);
    g->setHorizontalSpacing(10);
    g->setVerticalSpacing(12);

    m_rootPick = new QComboBox;
    auto* rootAdd = Theme::button("Make root");
    auto* rootDel = Theme::button("Remove root");
    auto* freeBtn = Theme::button("Free now", "danger");
    g->addWidget(Theme::heading("Object"), 0, 0);
    auto* r0 = new QHBoxLayout;
    r0->setSpacing(8);
    r0->addWidget(m_rootPick, 1);
    r0->addWidget(rootAdd);
    r0->addWidget(rootDel);
    r0->addWidget(freeBtn);
    g->addLayout(r0, 0, 1);

    m_from = new QComboBox;
    m_to = new QComboBox;
    auto* linkBtn = Theme::button("Link");
    auto* unlinkBtn = Theme::button("Unlink");
    auto* arrow = new QLabel("→");
    arrow->setFont(Theme::ui(16, QFont::Bold));
    g->addWidget(Theme::heading("Reference"), 1, 0);
    auto* r1 = new QHBoxLayout;
    r1->setSpacing(8);
    r1->addWidget(m_from, 1);
    r1->addWidget(arrow);
    r1->addWidget(m_to, 1);
    r1->addWidget(linkBtn);
    r1->addWidget(unlinkBtn);
    g->addLayout(r1, 1, 1);

    m_newSize = new QSpinBox;
    m_newSize->setRange(1, 200);
    m_newSize->setValue(10);
    m_newSize->setSuffix(" KB");
    auto* newBtn = Theme::button("Allocate object");
    g->addWidget(Theme::heading("New"), 2, 0);
    auto* r2 = new QHBoxLayout;
    r2->setSpacing(8);
    r2->addWidget(m_newSize);
    r2->addWidget(newBtn);
    r2->addWidget(Theme::caption("Placed in the first hole that fits. Starts unreferenced."), 1);
    g->addLayout(r2, 2, 1);

    auto* markBtn = Theme::button("1 · Mark", "primary");
    m_sweepBtn = Theme::button("2 · Sweep", "primary");
    auto* gcBtn = Theme::button("Run full GC");
    auto* compactBtn = Theme::button("Compact");
    auto* resetBtn = Theme::button("Reset");
    g->addWidget(Theme::heading("Collector"), 3, 0);
    auto* r3 = new QHBoxLayout;
    r3->setSpacing(8);
    r3->addWidget(markBtn);
    r3->addWidget(m_sweepBtn);
    r3->addWidget(gcBtn);
    r3->addStretch();
    r3->addWidget(compactBtn);
    r3->addWidget(resetBtn);
    g->addLayout(r3, 3, 1);
    g->setColumnStretch(1, 1);
    g->setRowStretch(4, 1);
    bottom->addWidget(ctrlCard, 3);

    auto* logCard = Theme::card();
    auto* ll = new QVBoxLayout(logCard);
    ll->setContentsMargins(18, 14, 18, 14);
    ll->addWidget(Theme::heading("Collector output"));
    m_log = new LogView;
    ll->addWidget(m_log);
    bottom->addWidget(logCard, 2);
    root->addLayout(bottom, 1);

    // --- Wiring -----------------------------------------------------------------
    connect(m_bar, &MemoryBar::segmentClicked, this, [this](int i) {
        const std::string name = m_gc.memory[i].name;
        bool isRoot = false;
        for (const auto& r : m_gc.roots) isRoot |= (r == name);
        CoutCapture cap;
        if (isRoot) m_gc.removeRoot(name); else m_gc.addRoot(name);
        logBackend(cap.lines(), Theme::gold);
        invalidateMarks();
        refresh(false);
    });
    connect(rootAdd, &QPushButton::clicked, this, [this] {
        CoutCapture cap;
        m_gc.addRoot(m_rootPick->currentText().toStdString());
        logBackend(cap.lines(), Theme::gold);
        invalidateMarks();
        refresh(false);
    });
    connect(rootDel, &QPushButton::clicked, this, [this] {
        CoutCapture cap;
        m_gc.removeRoot(m_rootPick->currentText().toStdString());
        logBackend(cap.lines(), Theme::gold);
        invalidateMarks();
        refresh(false);
    });
    connect(freeBtn, &QPushButton::clicked, this, [this] {
        const QString name = m_rootPick->currentText();
        if (name.isEmpty()) return;
        m_gc.freeMemory(name.toStdString());
        m_log->add(QString("%1 freed manually; references to it removed, holes coalesced.").arg(name), Theme::fault);
        invalidateMarks();
        refresh();
    });
    connect(linkBtn, &QPushButton::clicked, this, [this] {
        CoutCapture cap;
        m_gc.addReference(m_from->currentText().toStdString(), m_to->currentText().toStdString());
        logBackend(cap.lines(), Theme::accent);
        invalidateMarks();
        refresh(false);
    });
    connect(unlinkBtn, &QPushButton::clicked, this, [this] {
        CoutCapture cap;
        m_gc.removeReference(m_from->currentText().toStdString(), m_to->currentText().toStdString());
        logBackend(cap.lines(), Theme::accent);
        invalidateMarks();
        refresh(false);
    });
    connect(newBtn, &QPushButton::clicked, this, [this] { newObject(); });
    connect(markBtn, &QPushButton::clicked, this, [this] {
        // Same steps garbageCollection() runs before sweeping, done separately so the user sees them.
        m_gc.resetMarks();
        for (const auto& r : m_gc.roots) m_gc.mark(m_gc.findBlock(r));
        m_marked = true;
        int garbage = 0;
        for (const auto& b : m_gc.memory) garbage += (!b.isFree && !b.isReachable);
        m_log->add(QString("Mark: walked from %1 root(s). %2 object(s) unreachable.")
                       .arg(m_gc.roots.size()).arg(garbage), Theme::gold);
        refresh(false);
    });
    connect(m_sweepBtn, &QPushButton::clicked, this, [this] {
        CoutCapture cap;
        m_gc.sweep();
        logBackend(cap.lines(), Theme::fault);
        m_log->add("Sweep done: garbage freed, holes coalesced.", Theme::hit);
        m_marked = false;
        m_gc.resetMarks();
        refresh();
    });
    connect(gcBtn, &QPushButton::clicked, this, [this] {
        CoutCapture cap;
        m_gc.garbageCollection();
        logBackend(cap.lines(), Theme::fault);
        m_marked = false;
        m_gc.resetMarks();
        refresh();
    });
    connect(compactBtn, &QPushButton::clicked, this, [this] {
        m_gc.compact();
        m_log->add("Compaction: live objects moved left, one free hole at the end.", Theme::gold);
        invalidateMarks();
        refresh();
    });
    connect(resetBtn, &QPushButton::clicked, this, [this] { loadPreset(); });

    loadPreset();
}

void GcTab::loadPreset()
{
    using B = memoryManger::Block;
    m_gc.memory = {
        B{"A", 20, false, false, {"B"}},
        B{"B", 15, false, false, {"C"}},
        B{"",  10, true,  false, {}},
        B{"C", 25, false, false, {}},
        B{"D", 15, false, false, {"E"}},
        B{"",  15, true,  false, {}},
        B{"E", 20, false, false, {"D"}},
        B{"F", 12, false, false, {}},
        B{"",  18, true,  false, {}},
    };
    m_gc.roots = {"A"};
    m_nextName = 6;
    m_marked = false;
    m_log->clear();
    m_log->add("Heap loaded. Root A → B → C is live; D ⇄ E is a cycle nobody points to; F is orphaned.", Theme::muted);
    refresh(false);
}

void GcTab::invalidateMarks()
{
    if (m_marked) m_gc.resetMarks();
    m_marked = false;
}

void GcTab::newObject()
{
    const int req = m_newSize->value();
    for (size_t i = 0; i < m_gc.memory.size(); ++i) {
        auto& b = m_gc.memory[i];
        if (!b.isFree || b.size < req) continue;
        const int left = b.size - req;
        const QString name = heapName(m_nextName++);
        b = memoryManger::Block{name.toStdString(), req, false, false, {}};
        if (left > 0)
            m_gc.memory.insert(m_gc.memory.begin() + i + 1, memoryManger::Block{"", left, true, false, {}});
        m_log->add(QString("Allocated %1 (%2 KB). Link it or make it a root, or the next GC collects it.")
                       .arg(name).arg(req), Theme::hit);
        invalidateMarks();
        refresh();
        return;
    }
    m_log->add(QString("No hole fits %1 KB. Run GC or compact first.").arg(req), Theme::fault);
}

void GcTab::logBackend(const QStringList& lines, const QColor& tone)
{
    for (const QString& l : lines) {
        const bool bad = l.contains("not found") || l.contains("already");
        m_log->add(l, bad ? Theme::fault : tone);
    }
}

QStringList GcTab::liveNames() const
{
    QStringList names;
    for (const auto& b : m_gc.memory)
        if (!b.isFree) names << QString::fromStdString(b.name);
    return names;
}

void GcTab::refresh(bool animate)
{
    QVector<Segment> segs;
    QHash<QString, int> indexOf;
    int addr = 0;
    for (int i = 0; i < int(m_gc.memory.size()); ++i) {
        const auto& b = m_gc.memory[i];
        Segment s;
        s.size = b.size;
        s.free = b.isFree;
        if (b.isFree) {
            s.key = QString("free@%1/%2").arg(addr).arg(b.size);
        } else {
            const QString name = QString::fromStdString(b.name);
            s.key = "obj:" + name;
            s.label = name;
            s.color = Theme::procColor(name.isEmpty() ? 0 : name.at(0).unicode() - 'A' + (name.size() > 1 ? 3 : 0));
            for (const auto& r : m_gc.roots) s.root |= (r == b.name);
            if (m_marked) s.mark = b.isReachable ? 1 : 2;
            indexOf.insert(name, i);
        }
        segs.push_back(s);
        addr += b.size;
    }

    QVector<QPair<int, int>> arcs;
    for (int i = 0; i < int(m_gc.memory.size()); ++i)
        for (const auto& ref : m_gc.memory[i].references) {
            const int j = indexOf.value(QString::fromStdString(ref), -1);
            if (j >= 0) arcs.push_back({i, j});
        }

    m_bar->setSegments(segs, animate);
    m_bar->setArcs(arcs);

    // keep combo selections where possible
    const QStringList names = liveNames();
    auto refill = [&names](QComboBox* c, int fallback) {
        const QString cur = c->currentText();
        c->blockSignals(true);
        c->clear();
        c->addItems(names);
        const int idx = names.indexOf(cur);
        c->setCurrentIndex(idx >= 0 ? idx : std::min(fallback, int(names.size()) - 1));
        c->blockSignals(false);
    };
    refill(m_rootPick, 0);
    refill(m_from, 0);
    refill(m_to, 1);

    m_sweepBtn->setEnabled(m_marked);
    if (m_marked) {
        m_phase->setText("● Marked: gold = reachable, red = garbage");
        m_phase->setStyleSheet("color:#B7791F;");
    } else {
        m_phase->setText(QString("Roots: %1").arg(m_gc.roots.empty() ? "none" : [&] {
            QStringList r;
            for (const auto& s : m_gc.roots) r << QString::fromStdString(s);
            return r.join(", ");
        }()));
        m_phase->setStyleSheet("color:#66706A;");
    }
}
