#pragma once

#include <QGraphicsView>

class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsRectItem;

class EditableImageView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit EditableImageView(QWidget *parent = nullptr);

    void setPixmap(const QPixmap &pixmap);   // full reset (new image load)
    void updatePixmap(const QPixmap &pixmap); // preserves zoom & crop overlay
    void clearView();

    bool   hasImage()  const { return m_item != nullptr; }
    QSize  imageSize() const;
    double zoom()      const { return m_zoom; }

    // Crop
    void  setCropMode(bool on);
    bool  isCropMode() const { return m_cropMode; }
    QRect cropRectInImage() const;
    void  clearCropRect();

    void fitToWindow();
    void actualSize();
    void zoomIn();
    void zoomOut();

signals:
    void zoomChanged(int percent);

protected:
    void wheelEvent(QWheelEvent *event)        override;
    void resizeEvent(QResizeEvent *event)      override;
    void mousePressEvent(QMouseEvent *event)   override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event)    override;

private:
    void applyZoom(double z);
    void updateCropOverlay(const QPointF &scenePos);

    QGraphicsScene      *m_scene;
    QGraphicsPixmapItem *m_item     = nullptr;
    QGraphicsRectItem   *m_cropItem = nullptr;

    double  m_zoom        = 1.0;
    bool    m_fitMode     = true;
    bool    m_cropMode    = false;

    QPoint  m_panStart;
    bool    m_panning     = false;

    QPointF m_cropAnchor;
    bool    m_cropDragging = false;
};
