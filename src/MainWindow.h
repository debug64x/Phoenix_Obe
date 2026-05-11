#pragma once

#include <QImage>
#include <QMainWindow>
#include <QRect>
#include <QStringList>

class EditableImageView;
class EditPanel;
class QPushButton;
class QLabel;

enum class Effect { None, Grayscale, Sepia, Blur, Sharpen, Invert };

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    void openPath(const QString &path);

protected:
    void keyPressEvent(QKeyEvent *event)   override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dropEvent(QDropEvent *)           override;

private slots:
    void openDialog();
    void prevFile();
    void nextFile();

private:
    void buildUI();
    QWidget *buildNavBar();
    void setupMenus();

    void loadFolderFrom(const QString &filePath);
    void showIndex(int index);
    void updateNavBar();
    bool isImage(const QString &path) const;

    void rebuildImage();

    // image processing
    static QImage adjustHueSaturation(const QImage &src, int hueDelta, int satDelta);
    static QImage applyEffect(const QImage &src, Effect effect);
    static QImage applyBlur(const QImage &src, int radius);

    // ── Widgets ───────────────────────────────────────────────────────────────
    EditableImageView *m_view;
    EditPanel         *m_editPanel;
    QPushButton       *m_prevBtn;
    QPushButton       *m_nextBtn;
    QLabel            *m_infoLabel;
    QLabel            *m_zoomLabel;

    // ── File state ────────────────────────────────────────────────────────────
    QStringList m_files;
    int         m_current = -1;

    // ── Edit state ────────────────────────────────────────────────────────────
    QImage m_originalImage;
    QRect  m_cropRect;
    int    m_hue    = 0;
    int    m_sat    = 0;
    Effect m_effect = Effect::None;
};
