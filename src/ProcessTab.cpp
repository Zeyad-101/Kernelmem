#include "ProcessTab.h"
#include "MemoryBar.h"
#include "Theme.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <chrono>

namespace {
enum Col { ColName, ColPid, ColSize, ColIdle, ColStatus, ColActions, ColCount };

QString formatIdle(long long secs)
{
    if (secs < 60) return QString("%1s").arg(secs);
    if (secs < 3600) return QString("%1m %2s").arg(secs / 60).arg(secs % 60, 2, 10, QChar('0'));
    return QString("%1h %2m").arg(secs / 3600).arg((secs % 3600) / 60);
}

long long idleSeconds(const processManagement::Process& p)
{
    using namespace std::chrono;
    return duration_cast<seconds>(steady_clock::now() - p.lastActivity).count();
}
} // namespace

ProcessTab::ProcessTab(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 16, 0, 0);
    root->setSpacing(14);

    // --- Add process -----------------------------------------------------------
    auto* controls = Theme::card();
    auto* cv = new QVBoxLayout(controls);
    cv->setContentsMargins(18, 14, 18, 14);
    cv->setSpacing(8);
    auto* cl = new QHBoxLayout;
    cl->setSpacing(10);
    m_name = new QLineEdit;
    m_name->setPlaceholderText("Process name, e.g. Chrome");
    m_name->setMaxLength(16);
    m_size = new QSpinBox;
    m_size->setRange(1, 999);
    m_size->setValue(12);
    m_size->setSuffix(" KB");
    auto* addBtn = Theme::button("Start process", "primary");
    auto* resetBtn = Theme::button("Reset");
    cl->addWidget(Theme::caption("Name"));
    cl->addWidget(m_name, 1);
    cl->addWidget(Theme::caption("Size"));
    cl->addWidget(m_size);
    cl->addWidget(addBtn);
    cl->addWidget(resetBtn);
    cv->addLayout(cl);
    m_error = new QLabel;
    m_error->setObjectName("error");
    m_error->hide();
    cv->addWidget(m_error);
    root->addWidget(controls);

    // --- Memory -----------------------------------------------------------------
    auto* mapCard = Theme::card();
    auto* ml = new QVBoxLayout(mapCard);
    ml->setContentsMargins(18, 12, 18, 12);
    ml->setSpacing(4);
    auto* mh = new QHBoxLayout;
    mh->addWidget(Theme::heading("Memory used by processes"));
    mh->addStretch();
    mh->addWidget(Theme::caption("New processes land at a random free spot."));
    auto* compactBtn = Theme::button("Compact memory");
    mh->addWidget(compactBtn);
    ml->addLayout(mh);
    m_bar = new MemoryBar;
    m_bar->setTopSpace(14);
    ml->addWidget(m_bar);
    root->addWidget(mapCard);

    // --- Table + log ------------------------------------------------------------
    auto* bottom = new QHBoxLayout;
    bottom->setSpacing(14);

    auto* tableCard = Theme::card();
    auto* tl = new QVBoxLayout(tableCard);
    tl->setContentsMargins(18, 14, 18, 14);
    tl->setSpacing(8);
    auto* th = new QHBoxLayout;
    th->addWidget(Theme::heading("Process table"));
    th->addStretch();
    m_auto = new QCheckBox("Auto-terminate when idle more than");
    m_idleMin = new QSpinBox;
    m_idleMin->setRange(0, 120);
    m_idleMin->setValue(1);
    m_idleMin->setSuffix(" min");
    auto* cleanBtn = Theme::button("Clean up now");
    th->addWidget(m_auto);
    th->addWidget(m_idleMin);
    th->addWidget(cleanBtn);
    tl->addLayout(th);

    m_table = new QTableWidget(0, ColCount);
    m_table->setHorizontalHeaderLabels({"PROCESS", "PID", "SIZE", "IDLE", "STATUS", ""});
    m_table->verticalHeader()->hide();
    m_table->setShowGrid(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_table->horizontalHeader()->setSectionResizeMode(ColName, QHeaderView::Stretch);
    const int widths[] = {0, 52, 72, 72, 112, 184};
    for (int c = ColPid; c < ColCount; ++c) {
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Fixed);
        m_table->setColumnWidth(c, widths[c]);
    }
    m_table->verticalHeader()->setDefaultSectionSize(44);
    tl->addWidget(m_table, 1);
    bottom->addWidget(tableCard, 3);

    auto* logCard = Theme::card();
    auto* ll = new QVBoxLayout(logCard);
    ll->setContentsMargins(18, 14, 18, 14);
    ll->addWidget(Theme::heading("Kernel log"));
    m_log = new LogView;
    ll->addWidget(m_log);
    bottom->addWidget(logCard, 2);
    root->addLayout(bottom, 1);

    // --- Wiring -----------------------------------------------------------------
    connect(addBtn, &QPushButton::clicked, this, [this] { addProcess(); });
    connect(m_name, &QLineEdit::returnPressed, this, [this] { addProcess(); });
    connect(resetBtn, &QPushButton::clicked, this, [this] { resetAll(); });
    connect(compactBtn, &QPushButton::clicked, this, [this] { compact(); });
    connect(cleanBtn, &QPushButton::clicked, this, [this] { runCleanup(true); });
    connect(m_auto, &QCheckBox::toggled, this, [this](bool on) {
        m_log->add(QString("Auto-cleanup %1 (idle > %2 min).").arg(on ? "on" : "off").arg(m_idleMin->value()),
                   Theme::gold);
    });
    connect(&m_timer, &QTimer::timeout, this, [this] { tick(); });
    m_timer.start(1000);

    bootstrap();
}

void ProcessTab::bootstrap()
{
    // Register the processes already present in the backend's default memory layout.
    for (const Block& b : m_mm.getBlocks()) {
        if (b.pid == -1) continue;
        const QString name = QString("P%1").arg(b.pid);
        m_pm.addProcess(name.toStdString(), b.size);
        m_rec.push_back({m_nextPid++, b.pid});
    }
    m_log->add("P1–P4 loaded from the default memory layout and marked Running.", Theme::muted);
    refreshMemory(false);
    rebuildTable();
}

void ProcessTab::addProcess()
{
    m_error->hide();
    const QString name = m_name->text().trimmed();
    if (name.isEmpty()) {
        m_error->setText("Enter a process name.");
        m_error->show();
        return;
    }
    if (runningIndex(name) >= 0) {
        m_error->setText(QString("\"%1\" is already running. Terminate it first or pick another name.").arg(name));
        m_error->show();
        return;
    }

    const int size = m_size->value();
    const int memPid = allocateRandom(size);
    if (memPid < 0) {
        int freeMem = 0, largest = 0, frag = 0;
        m_mm.getMetrics(freeMem, largest, frag);
        m_error->setText(freeMem >= size
            ? QString("Can't start %1: no single hole fits %2 KB (largest %3 KB, %4 KB free). Compact memory first.")
                  .arg(name).arg(size).arg(largest).arg(freeMem)
            : QString("Can't start %1: only %2 KB free.").arg(name).arg(freeMem));
        m_error->show();
        m_log->add(QString("%1: allocation failed (%2 KB).").arg(name).arg(size), Theme::fault);
        return;
    }

    m_pm.addProcess(name.toStdString(), size);
    m_rec.push_back({m_nextPid, memPid});
    int start = 0;
    for (const Block& b : m_mm.getBlocks()) { if (b.pid == memPid) break; start += b.size; }
    m_log->add(QString("%1 started as PID %2 (%3 KB) at address %4.").arg(name).arg(m_nextPid).arg(size).arg(start),
               Theme::hit);
    ++m_nextPid;
    m_name->clear();
    refreshMemory();
    rebuildTable();
}

// Random placement using only the backend's public API:
//  1. pick a random hole that fits and a random offset inside it,
//  2. fill every earlier hole and the offset with temporary First-Fit blocks,
//  3. allocate the process (First Fit now lands exactly there),
//  4. release the temporary blocks (release() coalesces them back into holes).
int ProcessTab::allocateRandom(int size)
{
    const auto& blocks = m_mm.getBlocks();
    std::vector<int> fits;
    for (int i = 0; i < int(blocks.size()); ++i)
        if (blocks[i].pid == -1 && blocks[i].size >= size) fits.push_back(i);
    if (fits.empty()) return -1;

    const int target = fits[std::uniform_int_distribution<int>(0, int(fits.size()) - 1)(m_rng)];
    const int offset = std::uniform_int_distribution<int>(0, blocks[target].size - size)(m_rng);

    std::vector<int> earlierHoles;
    for (int i = 0; i < target; ++i)
        if (blocks[i].pid == -1) earlierHoles.push_back(blocks[i].size);

    auto newestPid = [this] {
        int pid = -1;
        for (const Block& b : m_mm.getBlocks()) pid = std::max(pid, b.pid);
        return pid;
    };

    std::vector<int> fillers;
    for (int holeSize : earlierHoles) {           // exact fill of each earlier hole
        m_mm.allocate(holeSize, 1);
        fillers.push_back(newestPid());
    }
    if (offset > 0) {                             // push the process away from the hole start
        m_mm.allocate(offset, 1);
        fillers.push_back(newestPid());
    }
    m_mm.allocate(size, 1);
    const int pid = newestPid();
    for (int f : fillers) m_mm.release(f);
    return pid;
}

int ProcessTab::runningIndex(const QString& name) const
{
    for (int i = 0; i < int(m_pm.processes.size()); ++i)
        if (m_pm.processes[i].isRunning &&
            QString::fromStdString(m_pm.processes[i].name).compare(name, Qt::CaseInsensitive) == 0)
            return i;
    return -1;
}

void ProcessTab::resetAll()
{
    m_pm.processes.clear();
    m_rec.clear();
    m_nextPid = 1;
    m_mm.loadDefaultPreset();
    m_error->hide();
    m_log->clear();
    bootstrap();
}

void ProcessTab::compact()
{
    m_mm.compact();
    m_log->add("Compaction: running processes moved to the start, free space merged into one hole.", Theme::gold);
    refreshMemory();
}

void ProcessTab::terminate(const QString& name)
{
    const int idx = runningIndex(name);
    if (idx < 0) return;
    m_pm.terminateProcess(name.toStdString());
    m_mm.release(m_rec[idx].memPid);
    m_log->add(QString("%1 terminated. Memory released and holes coalesced.").arg(name), Theme::fault);
    refreshMemory();
    rebuildTable();
}

void ProcessTab::touch(const QString& name)
{
    m_pm.updateActivity(name.toStdString());
    m_log->add(QString("%1 activity: idle timer reset.").arg(name), Theme::hit);
    tick();
}

void ProcessTab::runCleanup(bool manual)
{
    std::vector<bool> before;
    for (const auto& p : m_pm.processes) before.push_back(p.isRunning);

    m_pm.inactivityCleanup(m_idleMin->value());

    QStringList killed;
    for (int i = 0; i < int(m_pm.processes.size()); ++i) {
        if (before[i] && !m_pm.processes[i].isRunning) {
            m_mm.release(m_rec[i].memPid);
            killed << QString::fromStdString(m_pm.processes[i].name);
        }
    }
    if (!killed.isEmpty()) {
        m_log->add(QString("Idle cleanup (> %1 min) terminated %2.").arg(m_idleMin->value()).arg(killed.join(", ")),
                   Theme::fault);
        refreshMemory();
        rebuildTable();
    } else if (manual) {
        m_log->add(QString("Idle cleanup: nothing idle longer than %1 min.").arg(m_idleMin->value()), Theme::muted);
    }
}

void ProcessTab::refreshMemory(bool animate)
{
    QHash<int, QString> nameOf;     // backend PID -> running process name
    QHash<int, int> colorOf;        // backend PID -> GUI PID (for colour)
    for (int i = 0; i < int(m_pm.processes.size()); ++i)
        if (m_pm.processes[i].isRunning) {
            nameOf.insert(m_rec[i].memPid, QString::fromStdString(m_pm.processes[i].name));
            colorOf.insert(m_rec[i].memPid, m_rec[i].pid);
        }

    QVector<Segment> segs;
    int addr = 0;
    for (const Block& b : m_mm.getBlocks()) {
        Segment s;
        s.size = b.size;
        s.free = (b.pid == -1);
        if (s.free) {
            s.key = QString("free@%1/%2").arg(addr).arg(b.size);
        } else {
            s.key = QString("p%1").arg(b.pid);
            s.label = nameOf.value(b.pid, QString("P%1").arg(b.pid));
            s.color = Theme::procColor(colorOf.value(b.pid, b.pid) - 1);
        }
        segs.push_back(s);
        addr += b.size;
    }
    m_bar->setSegments(segs, animate);
}

void ProcessTab::rebuildTable()
{
    m_table->setRowCount(0);
    m_table->setRowCount(int(m_pm.processes.size()));
    for (int row = 0; row < int(m_pm.processes.size()); ++row) {
        const auto& p = m_pm.processes[row];
        const QString name = QString::fromStdString(p.name);
        const int pid = m_rec[row].pid;
        const QColor dim("#A5ADA7");

        auto* nameItem = new QTableWidgetItem(name);
        nameItem->setFont(Theme::ui(13, QFont::DemiBold));
        QPixmap sw(12, 12);
        sw.fill(p.isRunning ? Theme::procColor(pid - 1) : dim);
        nameItem->setIcon(QIcon(sw));
        if (!p.isRunning) nameItem->setForeground(dim);
        m_table->setItem(row, ColName, nameItem);

        auto mono = [&](const QString& t) {
            auto* it = new QTableWidgetItem(t);
            it->setFont(Theme::mono(12));
            if (!p.isRunning) it->setForeground(dim);
            return it;
        };
        m_table->setItem(row, ColPid, mono(QString::number(pid)));
        m_table->setItem(row, ColSize, mono(QString("%1 KB").arg(p.size)));
        m_table->setItem(row, ColIdle, mono(p.isRunning ? formatIdle(idleSeconds(p)) : "—"));

        auto* status = new QLabel(p.isRunning ? "● Running" : "● Terminated");
        status->setFont(Theme::ui(12, QFont::Bold));
        status->setStyleSheet(p.isRunning ? "color:#2E8B57;" : "color:#C8463D;");
        m_table->setCellWidget(row, ColStatus, status);

        auto* actions = new QWidget;
        auto* al = new QHBoxLayout(actions);
        al->setContentsMargins(0, 4, 4, 4);
        al->setSpacing(6);
        if (p.isRunning) {
            auto* act = Theme::button("Activity", "small");
            auto* kill = Theme::button("Terminate", "smalldanger");
            connect(act, &QPushButton::clicked, this, [this, name] { touch(name); });
            connect(kill, &QPushButton::clicked, this, [this, name] { terminate(name); });
            al->addWidget(act);
            al->addWidget(kill);
        } else {
            auto* l = Theme::caption("memory released");
            al->addWidget(l);
        }
        al->addStretch();
        m_table->setCellWidget(row, ColActions, actions);
    }
}

void ProcessTab::tick()
{
    if (m_auto->isChecked()) runCleanup(false);

    const long long limit = 60LL * m_idleMin->value();
    for (int row = 0; row < int(m_pm.processes.size()) && row < m_table->rowCount(); ++row) {
        const auto& p = m_pm.processes[row];
        auto* item = m_table->item(row, ColIdle);
        if (!item || !p.isRunning) continue;
        const long long s = idleSeconds(p);
        item->setText(formatIdle(s));
        item->setForeground(limit > 0 && s >= limit * 0.7 ? QColor("#B7791F") : Theme::ink);
    }
}
