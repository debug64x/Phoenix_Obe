#include "EditableImageView.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QPen>
#include <QScrollBar>
#include <QWheelEvent>

static constexpr double ZOOM_MIN  = 0.02;
static constexpr double ZOOM_MAX  = 32.0;
static constexpr double ZOOM_STEP = 1.25;

EditableImageView::EditableImageView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setFrameShape(QFrame::NoFrame);
    setBackgroundBrush(QColor(28, 28, 28));
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
}

void EditableImageView::setPixmap(const QPixmap &pixmap)
{
    m_cropDragging = false;
    m_panning      = false;
    m_scene->clear();
    m_item     = nullptr;
    m_cropItem = nullptr;
    m_fitMode  = true;

    m_item = m_scene->addPixmap(pixmap);
    m_scene->setSceneRect(m_item->boundingRect());
    fitToWindow();
}

void EditableImageView::updatePixmap(const QPixmap &pixmap)
{
    if (!m_item) {
        setPixmap(pixmap);
        return;
    }
    m_item->setPixmap(pixmap);
    m_scene->setSceneRect(m_item->boundingRect());
    if (m_cropItem) m_cropItem->setZValue(10);
}

void EditableImageView::clearView()
{
    m_cropDragging = false;
    m_panning      = false;
    m_scene->clear();
    m_item     = nullptr;
    m_cropItem = nullptr;
    m_fitMode  = true;
}

QSize EditableImageView::imageSize() const
{
    return m_item ? m_item->pixmap().size() : QSize();
}

void EditableImageView::setCropMode(bool on)
{
    m_cropMode = on;
    if (!on) {
        m_cropDragging = false;
    }
    setCursor(on ? Qt::CrossCursor : Qt::ArrowCursor);
}

QRect EditableImageView::cropRectInImage() const
{
    if (!m_cropItem || !m_item) return {};
    const QRectF imgRect = m_item->boundingRect();
    const QRectF crop    = m_cropItem->rect().normalized();
    return crop.intersected(imgRect).toRect();
}

void EditableImageView::clearCropRect()
{
    if (m_cropItem) {
        m_scene->removeItem(m_cropItem);
        delete m_cropItem;
        m_cropItem = nullptr;
    }
    m_cropDragging = false;
}

void EditableImageView::fitToWindow()
{
    if (!m_item) return;
    QGraphicsView::fitInView(m_item, Qt::KeepAspectRatio);
    m_zoom    = transform().m11();
    m_fitMode = true;
    emit zoomChanged(qRound(m_zoom * 100));
}

void EditableImageView::actualSize()
{
    applyZoom(1.0);
    m_fitMode = false;
}

void EditableImageView::zoomIn()
{
    m_fitMode = false;
    applyZoom(m_zoom * ZOOM_STEP);
}

void EditableImageView::zoomOut()
{
    m_fitMode = false;
    applyZoom(m_zoom / ZOOM_STEP);
}

void EditableImageView::applyZoom(double z)
{
    m_zoom = qBound(ZOOM_MIN, z, ZOOM_MAX);
    setTransform(QTransform::fromScale(m_zoom, m_zoom));
    emit zoomChanged(qRound(m_zoom * 100));
}

void EditableImageView::wheelEvent(QWheelEvent *event)
{
    if (!m_item) { event->ignore(); return; }
    m_fitMode = false;
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    const double factor = event->angleDelta().y() > 0 ? ZOOM_STEP : 1.0 / ZOOM_STEP;
    applyZoom(m_zoom * factor);
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    event->accept();
}

void EditableImageView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);
    if (m_fitMode && m_item) fitToWindow();
}

void EditableImageView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_cropMode && m_item) {
            clearCropRect();                          // resets m_cropDragging → false
            m_cropAnchor   = mapToScene(event->pos());
            m_cropDragging = true;                    // set AFTER clear
            m_cropItem = m_scene->addRect(QRectF(m_cropAnchor, QSizeF()),
                QPen(Qt::white, 1.0 / m_zoom, Qt::DashLine),
                QBrush(QColor(0, 120, 215, 40)));
            m_cropItem->setZValue(10);
            event->accept();
            return;
        }
        m_panStart = event->pos();
        m_panning  = true;
        setCursor(Qt::ClosedHandCursor);
    }
    QGraphicsView::mousePressEvent(event);
}

void EditableImageView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_cropDragging) {
            m_cropDragging = false;
            updateCropOverlay(mapToScene(event->pos()));
            event->accept();
            return;
        }
        m_panning = false;
        setCursor(m_cropMode ? Qt::CrossCursor : Qt::ArrowCursor);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void EditableImageView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_cropDragging) {
        updateCropOverlay(mapToScene(event->pos()));
        event->accept();
        return;
    }
    if (m_panning) {
        const QPoint delta = event->pos() - m_panStart;
        m_panStart = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    }
    QGraphicsView::mouseMoveEvent(event);
}

void EditableImageView::updateCropOverlay(const QPointF &scenePos)
{
    if (!m_cropItem) return;
    const QRectF imgRect = m_item ? m_item->boundingRect() : QRectF();
    QPointF clamped(
        qBound(imgRect.left(),  scenePos.x(), imgRect.right()),
        qBound(imgRect.top(),   scenePos.y(), imgRect.bottom())
    );
    m_cropItem->setRect(QRectF(m_cropAnchor, clamped).normalized());
    // keep pen width at 1 screen pixel regardless of zoom
    QPen pen = m_cropItem->pen();
    pen.setWidthF(1.0 / m_zoom);
    m_cropItem->setPen(pen);
}
