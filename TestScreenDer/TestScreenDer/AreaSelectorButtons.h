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

public slots:
  QPixmap getPixmapHandle(QColor color, QColor colorSelected, degreeArrow degree );
  QPixmap getButton( QColor color );
  QPixmap getArrow(degreeArrow degree , QColor colorSelected);
  int getWithHalf();

private slots:

protected:

};

#endif // AREASELECTORBUTTONS_H
