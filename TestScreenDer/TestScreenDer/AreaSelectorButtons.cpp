#include "AreaSelectorButtons.h"

AreaSelectorButtons::AreaSelectorButtons()
{

}

AreaSelectorButtons::~AreaSelectorButtons()
{
}

QPixmap AreaSelectorButtons::getButton( QColor color )
{
    QPixmap pixmap( diameter+penWidth, diameter+penWidth );
    pixmap.fill( Qt::transparent );

    QPainter painter;
    painter.begin( &pixmap );
      painter.setRenderHints( QPainter::Antialiasing, true );
      QBrush brush;
        brush.setColor( color );
        brush.setStyle( Qt::SolidPattern );
      painter.setBrush( brush );
      QPen pen;
        pen.setColor( Qt::black );
        pen.setWidth( penWidth );
      painter.setPen( pen );
      painter.drawEllipse( penWidthHalf,
                           penWidthHalf,
                           diameter,
                           diameter );
    painter.end();

    return pixmap;
}


QPixmap AreaSelectorButtons::getArrow( degreeArrow degree, QColor colorSelected )
{
    QPixmap pixmap( diameter+penWidth, diameter+penWidth );
    pixmap.fill( Qt::transparent );

    QPainter painter;
    painter.begin( &pixmap );
        painter.setRenderHints( QPainter::Antialiasing, true );
        painter.translate((diameter+penWidth)/2, (diameter+penWidth)/2);
        painter.rotate( degree );
        QPen pen;
            pen.setCapStyle( Qt::RoundCap );
            pen.setColor( colorSelected );
            pen.setWidthF( penWidth );
            pen.setJoinStyle( Qt::RoundJoin );
        painter.setPen( pen );
        QBrush brush;
            brush.setColor( colorSelected );
            brush.setStyle( Qt::SolidPattern );
        painter.setBrush( brush );
        QPainterPath painterPath;
            painterPath.moveTo(  0, 0 );
            painterPath.lineTo(  0, -radius + penWidth );
            painterPath.lineTo( -3, -radius + penWidth + 7 );
            painterPath.lineTo(  3, -radius + penWidth + 7 );
            painterPath.lineTo(  0, -radius + penWidth );
        painter.drawPath( painterPath );
    painter.end();

    return pixmap;
}


QPixmap AreaSelectorButtons::getPixmapHandle( QColor color, QColor colorSelected ,degreeArrow degree )
{
    QPixmap pixmap( diameter+penWidth, diameter+penWidth );
    pixmap.fill( Qt::transparent );

    QPainter painter;
    painter.begin( &pixmap );
      painter.setRenderHints( QPainter::Antialiasing, true );
      painter.drawPixmap( 0, 0, getButton( color ) );
      painter.drawPixmap( 0, 0, getArrow( degree , colorSelected ) );
    painter.end();

    return pixmap;
}


int AreaSelectorButtons::getWithHalf()
{
   return ( diameter + penWidth ) / 2;
}
