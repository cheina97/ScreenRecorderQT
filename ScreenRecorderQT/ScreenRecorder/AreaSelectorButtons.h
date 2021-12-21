#ifndef AREASELECTORBUTTONS_H
#define AREASELECTORBUTTONS_H

#include <QPainter>
#include <QPainterPath>

class AreaSelectorButtons : public QObject
{
Q_OBJECT
public:
    AreaSelectorButtons();
    virtual ~AreaSelectorButtons();
    enum degreeArrow { topMiddle=0, topRight=45, rightMiddle=90, bottomRight=135,
                       bottomMiddle=180, bottomLeft=225, leftMiddle=270, topLeft=315 };

private:
  int penWidth = 2;
  int penWidthHalf = penWidth/2;
  int radius = 20;
  int diameter = 2 * radius;
  QColor color = Qt::yellow; //button background
  QColor colorSelected = Qt::black; //arrow color

public slots:
  QPixmap getPixmapHandle(degreeArrow degree ); //creates a pixmap to use somewhere else
  QPixmap getButton( );                         //draws the ellipse
  QPixmap getArrow(degreeArrow degree);         //draws the arrow in the button
  int getWithHalf();

private slots:

protected:

};

#endif // AREASELECTORBUTTONS_H
