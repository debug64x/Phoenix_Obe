#pragma once

#include <QWidget>

class QSlider;
class QLabel;
class QPushButton;

class EditPanel : public QWidget
{
    Q_OBJECT
public:
    explicit EditPanel(QWidget *parent = nullptr);

    void resetControls();
    void setCropMode(bool on);

signals:
    void hueChanged(int value);         // -180 .. 180
    void saturationChanged(int value);  // -100 .. 100
    void effectRequested(const QString &name); // "grayscale","sepia","blur","sharpen","invert", or ""
    void cropStartRequested();
    void cropApplyRequested();
    void cropCancelRequested();
    void exportRequested(int quality);  // 1..100
    void resetRequested();

private slots:
    void onEffectClicked();

private:
    QWidget *makeSectionLabel(const QString &text);

    QSlider     *m_hueSlider;
    QLabel      *m_hueValLabel;
    QSlider     *m_satSlider;
    QLabel      *m_satValLabel;

    QList<QPushButton *> m_effectBtns;

    QPushButton *m_cropStartBtn;
    QPushButton *m_cropApplyBtn;
    QPushButton *m_cropCancelBtn;

    QSlider     *m_qualitySlider;
    QLabel      *m_qualityValLabel;
};
