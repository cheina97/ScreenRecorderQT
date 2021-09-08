#include "AreaSelector.h"
#include "AreaSelectorButtons.h"

#include <QDebug>
#include <QPainter>
#include <QGuiApplication>
#include <QBitmap>
#include <QPaintEvent>
#include <QIcon>
#include <QRect>
#include <QPen>

AreaSelector::AreaSelector()
{

    this->framePenWidth = 4;

    frame_X =200-framePenWidth/2;
    frame_Y = 200-framePenWidth/2;
    frame_Width = 320 + framePenWidth;
    frame_height = 200 + framePenWidth;
     setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::ToolTip);
     setAttribute( Qt::WA_TranslucentBackground, true);
     setMouseTracking( true );
     setFrameColor( Qt::black );
     hide(); //setVisibile(false);
//     qDebug()<<"creato";
}

AreaSelector::~AreaSelector(){}

void AreaSelector::paintEvent( QPaintEvent *event ){
    Q_UNUSED(event);
    QPixmap pixmap( screenWidth, screenHeight);
    pixmap.fill(Qt::transparent);
    QPainter painterPixmap;
    painterPixmap.begin(&pixmap);
    painterPixmap.setRenderHints( QPainter::Antialiasing, true );
    drawFrame( painterPixmap );
    HandleMiddle( painterPixmap );

    QPainter painter;
    painter.begin( this );
    painter.drawPixmap( QPoint( 0, 0 ), pixmap );
    painter.end();

    setMask( pixmap.mask() );
}

void AreaSelector::slot_init()
{
    this->screen = QGuiApplication::primaryScreen();
    QRect geom = screen->geometry();
    this->screenWidth = geom.width();
    this->screenHeight = geom.height();
    qDebug()<<"Slot init!";
}

void AreaSelector::drawFrame(QPainter &painter)
{
    QPen pen( getFrameColor(), framePenWidth );
    pen.setJoinStyle( Qt::MiterJoin );
    painter.setPen( pen );
    QBrush brush( Qt::transparent, Qt::SolidPattern);
    painter.setBrush( brush );
    painter.drawRect( frame_X,
                      frame_Y,
                      frame_Width,
                      frame_height);
}

QColor AreaSelector::getFrameColor()
{
    return frameColor;
}
void AreaSelector::setFrameColor( QColor color )
{
    frameColor = color;
}

void AreaSelector::HandleMiddle( QPainter &painter )
{
    AreaSelectorButtons buttonArrow;
    QColor color = Qt::yellow;

    QColor colorSelected = Qt::black;


    painter.drawPixmap( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                        frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                        buttonArrow.getButton( color) );

    painter.drawPixmap( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                        frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                        buttonArrow.getArrow( buttonArrow.degreeArrow::topMiddle, colorSelected ) );

    painter.drawPixmap( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                        frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                        buttonArrow.getArrow( buttonArrow.degreeArrow::rightMiddle, colorSelected ) );

    painter.drawPixmap( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                        frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                        buttonArrow.getArrow( buttonArrow.degreeArrow::bottomMiddel, colorSelected ) );

    painter.drawPixmap( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                        frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                        buttonArrow.getArrow( buttonArrow.degreeArrow::leftMiddel, colorSelected ) );
}


void AreaSelector::mousePressEvent(QMouseEvent *event)
{
    if( event->button() != Qt::LeftButton)
    {
        return;
    }

    switch ( handleUnderMouse )
    {
      case NoHandle    : handlePressed = NoHandle;     break;
      case TopLeft     : { handlePressed = TopLeft;      HandleSelected = TopLeft;      break; }
      case TopMiddle   : { handlePressed = TopMiddle;    HandleSelected = TopMiddle;    break; }
      case TopRight    : { handlePressed = TopRight;     HandleSelected = TopRight;     break; }
      case RightMiddle : { handlePressed = RightMiddle;  HandleSelected = RightMiddle;  break; }
      case BottomRight : { handlePressed = BottomRight;  HandleSelected = BottomRight;  break; }
      case BottomMiddle: { handlePressed = BottomMiddle; HandleSelected = BottomMiddle; break; }
      case BottomLeft  : { handlePressed = BottomLeft;   HandleSelected = BottomLeft;   break; }
      case LeftMiddle  : { handlePressed = LeftMiddle;   HandleSelected = LeftMiddle;   break; }
      case Middle      : { handlePressed = Middle;       HandleSelected = Middle;       break; }
    }

    mous_delta_X_to_blueline = event->x() - frame_X;
    mous_delta_Y_to_blueline = event->y() - frame_Y;

    old_Mouse_X = event->x();
    old_Mouse_Y = event->y();
    old_Frame_Width = frame_Width;
    old_Frame_Height = frame_height;

    old_Frame_X2 = frame_X + frame_Width;
    old_Frame_Y2 = frame_Y + frame_height;

      repaint();
      update();

}

void AreaSelector::mouseReleaseEvent( QMouseEvent * event )
{
  if( event->button() != Qt::LeftButton)
  {
      return;
  }

  handlePressed = NoHandle;

  update();
}

void AreaSelector::mouseMoveEvent( QMouseEvent *event )
{
    switch ( handlePressed )
    {
      case NoHandle    : break;
      case TopLeft     : { // Move
                           frame_X = event->x() - mous_delta_X_to_blueline;
                           frame_Y = event->y() - mous_delta_Y_to_blueline;
                           frame_Width = old_Mouse_X - event->x() + old_Frame_Width;
                           frame_height = old_Mouse_Y - event->y() + old_Frame_Height;

                           // Limit min
                           if ( frame_Width < frame_min_width )
                           {
                             frame_X = old_Frame_X2 - frame_min_width;
                             frame_Width = frame_min_width;
                           }

                           if ( frame_height < frame_min_height )
                           {
                             frame_Y = old_Frame_Y2 - frame_min_height;
                             frame_height = frame_min_height;
                           }

                           // Limit max
                           if ( frame_Y <= 0 - framePenHalf )
                           {
                             frame_Y = 0 - framePenHalf;
                             frame_height = old_Frame_Y2 + framePenHalf;
                           }

                           if ( frame_X <= 0 - framePenHalf )
                           {
                              frame_X = 0 - framePenHalf;
                              frame_Width = old_Frame_X2 + framePenHalf;
                           }

                           break;
                         }
      case TopMiddle   : { // Move
                           frame_Y = event->y() - mous_delta_Y_to_blueline;
                           frame_height = old_Mouse_Y - event->y() + old_Frame_Height;

                           // Limit min
                           if ( frame_height < frame_min_height )
                           {
                             frame_Y = old_Frame_Y2 - frame_min_height;
                             frame_height = frame_min_height;
                           }

                           // Limit max
                           if ( frame_Y <= 0 - framePenHalf )
                           {
                             frame_Y = 0 - framePenHalf;
                             frame_height = old_Frame_Y2 + framePenHalf;
                           }

                           break;
                         }
      case TopRight    : { // Move
                           frame_Y = event->y() - mous_delta_Y_to_blueline;
                           frame_Width = event->x() - old_Mouse_X + old_Frame_Width;
                           frame_height = old_Mouse_Y - event->y() + old_Frame_Height;;

                           // Limit min
                           if ( frame_Width < frame_min_width )
                           {
                             frame_Width = frame_min_width;
                           }

                           if ( frame_height < frame_min_height )
                           {
                             frame_Y = old_Frame_Y2 - frame_min_height;
                             frame_height = frame_min_height;
                           }

                           // Limit max
                           if ( frame_Y <= 0 - framePenHalf )
                           {
                             frame_Y = 0 - framePenHalf;
                             frame_height = old_Frame_Y2 + framePenHalf;
                           }

                           if( ( frame_X + frame_Width - framePenHalf ) > screenWidth )
                           {
                             frame_Width = screenWidth + framePenHalf - frame_X;
                           }

                           break;
                         }
      case RightMiddle : { // Move
                           frame_Width = event->x() - old_Mouse_X + old_Frame_Width;

                           // Limit min
                           if ( frame_Width < frame_min_width )
                           {
                             frame_Width = frame_min_width;
                           }

                           // Limit max
                           if( ( frame_X + frame_Width - framePenHalf ) > screenWidth )
                           {
                             frame_Width = screenWidth + framePenHalf - frame_X;
                           }

                           break;
                         }
      case BottomRight : { // Move
                           frame_Width = event->x() - old_Mouse_X + old_Frame_Width;
                           frame_height = event->y() - old_Mouse_Y + old_Frame_Height;

                           // Limit min
                           if ( frame_Width < frame_min_width )
                           {
                             frame_Width = frame_min_width;
                           }

                           if ( frame_height < frame_min_height )
                           {
                             frame_height = frame_min_height;
                           }

                           //Limit max
                           if( ( frame_X + frame_Width - framePenHalf ) > screenWidth )
                           {
                             frame_Width = screenWidth + framePenHalf - frame_X;
                           }

                           if( ( frame_Y + frame_height - framePenHalf ) > screenHeight )
                           {
                             frame_height = screenHeight + framePenHalf - frame_Y;
                           }

                           break;
                         }
      case BottomMiddle: { // Move
                           frame_height = event->y() - old_Mouse_Y + old_Frame_Height;

                           // Limit min
                           if ( frame_height < frame_min_height )
                           {
                             frame_height = frame_min_height;
                           }

                           //Limit max
                           if( ( frame_Y + frame_height - framePenHalf ) > screenHeight )
                           {
                             frame_height = screenHeight + framePenHalf - frame_Y;
                           }

                           break;
                         }
    case BottomLeft    : {
                           // Move
                           frame_X = event->x() - mous_delta_X_to_blueline;
                           frame_height = event->y() - old_Mouse_Y + old_Frame_Height;
                           frame_Width = old_Mouse_X - event->x() + old_Frame_Width;

                           // Limit min
                           if ( frame_Width < frame_min_width )
                           {
                             frame_X = old_Frame_X2 - frame_min_width;
                             frame_Width = frame_min_width;
                           }

                           if ( frame_height < frame_min_height )
                           {
                             frame_height = frame_min_height;
                           }

                           // Limit max
                           if ( frame_X <= 0 - framePenHalf )
                           {
                              frame_X = 0 - framePenHalf;
                              frame_Width = old_Frame_X2 + framePenHalf;
                           }

                           if( ( frame_Y + frame_height - framePenHalf ) > screenHeight )
                           {
                             frame_height = screenHeight + framePenHalf - frame_Y;
                           }

                           break;
                         }
      case LeftMiddle  : { // Move
                           frame_X = event->x() - mous_delta_X_to_blueline;
                           frame_Width = old_Mouse_X - event->x() + old_Frame_Width;

                           // Limit min
                           if ( frame_Width < frame_min_width )
                           {
                             frame_X = old_Frame_X2 - frame_min_width;
                             frame_Width = frame_min_width;
                           }

                           // Limit max
                           if ( frame_X <= 0 - framePenHalf )
                           {
                              frame_X = 0 - framePenHalf;
                              frame_Width = old_Frame_X2 + framePenHalf;
                           }

                           break;
                         }
      case Middle      : { // Move
                           int deltaX = ( old_Frame_X2 - framePenHalf - frame_Width/2 ) - old_Mouse_X;
                           int deltaY = ( old_Frame_Y2 - framePenHalf - frame_height/2 ) - old_Mouse_Y;
                           frame_X = event->x() - frame_Width/2 + framePenHalf + deltaX;
                           frame_Y = event->y() - frame_height/2 + framePenHalf + deltaY;

                           // Limit Top
                           if ( frame_Y <= 0 - framePenHalf )
                           {
                             frame_Y = 0 - framePenHalf;
                           }

                           // Limit Left
                           if ( frame_X <= 0 - framePenHalf )
                           {
                             frame_X = 0 - framePenHalf;
                           }

                           // Limit Right
                           if( ( frame_X + frame_Width - framePenHalf ) > screenWidth )
                           {
                               frame_X = screenWidth - frame_Width + framePenHalf;
                           }

                           // Limit Bottom
                           if( ( frame_Y + frame_height - framePenHalf ) > screenHeight )
                           {
                               frame_Y = screenHeight - frame_height + framePenHalf;
                           }

                           break;
                         }
    } // end switch

    repaint();
    update();

    if ( handlePressed != NoHandle )
        return;

    QRect regionTopLeft( frame_X - radius - 1, frame_Y - radius - 1, diameter + 2, diameter + 2 );
    if ( regionTopLeft.contains( event->pos() ) )
    {
        QPixmap pixmap( ":/pictures/cursor/size_fdiag.png" );
        QCursor cursor( pixmap );
        setCursor( cursor );
        handleUnderMouse = TopLeft;
        return;
    }

    QRect regionTopMiddle( frame_X + frame_Width/2 - radius - 1, frame_Y - radius - 1, diameter + 2, diameter + 2 );
    if ( regionTopMiddle.contains( event->pos() )  )
    {
        QPixmap pixmap( ":/pictures/cursor/size_ver.png" );
        QCursor cursor( pixmap );
        setCursor( cursor );
        handleUnderMouse = TopMiddle;
        return;
    }

    QRect regionTopRight( frame_X + frame_Width - radius - 1, frame_Y - radius - 1, diameter + 2, diameter + 2 );
    if ( regionTopRight.contains( event->pos() )  )
    {
        QPixmap pixmap( ":/pictures/cursor/size_bdiag.png" );
        QCursor cursor( pixmap );
        setCursor( cursor );
        handleUnderMouse = TopRight;
        return;
    }

    QRect regionRightMiddle( frame_X + frame_Width - radius - 1, frame_Y + frame_height/2 - radius - 1, diameter + 2, diameter + 2 );
    if ( regionRightMiddle.contains( event->pos() )  )
    {
        QPixmap pixmap( ":/pictures/cursor/size_hor.png" );
        QCursor cursor( pixmap );
        setCursor( cursor );
        handleUnderMouse = RightMiddle;
        return;
    }

    QRect regionMiddle( frame_X + frame_Width/2 - radius - penHalf, frame_Y + frame_height/2 - radius - penHalf, 2 * radius + penWidth, 2 * radius + penWidth);
    if ( regionMiddle.contains( event->pos() )  )
    {
        QPixmap pixmap( ":/pictures/cursor/size_all.png" );
        QCursor cursor( pixmap );
        setCursor( cursor );
        handleUnderMouse = Middle;
        return;
    }

    AreaSelectorButtons buttonArrow;
    QRect regionBottomRight( frame_X + frame_Width - buttonArrow.getWithHalf(),
                               frame_Y + frame_height - buttonArrow.getWithHalf(),
                               buttonArrow.getWithHalf()*2,
                               buttonArrow.getWithHalf()*2
                              );
    if( regionBottomRight.contains( event->pos()) )
    {
        QPixmap pixmap( ":/pictures/cursor/size_fdiag.png" ); //FIXMEEEEE
        QCursor cursor( pixmap );
        setCursor( cursor );
        handleUnderMouse = BottomRight;
        return;
    }

    QRect regionBottomMiddle( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                                frame_Y + frame_height - buttonArrow.getWithHalf(),
                                buttonArrow.getWithHalf()*2,
                                buttonArrow.getWithHalf()*2
                               );
    if( regionBottomMiddle.contains( event->pos()) )
    {
        QPixmap pixmap( ":/pictures/cursor/size_ver.png" );
        QCursor cursor( pixmap );
        setCursor( cursor );
        handleUnderMouse = BottomMiddle;
        return;
    }

    QRect regionBottomLeft( frame_X - buttonArrow.getWithHalf(),
                              frame_Y + frame_height - buttonArrow.getWithHalf(),
                              buttonArrow.getWithHalf()*2,
                              buttonArrow.getWithHalf()*2
                            );
    if( regionBottomLeft.contains( event->pos()) )
    {
        QPixmap pixmap( ":/pictures/cursor/size_bdiag.png" );
        QCursor cursor( pixmap );
        setCursor( cursor );
        handleUnderMouse = BottomLeft;
        return;
    }

    QRect regionLeftMiddle( frame_X - buttonArrow.getWithHalf(),
                              frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                              buttonArrow.getWithHalf()*2,
                              buttonArrow.getWithHalf()*2
                            );
    if( regionLeftMiddle.contains( event->pos()) )
    {
        QPixmap pixmap( ":/pictures/cursor/size_hor.png" );
        QCursor cursor( pixmap );
        setCursor( cursor );
        handleUnderMouse = LeftMiddle;
        return;
    }

    unsetCursor();
    handleUnderMouse = NoHandle;
}
