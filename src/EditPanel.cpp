#include "EditPanel.h"

#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QVBoxLayout>

EditPanel::EditPanel(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(210);

    // ── inner widget inside a scroll area ────────────────────────────────────
    auto *inner  = new QWidget;
    auto *vbox   = new QVBoxLayout(inner);
    vbox->setSpacing(10);
    vbox->setContentsMargins(8, 8, 8, 8);

    // ── Adjustments ───────────────────────────────────────────────────────────
    {
        auto *grp    = new QGroupBox("Adjustments");
        auto *layout = new QGridLayout(grp);
        layout->setSpacing(4);

        layout->addWidget(new QLabel("Hue"), 0, 0);
        m_hueValLabel = new QLabel("0°");
        m_hueValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(m_hueValLabel, 0, 1);

        m_hueSlider = new QSlider(Qt::Horizontal);
        m_hueSlider->setRange(-180, 180);
        m_hueSlider->setValue(0);
        layout->addWidget(m_hueSlider, 1, 0, 1, 2);

        layout->addWidget(new QLabel("Saturation"), 2, 0);
        m_satValLabel = new QLabel("0%");
        m_satValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(m_satValLabel, 2, 1);

        m_satSlider = new QSlider(Qt::Horizontal);
        m_satSlider->setRange(-100, 100);
        m_satSlider->setValue(0);
        layout->addWidget(m_satSlider, 3, 0, 1, 2);

        vbox->addWidget(grp);

        connect(m_hueSlider, &QSlider::valueChanged, this, [this](int v){
            m_hueValLabel->setText(QString("%1°").arg(v));
            emit hueChanged(v);
        });
        connect(m_satSlider, &QSlider::valueChanged, this, [this](int v){
            m_satValLabel->setText(QString("%1%").arg(v));
            emit saturationChanged(v);
        });
    }

    // ── Effects ───────────────────────────────────────────────────────────────
    {
        auto *grp    = new QGroupBox("Effects");
        auto *layout = new QGridLayout(grp);
        layout->setSpacing(4);

        const QStringList names  = {"Grayscale", "Sepia", "Blur", "Sharpen", "Invert"};
        const QStringList keys   = {"grayscale", "sepia", "blur", "sharpen", "invert"};

        for (int i = 0; i < names.size(); ++i) {
            auto *btn = new QPushButton(names[i]);
            btn->setCheckable(true);
            btn->setProperty("effectKey", keys[i]);
            layout->addWidget(btn, i / 2, i % 2);
            m_effectBtns << btn;
            connect(btn, &QPushButton::clicked, this, &EditPanel::onEffectClicked);
        }
        vbox->addWidget(grp);
    }

    // ── Crop ─────────────────────────────────────────────────────────────────
    {
        auto *grp    = new QGroupBox("Crop");
        auto *layout = new QVBoxLayout(grp);
        layout->setSpacing(4);

        m_cropStartBtn = new QPushButton("✂  Start Crop");
        m_cropApplyBtn = new QPushButton("✓  Apply");
        m_cropCancelBtn = new QPushButton("✗  Cancel");
        m_cropApplyBtn->setEnabled(false);
        m_cropCancelBtn->setEnabled(false);

        layout->addWidget(m_cropStartBtn);
        auto *row = new QHBoxLayout;
        row->addWidget(m_cropApplyBtn);
        row->addWidget(m_cropCancelBtn);
        layout->addLayout(row);

        vbox->addWidget(grp);

        connect(m_cropStartBtn, &QPushButton::clicked, this, [this]{
            m_cropStartBtn->setEnabled(false);
            m_cropApplyBtn->setEnabled(true);
            m_cropCancelBtn->setEnabled(true);
            emit cropStartRequested();
        });
        connect(m_cropApplyBtn, &QPushButton::clicked, this, [this]{
            m_cropStartBtn->setEnabled(true);
            m_cropApplyBtn->setEnabled(false);
            m_cropCancelBtn->setEnabled(false);
            emit cropApplyRequested();
        });
        connect(m_cropCancelBtn, &QPushButton::clicked, this, [this]{
            m_cropStartBtn->setEnabled(true);
            m_cropApplyBtn->setEnabled(false);
            m_cropCancelBtn->setEnabled(false);
            emit cropCancelRequested();
        });
    }

    // ── Export / Compress ─────────────────────────────────────────────────────
    {
        auto *grp    = new QGroupBox("Export");
        auto *layout = new QGridLayout(grp);
        layout->setSpacing(4);

        layout->addWidget(new QLabel("JPEG Quality"), 0, 0);
        m_qualityValLabel = new QLabel("85%");
        m_qualityValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        layout->addWidget(m_qualityValLabel, 0, 1);

        m_qualitySlider = new QSlider(Qt::Horizontal);
        m_qualitySlider->setRange(1, 100);
        m_qualitySlider->setValue(85);
        layout->addWidget(m_qualitySlider, 1, 0, 1, 2);

        auto *saveBtn = new QPushButton("Save As...");
        layout->addWidget(saveBtn, 2, 0, 1, 2);

        vbox->addWidget(grp);

        connect(m_qualitySlider, &QSlider::valueChanged, this, [this](int v){
            m_qualityValLabel->setText(QString("%1%").arg(v));
        });
        connect(saveBtn, &QPushButton::clicked, this, [this]{
            emit exportRequested(m_qualitySlider->value());
        });
    }

    // ── Reset ─────────────────────────────────────────────────────────────────
    {
        auto *resetBtn = new QPushButton("↺  Reset All");
        resetBtn->setFixedHeight(30);
        vbox->addWidget(resetBtn);
        connect(resetBtn, &QPushButton::clicked, this, [this]{
            resetControls();
            emit resetRequested();
        });
    }

    vbox->addStretch();

    // wrap inner in a scroll area
    auto *scroll = new QScrollArea(this);
    scroll->setWidget(inner);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(scroll);
}

void EditPanel::resetControls()
{
    m_hueSlider->setValue(0);
    m_satSlider->setValue(0);
    m_qualitySlider->setValue(85);
    for (auto *btn : m_effectBtns)
        btn->setChecked(false);
    m_cropStartBtn->setEnabled(true);
    m_cropApplyBtn->setEnabled(false);
    m_cropCancelBtn->setEnabled(false);
}

void EditPanel::setCropMode(bool on)
{
    m_cropStartBtn->setEnabled(!on);
    m_cropApplyBtn->setEnabled(on);
    m_cropCancelBtn->setEnabled(on);
}

void EditPanel::onEffectClicked()
{
    auto *clicked = qobject_cast<QPushButton *>(sender());
    if (!clicked) return;

    const bool nowChecked = clicked->isChecked();

    for (auto *btn : m_effectBtns)
        btn->setChecked(false);

    if (nowChecked) {
        clicked->setChecked(true);
        emit effectRequested(clicked->property("effectKey").toString());
    } else {
        emit effectRequested("");
    }
}
