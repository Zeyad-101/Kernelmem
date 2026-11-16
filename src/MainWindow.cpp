#include "MainWindow.h"
#include "AllocationTab.h"
#include "GcTab.h"
#include "PageTab.h"
#include "ProcessTab.h"
#include "Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("KernelMem — Memory Management Simulator");
    resize(1240, 860);
    setMinimumSize(980, 720);

    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(28, 22, 28, 22);
    root->setSpacing(4);

    auto* titleRow = new QHBoxLayout;
    auto* title = new QLabel("KernelMem");
    title->setFont(Theme::ui(26, QFont::Black));
    title->setStyleSheet("color:#10392C;");
    auto* tag = new QLabel("MEMORY MANAGEMENT SIMULATOR");
    tag->setFont(Theme::mono(11, true));
    tag->setStyleSheet("color:#10392C; background:#D2A441; border-radius:4px; padding:3px 8px;");
    titleRow->addWidget(title);
    titleRow->addSpacing(10);
    titleRow->addWidget(tag, 0, Qt::AlignVCenter);
    titleRow->addStretch();
    root->addLayout(titleRow);
    root->addWidget(Theme::caption("Try it yourself: allocation, page replacement, garbage collection and processes."));
    root->addSpacing(8);

    m_tabs = new QTabWidget;
    m_tabs->setDocumentMode(true);
    m_tabs->addTab(new AllocationTab, "Allocation");
    m_tabs->addTab(new PageTab, "Page replacement");
    m_tabs->addTab(new GcTab, "Garbage collector");
    m_tabs->addTab(new ProcessTab, "Processes");
    root->addWidget(m_tabs, 1);

    setCentralWidget(central);
}
