#include "Theme.h"

#include <QDateTime>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>

namespace Theme {

QColor procColor(int index)
{
    static const QColor colors[] = {
        QColor("#E0A458"), QColor("#5B8DEF"), QColor("#E86A5C"), QColor("#6CC4A1"),
        QColor("#B07CE8"), QColor("#F2D16B"), QColor("#4FB3D9"), QColor("#E58FB6"),
        QColor("#A3C76D"), QColor("#F08A4B")
    };
    const int n = int(sizeof(colors) / sizeof(colors[0]));
    return colors[((index % n) + n) % n];
}

QFont mono(int px, bool bold)
{
    QFont f;
    f.setFamilies({"JetBrains Mono", "Cascadia Mono", "Consolas", "Menlo", "DejaVu Sans Mono", "monospace"});
    f.setStyleHint(QFont::Monospace);
    f.setPixelSize(px);
    f.setBold(bold);
    return f;
}

QFont ui(int px, int weight)
{
    QFont f;
    f.setFamilies({"Inter", "Segoe UI", "SF Pro Text", "Helvetica Neue", "Noto Sans", "DejaVu Sans"});
    f.setPixelSize(px);
    f.setWeight(QFont::Weight(weight));
    return f;
}

QString styleSheet()
{
    return QStringLiteral(R"(
QFrame#card {
    background: #FFFFFF;
    border: 1px solid #D9DDD4;
    border-radius: 12px;
}
QLabel { background: transparent; }
QLabel#heading { color: #1A221E; font-weight: 700; font-size: 14px; }
QLabel#caption { color: #66706A; font-size: 12px; }
QLabel#error   { color: #C8463D; font-size: 12px; font-weight: 600; }

QPushButton {
    background: #FFFFFF;
    color: #1A221E;
    border: 1px solid #C9CEC4;
    border-radius: 8px;
    padding: 7px 14px;
    font-weight: 600;
}
QPushButton:hover   { border-color: #1F7A5A; color: #10392C; }
QPushButton:pressed { background: #E4E8E0; }
QPushButton:focus   { border: 2px solid #D2A441; }
QPushButton:disabled { color: #A5ADA7; border-color: #E0E3DC; }
QPushButton[kind="primary"] { background: #1F7A5A; color: #FFFFFF; border: 1px solid #1F7A5A; }
QPushButton[kind="primary"]:hover { background: #10392C; }
QPushButton[kind="primary"]:disabled { background: #9DBDB0; border-color: #9DBDB0; }
QPushButton[kind="danger"] { color: #C8463D; border-color: #EBC3BF; }
QPushButton[kind="danger"]:hover { background: #FBEDEB; border-color: #C8463D; }
QPushButton[kind="small"] { padding: 4px 10px; font-size: 12px; }
QPushButton[kind="smalldanger"] { padding: 4px 10px; font-size: 12px; color: #C8463D; border-color: #EBC3BF; }

QLineEdit, QSpinBox, QComboBox {
    background: #FFFFFF;
    border: 1px solid #C9CEC4;
    border-radius: 8px;
    padding: 6px 8px;
    min-height: 20px;
    selection-background-color: #1F7A5A;
}
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border: 2px solid #1F7A5A; }
QSpinBox { padding-right: 20px; }
QSpinBox::up-button, QSpinBox::down-button {
    subcontrol-origin: border; width: 18px; border: none; background: transparent;
}
QSpinBox::up-button   { subcontrol-position: top right; margin-top: 3px; }
QSpinBox::down-button { subcontrol-position: bottom right; margin-bottom: 3px; }
QSpinBox::up-arrow   { image: url(:/icons/arrow-up.png);   width: 9px; height: 6px; }
QSpinBox::down-arrow { image: url(:/icons/arrow-down.png); width: 9px; height: 6px; }
QComboBox { padding-right: 24px; }
QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 22px; border: none; }
QComboBox::down-arrow { image: url(:/icons/arrow-down.png); width: 9px; height: 6px; }
QComboBox QAbstractItemView { background: #FFFFFF; selection-background-color: #1F7A5A; }

QTabWidget::pane { border: none; }
QTabBar::tab {
    background: transparent;
    color: #66706A;
    padding: 10px 20px;
    margin-right: 4px;
    font-weight: 600;
    font-size: 13px;
    border-bottom: 3px solid transparent;
}
QTabBar::tab:hover    { color: #10392C; }
QTabBar::tab:selected { color: #10392C; border-bottom: 3px solid #D2A441; }

QListWidget#log {
    background: #10392C;
    color: #DDE8E1;
    border: none;
    border-radius: 10px;
    padding: 6px;
}
QListWidget#log::item { padding: 3px 4px; }
QListWidget#log::item:selected { background: #1A4D3C; color: #FFFFFF; }

QTableWidget {
    background: #FFFFFF;
    border: none;
    gridline-color: transparent;
    selection-background-color: #EEF0EA;
    selection-color: #1A221E;
}
QHeaderView::section {
    background: #FFFFFF;
    color: #66706A;
    border: none;
    border-bottom: 1px solid #D9DDD4;
    padding: 6px 4px;
    font-weight: 600;
    font-size: 11px;
}
QCheckBox { spacing: 6px; }
QScrollArea { background: transparent; border: none; }
QScrollBar:horizontal { height: 10px; background: transparent; }
QScrollBar::handle:horizontal { background: #C9CEC4; border-radius: 5px; min-width: 30px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
)");
}

QFrame* card(QWidget* parent)
{
    auto* f = new QFrame(parent);
    f->setObjectName("card");
    return f;
}

QLabel* heading(const QString& text, QWidget* parent)
{
    auto* l = new QLabel(text, parent);
    l->setObjectName("heading");
    return l;
}

QLabel* caption(const QString& text, QWidget* parent)
{
    auto* l = new QLabel(text, parent);
    l->setObjectName("caption");
    l->setWordWrap(true);
    return l;
}

QPushButton* button(const QString& text, const QString& kind, QWidget* parent)
{
    auto* b = new QPushButton(text, parent);
    if (!kind.isEmpty())
        b->setProperty("kind", kind);
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

} // namespace Theme

// ---------------------------------------------------------------------------

LogView::LogView(QWidget* parent) : QListWidget(parent)
{
    setObjectName("log");
    setFont(Theme::mono(12));
    setWordWrap(true);
    setUniformItemSizes(false);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void LogView::add(const QString& message, const QColor& tone)
{
    QPixmap dot(10, 10);
    dot.fill(Qt::transparent);
    {
        QPainter p(&dot);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(tone);
        p.drawRoundedRect(QRectF(1, 1, 8, 8), 2, 2);
    }
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    auto* item = new QListWidgetItem(QIcon(dot), QString("%1  %2").arg(time, message));
    insertItem(0, item);
    while (count() > 200)
        delete takeItem(count() - 1);
}

// ---------------------------------------------------------------------------

StatTile::StatTile(const QString& label, QWidget* parent) : QFrame(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(2);
    auto* l = new QLabel(label.toUpper(), this);
    l->setFont(Theme::ui(10, QFont::DemiBold));
    l->setStyleSheet("color:#66706A; letter-spacing:1px;");
    m_value = new QLabel("—", this);
    m_value->setFont(Theme::mono(22, true));
    lay->addWidget(l);
    lay->addWidget(m_value);
}

void StatTile::setValue(const QString& value, const QColor& tone)
{
    m_value->setText(value);
    m_value->setStyleSheet(QString("color:%1;").arg(tone.name()));
}
