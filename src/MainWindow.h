#pragma once

#include <QMainWindow>

class QTabWidget;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    QTabWidget* tabs() const { return m_tabs; }
private:
    QTabWidget* m_tabs;
};
