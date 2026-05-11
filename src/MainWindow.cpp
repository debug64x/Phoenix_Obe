#include "MainWindow.h"
#include "EditableImageView.h"
#include "EditPanel.h"

#include <QApplication>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>

static const QStringList IMAGE_EXTS = {
    "jpg", "jpeg", "png", "bmp", "gif", "webp", "tif", "tiff", "ico"
};

// ── Construction ──────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Phoenix Obe");
    setMinimumSize(800, 540);
    resize(1100, 720);
    setAcceptDrops(true);

    buildUI();
    setupMenus();
    updateNavBar();
}

void MainWindow::openPath(const QString &path)
{
    loadFolderFrom(path);
    if (m_current >= 0) showIndex(m_current);
}

// ── UI construction ───────────────────────────────────────────────────────────

void MainWindow::buildUI()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setSpacing(0);
    root->setContentsMargins(0, 0, 0, 0);

    // main row: left panel + image view
    auto *row = new QHBoxLayout;
    row->setSpacing(0);
    row->setContentsMargins(0, 0, 0, 0);

    m_editPanel = new EditPanel;
    m_view      = new EditableImageView;

    row->addWidget(m_editPanel);

    // thin separator line
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    row->addWidget(sep);

    row->addWidget(m_view, 1);

    root->addLayout(row, 1);
    root->addWidget(buildNavBar());

    // ── connect EditPanel signals ─────────────────────────────────────────────
    connect(m_editPanel, &EditPanel::hueChanged, this, [this](int v){
        m_hue = v;
        rebuildImage();
    });
    connect(m_editPanel, &EditPanel::saturationChanged, this, [this](int v){
        m_sat = v;
        rebuildImage();
    });
    connect(m_editPanel, &EditPanel::effectRequested, this, [this](const QString &key){
        if      (key == "grayscale") m_effect = Effect::Grayscale;
        else if (key == "sepia")     m_effect = Effect::Sepia;
        else if (key == "blur")      m_effect = Effect::Blur;
        else if (key == "sharpen")   m_effect = Effect::Sharpen;
        else if (key == "invert")    m_effect = Effect::Invert;
        else                         m_effect = Effect::None;
        rebuildImage();
    });
    connect(m_editPanel, &EditPanel::cropStartRequested, this, [this]{
        m_view->setCropMode(true);
    });
    connect(m_editPanel, &EditPanel::cropApplyRequested, this, [this]{
        const QRect rect = m_view->cropRectInImage();
        if (rect.isValid() && !m_originalImage.isNull()) {
            m_cropRect     = rect;
            m_originalImage = m_originalImage.copy(rect);
            m_cropRect     = {};
        }
        m_view->setCropMode(false);
        m_view->clearCropRect();
        rebuildImage();
    });
    connect(m_editPanel, &EditPanel::cropCancelRequested, this, [this]{
        m_view->setCropMode(false);
        m_view->clearCropRect();
    });
    connect(m_editPanel, &EditPanel::exportRequested, this, [this](int quality){
        if (m_originalImage.isNull()) return;

        const QString filter =
            "JPEG (*.jpg *.jpeg);;"
            "PNG (*.png);;"
            "BMP (*.bmp);;";
        const QString path = QFileDialog::getSaveFileName(
            this, "Export Image", {}, filter);
        if (path.isEmpty()) return;

        // build the current processed image
        QImage img = m_originalImage;
        if (!m_cropRect.isNull()) img = img.copy(m_cropRect);
        img = adjustHueSaturation(img, m_hue, m_sat);
        img = applyEffect(img, m_effect);

        const QString ext = QFileInfo(path).suffix().toLower();
        const int q = (ext == "jpg" || ext == "jpeg") ? quality : -1;
        if (!img.save(path, nullptr, q)) {
            QMessageBox::warning(this, "Export Failed",
                "Could not save the image to:\n" + path);
        }
    });
    connect(m_editPanel, &EditPanel::resetRequested, this, [this]{
        m_hue    = 0;
        m_sat    = 0;
        m_effect = Effect::None;
        m_cropRect = {};
        // reload original from disk
        if (m_current >= 0 && !m_files.isEmpty()) {
            m_originalImage = QImage(m_files[m_current]);
        }
        rebuildImage();
    });

    connect(m_view, &EditableImageView::zoomChanged, this, [this](int pct){
        m_zoomLabel->setText(QString("%1%").arg(pct));
        statusBar()->showMessage(
            m_view->hasImage()
                ? QString("%1 × %2 px   |   %3%")
                      .arg(m_view->imageSize().width())
                      .arg(m_view->imageSize().height())
                      .arg(pct)
                : QString());
    });
}

QWidget *MainWindow::buildNavBar()
{
    auto *bar    = new QWidget;
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(4);
    bar->setFixedHeight(44);

    m_prevBtn = new QPushButton("◀");
    m_prevBtn->setFixedWidth(32);
    m_prevBtn->setToolTip("Previous  (←)");

    m_infoLabel = new QLabel("No file open");
    m_infoLabel->setAlignment(Qt::AlignCenter);

    m_nextBtn = new QPushButton("▶");
    m_nextBtn->setFixedWidth(32);
    m_nextBtn->setToolTip("Next  (→)");

    m_zoomLabel = new QLabel("---");
    m_zoomLabel->setFixedWidth(52);
    m_zoomLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(m_prevBtn);
    layout->addWidget(m_infoLabel, 1);
    layout->addWidget(m_nextBtn);
    layout->addSpacing(8);
    layout->addWidget(m_zoomLabel);

    connect(m_prevBtn, &QPushButton::clicked, this, &MainWindow::prevFile);
    connect(m_nextBtn, &QPushButton::clicked, this, &MainWindow::nextFile);

    return bar;
}

void MainWindow::setupMenus()
{
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Open...", this, &MainWindow::openDialog, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", qApp, &QApplication::quit, QKeySequence::Quit);

    QMenu *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction("Zoom &In",       m_view, &EditableImageView::zoomIn,
                        QKeySequence::ZoomIn);
    viewMenu->addAction("Zoom &Out",      m_view, &EditableImageView::zoomOut,
                        QKeySequence::ZoomOut);
    viewMenu->addAction("&Actual Size",   m_view, &EditableImageView::actualSize,
                        QKeySequence(Qt::CTRL | Qt::Key_0));
    viewMenu->addAction("&Fit to Window", m_view, &EditableImageView::fitToWindow,
                        QKeySequence(Qt::Key_F));
    viewMenu->addSeparator();
    viewMenu->addAction("&Previous", this, &MainWindow::prevFile,
                        QKeySequence(Qt::Key_Left));
    viewMenu->addAction("&Next",     this, &MainWindow::nextFile,
                        QKeySequence(Qt::Key_Right));
    viewMenu->addSeparator();
    viewMenu->addAction("F&ullscreen", this, [this]{
        isFullScreen() ? showNormal() : showFullScreen();
    }, QKeySequence(Qt::Key_F11));

    QMenu *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About Phoenix Obe...", this, [this]{
        QMessageBox::about(this, "About Phoenix Obe",
            "<b>Phoenix Obe</b> v1.0<br>"
            "Part of <b>Obe Office</b><br><br>"
            "An image editor with hue, saturation,<br>"
            "effects, crop, and export.");
    });
}

// ── Navigation ────────────────────────────────────────────────────────────────

void MainWindow::openDialog()
{
    const QString path = QFileDialog::getOpenFileName(this, "Open Image", {},
        "Images (*.jpg *.jpeg *.png *.bmp *.gif *.webp *.tif *.tiff *.ico);;"
        "All Files (*)");
    if (!path.isEmpty()) openPath(path);
}

void MainWindow::prevFile()
{
    if (m_current > 0) showIndex(m_current - 1);
}

void MainWindow::nextFile()
{
    if (m_current < m_files.size() - 1) showIndex(m_current + 1);
}

void MainWindow::loadFolderFrom(const QString &filePath)
{
    const QString absPath = QFileInfo(filePath).absoluteFilePath();
    const QString dir     = QFileInfo(absPath).absolutePath();

    m_files.clear();
    QStringList filters;
    for (const auto &e : IMAGE_EXTS) filters << "*." + e;

    const auto entries = QDir(dir).entryInfoList(filters, QDir::Files, QDir::Name);
    for (const auto &fi : entries)
        m_files << fi.absoluteFilePath();

    m_current = -1;
    for (int i = 0; i < m_files.size(); ++i) {
        if (QDir::cleanPath(m_files[i]).compare(
                QDir::cleanPath(absPath), Qt::CaseInsensitive) == 0) {
            m_current = i;
            break;
        }
    }
    if (m_current < 0) {
        m_files.prepend(absPath);
        m_current = 0;
    }
}

void MainWindow::showIndex(int index)
{
    if (index < 0 || index >= m_files.size()) return;
    m_current = index;

    m_originalImage = QImage(m_files[m_current]);
    if (m_originalImage.isNull()) {
        statusBar()->showMessage("Failed to load: " + QFileInfo(m_files[m_current]).fileName());
        return;
    }

    m_hue      = 0;
    m_sat      = 0;
    m_effect   = Effect::None;
    m_cropRect = {};
    m_editPanel->resetControls();
    m_view->clearView();
    m_view->setCropMode(false);

    rebuildImage();
    setWindowTitle(QFileInfo(m_files[m_current]).fileName() + " — Phoenix Obe");
    updateNavBar();
}

void MainWindow::updateNavBar()
{
    const bool hasFile = m_current >= 0 && !m_files.isEmpty();
    m_infoLabel->setText(hasFile
        ? QString("%1   (%2 / %3)")
              .arg(QFileInfo(m_files[m_current]).fileName())
              .arg(m_current + 1).arg(m_files.size())
        : "No file open");
    m_prevBtn->setEnabled(m_current > 0);
    m_nextBtn->setEnabled(hasFile && m_current < m_files.size() - 1);
}

bool MainWindow::isImage(const QString &path) const
{
    return IMAGE_EXTS.contains(QFileInfo(path).suffix().toLower());
}

// ── Image processing ──────────────────────────────────────────────────────────

void MainWindow::rebuildImage()
{
    if (m_originalImage.isNull()) return;

    QImage img = m_originalImage;
    if (!m_cropRect.isNull()) img = img.copy(m_cropRect);
    img = adjustHueSaturation(img, m_hue, m_sat);
    img = applyEffect(img, m_effect);

    m_view->updatePixmap(QPixmap::fromImage(img));
}

QImage MainWindow::adjustHueSaturation(const QImage &src, int hueDelta, int satDelta)
{
    if (hueDelta == 0 && satDelta == 0) return src;

    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    const int w = img.width(), h = img.height();

    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QColor c(line[x]);
            int hue = c.hslHue();
            int sat = c.hslSaturation();
            int lit = c.lightness();
            int alpha = c.alpha();

            if (hue != -1)
                hue = (hue + hueDelta + 360) % 360;

            sat = qBound(0, sat + satDelta * 255 / 100, 255);

            c.setHsl(hue == -1 ? 0 : hue, sat, lit, alpha);
            line[x] = c.rgba();
        }
    }
    return img;
}

QImage MainWindow::applyEffect(const QImage &src, Effect effect)
{
    if (effect == Effect::None) return src;

    if (effect == Effect::Invert) {
        QImage img = src.convertToFormat(QImage::Format_ARGB32);
        img.invertPixels(QImage::InvertRgb);
        return img;
    }

    if (effect == Effect::Grayscale) {
        return src.convertToFormat(QImage::Format_Grayscale8)
                  .convertToFormat(QImage::Format_ARGB32);
    }

    if (effect == Effect::Sepia) {
        QImage img = src.convertToFormat(QImage::Format_ARGB32);
        const int w = img.width(), h = img.height();
        for (int y = 0; y < h; ++y) {
            QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
            for (int x = 0; x < w; ++x) {
                const int r = qRed(line[x]);
                const int g = qGreen(line[x]);
                const int b = qBlue(line[x]);
                const int a = qAlpha(line[x]);
                line[x] = qRgba(
                    qMin(255, (int)(r * 0.393 + g * 0.769 + b * 0.189)),
                    qMin(255, (int)(r * 0.349 + g * 0.686 + b * 0.168)),
                    qMin(255, (int)(r * 0.272 + g * 0.534 + b * 0.131)),
                    a);
            }
        }
        return img;
    }

    if (effect == Effect::Blur) {
        return applyBlur(src, 3);
    }

    if (effect == Effect::Sharpen) {
        QImage img = src.convertToFormat(QImage::Format_ARGB32);
        const QImage orig = img;
        const int w = img.width(), h = img.height();
        // kernel: 0 -1 0 / -1 5 -1 / 0 -1 0
        const int kx[] = { 0,  1, 0, -1,  0,  1,  0, -1 };
        const int ky[] = {-1,  0, 1,  0, -1,  0,  1,  0 };
        for (int y = 1; y < h - 1; ++y) {
            QRgb *dst = reinterpret_cast<QRgb *>(img.scanLine(y));
            for (int x = 1; x < w - 1; ++x) {
                auto px = [&](int dx, int dy) -> QRgb {
                    return reinterpret_cast<const QRgb *>(orig.constScanLine(y + dy))[x + dx];
                };
                const int r = qBound(0, 5*qRed(px(0,0))   - qRed(px(-1,0))   - qRed(px(1,0))   - qRed(px(0,-1))   - qRed(px(0,1)),   255);
                const int g = qBound(0, 5*qGreen(px(0,0)) - qGreen(px(-1,0)) - qGreen(px(1,0)) - qGreen(px(0,-1)) - qGreen(px(0,1)), 255);
                const int b = qBound(0, 5*qBlue(px(0,0))  - qBlue(px(-1,0))  - qBlue(px(1,0))  - qBlue(px(0,-1))  - qBlue(px(0,1)),  255);
                dst[x] = qRgba(r, g, b, qAlpha(px(0, 0)));
            }
        }
        return img;
    }

    return src;
}

QImage MainWindow::applyBlur(const QImage &src, int radius)
{
    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    const int w = img.width(), h = img.height();
    QImage tmp(w, h, QImage::Format_ARGB32);

    // horizontal pass
    for (int y = 0; y < h; ++y) {
        const QRgb *s = reinterpret_cast<const QRgb *>(img.constScanLine(y));
        QRgb       *d = reinterpret_cast<QRgb *>(tmp.scanLine(y));
        for (int x = 0; x < w; ++x) {
            int r=0,g=0,b=0,a=0,n=0;
            for (int dx = -radius; dx <= radius; ++dx) {
                QRgb c = s[qBound(0, x+dx, w-1)];
                r+=qRed(c); g+=qGreen(c); b+=qBlue(c); a+=qAlpha(c); ++n;
            }
            d[x] = qRgba(r/n, g/n, b/n, a/n);
        }
    }

    // vertical pass
    QImage result(w, h, QImage::Format_ARGB32);
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            int r=0,g=0,b=0,a=0,n=0;
            for (int dy = -radius; dy <= radius; ++dy) {
                QRgb c = reinterpret_cast<const QRgb *>(
                    tmp.constScanLine(qBound(0, y+dy, h-1)))[x];
                r+=qRed(c); g+=qGreen(c); b+=qBlue(c); a+=qAlpha(c); ++n;
            }
            reinterpret_cast<QRgb *>(result.scanLine(y))[x] = qRgba(r/n, g/n, b/n, a/n);
        }
    }
    return result;
}

// ── Events ────────────────────────────────────────────────────────────────────

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && isFullScreen()) {
        showNormal();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const auto urls = event->mimeData()->urls();
    if (!urls.isEmpty()) openPath(urls.first().toLocalFile());
}
