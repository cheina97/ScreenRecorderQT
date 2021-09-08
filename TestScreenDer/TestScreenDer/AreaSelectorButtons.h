#ifndef AREASELECTORBUTTONS_H
#define AREASELECTORBUTTONS_H

#include <QPainter>

class AreaSelectorButtons : public QObject
{
Q_OBJECT
public:
    AreaSelectorButtons();
    virtual ~AreaSelectorButtons();
    enum degreeArrow { topMiddle=0, topRight=45, rightMiddle=90, bottomRight=135,
                       bottomMiddel=180, bottomLeft=225, leftMiddel=270, topLeft=315 };

//TODO:: rimuovere le freccie dei latini

private:
  int penWidth = 2;
  int penWidthHalf = penWidth/2;
  int radius = 20;
  int diameter = 2 * radius;
  QColor color = Qt::yellow; //button background
  QColor colorSelected = Qt::black; //arrow color

public slots:
  QPixmap getPixmapHandle(degreeArrow degree );
  QPixmap getButton( );
  QPixmap getArrow(degreeArrow degree);
  int getWithHalf();

private slots:

protected:

};

#endif // AREASELECTORBUTTONS_H
