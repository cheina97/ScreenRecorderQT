#include "AreaSelector.h"
#include "AreaSelectorButtons.h"

#include <QDebug>
#include <QPainter>
#include <QGuiApplication>
#include <QBitmap>
#include <QPaintEvent>
#include <QIcon>
#include <QPen>

AreaSelector::AreaSelector()
     :handlePressed(NoHandle),
      handleUnderMouse(NoHandle),
      framePenWidth(4), // framePenWidth must be an even number
      radius(20),
      penWidth(2),
      frame_Width(320 + framePenWidth), //default dimension
      frame_height(200 + framePenWidth),
      frame_min_width(320 + framePenWidth),
      frame_min_height(200 + framePenWidth),
      frameColor(Qt::black),
      colorSelectedArrow(Qt::yellow)
{
    //window settings
    setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::ToolTip);
    setAttribute( Qt::WA_TranslucentBackground, true);
    setMouseTracking( true );
    hide(); //setVisibile(false);
}

AreaSelector::~AreaSelector(){}

void AreaSelector::slot_init()
{
    screen = QGuiApplication::primaryScreen();
    resize( screen->size().width(), screen->size().height() );
    screenWidth = screen->size().width();
    screenHeight = screen->size().height();
    move( screen->geometry().x(), screen->geometry().y() );

    //positioning the rectange in the middle of the screen:
    frame_X = (screenWidth/2 - frame_Width/2)-framePenWidth/2;
    frame_Y = (screenHeight/2 - frame_height/2)-framePenWidth/2;
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

void AreaSelector::HandleMiddle( QPainter &painter )
{
    AreaSelectorButtons buttonArrow;

    painter.drawPixmap( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                        frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                        buttonArrow.getButton() );

    painter.drawPixmap( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                        frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                        buttonArrow.getArrow( buttonArrow.degreeArrow::topMiddle) );

    painter.drawPixmap( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                        frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                        buttonArrow.getArrow( buttonArrow.degreeArrow::rightMiddle) );

    painter.drawPixmap( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                        frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                        buttonArrow.getArrow( buttonArrow.degreeArrow::bottomMiddle) );

    painter.drawPixmap( frame_X + frame_Width/2 - buttonArrow.getWithHalf(),
                        frame_Y + frame_height/2 - buttonArrow.getWithHalf(),
                        buttonArrow.getArrow( buttonArrow.degreeArrow::leftMiddle) );
}

void AreaSelector::HandleTopLeft( QPainter &painter )
{
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap( frame_X - radius,
                        frame_Y - radius,
                        buttonArrow.getPixmapHandle(buttonArrow.topLeft ) );
}

void AreaSelector::HandleTopRight(QPainter &painter)
{
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap( frame_X + frame_Width - buttonArrow.getWithHalf(),
                        frame_Y - buttonArrow.getWithHalf(),
                        buttonArrow.getPixmapHandle( buttonArrow.topRight ) );
}

void AreaSelector::HandleBottomLeft( QPainter &painter )
{
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap( frame_X - buttonArrow.getWithHalf(),
                        frame_Y + frame_height - buttonArrow.getWithHalf(),
                        buttonArrow.getPixmapHandle( buttonArrow.bottomLeft ) );
}

void AreaSelector::HandleBottomRight( QPainter &painter )
{
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap( frame_X + frame_Width - buttonArrow.getWithHalf(),
                        frame_Y + frame_height - buttonArrow.getWithHalf(),
                        buttonArrow.getPixmapHandle( buttonArrow.bottomRight ) );
}

void AreaSelector::HandleRecord( QPainter &painter, int x, int y, int startAngle, int spanAngle )
{
    //this draws on top of the previous buttons
    QBrush brush;
      brush.setColor( Qt::darkGray );
      brush.setStyle( Qt::SolidPattern );
    painter.setBrush( brush );
    QPen pen;
      pen.setColor( Qt::black );
      pen.setWidth( penWidth );
    painter.setPen( pen );
    QRectF rectangle = QRectF( x,y,radius*2,radius*2);

    painter.drawPie( rectangle, startAngle, spanAngle );
}

/*////////////////////////////
// reacting to events
////////////////////////////*/

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
    if ( recordemode == true ){
        unsetCursor();
        return;
    }
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
                           if ( frame_Y <= 0 - framePenWidth/2 )
                           {
                             frame_Y = 0 - framePenWidth/2;
                             frame_height = old_Frame_Y2 + framePenWidth/2;
                           }

                           if ( frame_X <= 0 - framePenWidth/2 )
                           {
                              frame_X = 0 - framePenWidth/2;
                              frame_Width = old_Frame_X2 + framePenWidth/2;
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
                           if ( frame_Y <= 0 - framePenWidth/2 )
                           {
                             frame_Y = 0 - framePenWidth/2;
                             frame_height = old_Frame_Y2 + framePenWidth/2;
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
                           if ( frame_Y <= 0 - framePenWidth/2 )
                           {
                             frame_Y = 0 - framePenWidth/2;
                             frame_height = old_Frame_Y2 + framePenWidth/2;
                           }

                           if( ( frame_X + frame_Width - framePenWidth/2 ) > screenWidth )
                           {
                             frame_Width = screenWidth + framePenWidth/2 - frame_X;
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
                           if( ( frame_X + frame_Width - framePenWidth/2 ) > screenWidth )
                           {
                             frame_Width = screenWidth + framePenWidth/2 - frame_X;
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
                           if( ( frame_X + frame_Width - framePenWidth/2 ) > screenWidth )
                           {
                             frame_Width = screenWidth + framePenWidth/2 - frame_X;
                           }

                           if( ( frame_Y + frame_height - framePenWidth/2 ) > screenHeight )
                           {
                             frame_height = screenHeight + framePenWidth/2 - frame_Y;
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
                           if( ( frame_Y + frame_height - framePenWidth/2 ) > screenHeight )
                           {
                             frame_height = screenHeight + framePenWidth/2 - frame_Y;
                           }

                           break;
                         }
      case BottomLeft  : { // Move
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
                           if ( frame_X <= 0 - framePenWidth/2 )
                           {
                              frame_X = 0 - framePenWidth/2;
                              frame_Width = old_Frame_X2 + framePenWidth/2;
                           }

                           if( ( frame_Y + frame_height - framePenWidth/2 ) > screenHeight )
                           {
                             frame_height = screenHeight + framePenWidth/2 - frame_Y;
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
                           if ( frame_X <= 0 - framePenWidth/2 )
                           {
                              frame_X = 0 - framePenWidth/2;
                              frame_Width = old_Frame_X2 + framePenWidth/2;
                           }

                           break;
                         }
      case Middle      : { // Move
                           int deltaX = ( old_Frame_X2 - framePenWidth/2 - frame_Width/2 ) - old_Mouse_X;
                           int deltaY = ( old_Frame_Y2 - framePenWidth/2 - frame_height/2 ) - old_Mouse_Y;
                           frame_X = event->x() - frame_Width/2 + framePenWidth/2 + deltaX;
                           frame_Y = event->y() - frame_height/2 + framePenWidth/2 + deltaY;

                           // Limit Top
                           if ( frame_Y <= 0 - framePenWidth/2 )
                           {
                             frame_Y = 0 - framePenWidth/2;
                           }

                           // Limit Left
                           if ( frame_X <= 0 - framePenWidth/2 )
                           {
                             frame_X = 0 - framePenWidth/2;
                           }

                           // Limit Right
                           if( ( frame_X + frame_Width - framePenWidth/2 ) > screenWidth )
                           {
                               frame_X = screenWidth - frame_Width + framePenWidth/2;
                           }

                           // Limit Bottom
                           if( ( frame_Y + frame_height - framePenWidth/2 ) > screenHeight )
                           {
                               frame_Y = screenHeight - frame_height + framePenWidth/2;
                           }

                           break;
                         }
    } // end switch

    repaint();
    update();

    if ( handlePressed != NoHandle )
        return;

    QRect regionTopLeft( frame_X - radius - 1, frame_Y - radius - 1, radius*2 + 2, radius*2 + 2 );
    if ( regionTopLeft.contains( event->pos() ) )
    {
        setCursor( Qt::ClosedHandCursor);
        handleUnderMouse = TopLeft;
        return;
    }

    QRect regionTopMiddle( frame_X + frame_Width/2 - radius - 1, frame_Y - radius - 1, radius*2 + 2, radius*2 + 2 );
    if ( regionTopMiddle.contains( event->pos() )  )
    {
        setCursor( Qt::ClosedHandCursor);
        handleUnderMouse = TopMiddle;
        return;
    }

    QRect regionTopRight( frame_X + frame_Width - radius - 1, frame_Y - radius - 1, radius*2 + 2, radius*2 + 2 );
    if ( regionTopRight.contains( event->pos() )  )
    {
       setCursor( Qt::ClosedHandCursor);
        handleUnderMouse = TopRight;
        return;
    }

    QRect regionRightMiddle( frame_X + frame_Width - radius - 1, frame_Y + frame_height/2 - radius - 1, radius*2 + 2, radius*2 + 2 );
    if ( regionRightMiddle.contains( event->pos() )  )
    {
        setCursor( Qt::ClosedHandCursor);
        handleUnderMouse = RightMiddle;
        return;
    }

    QRect regionMiddle( frame_X + frame_Width/2 - radius - penWidth/2, frame_Y + frame_height/2 - radius - penWidth/2, 2 * radius + penWidth, 2 * radius + penWidth);
    if ( regionMiddle.contains( event->pos() )  )
    {
        setCursor( Qt::ClosedHandCursor);
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
        setCursor( Qt::ClosedHandCursor);
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
        setCursor( Qt::ClosedHandCursor);
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
        setCursor( Qt::ClosedHandCursor);
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
        setCursor( Qt::ClosedHandCursor);
        handleUnderMouse = LeftMiddle;
        return;
    }

    unsetCursor();
    handleUnderMouse = NoHandle;
}

void AreaSelector::paintEvent( QPaintEvent *event ){
    Q_UNUSED(event);
    QPixmap pixmap( screenWidth, screenHeight);
    pixmap.fill(Qt::transparent);
    QPainter painterPixmap;
    painterPixmap.begin(&pixmap);
    painterPixmap.setRenderHints( QPainter::Antialiasing, true );

    if(!recordemode){
        //drawing buttons and frame
        HandleTopLeft( painterPixmap );
        HandleTopRight( painterPixmap );
        HandleBottomLeft( painterPixmap );
        HandleBottomRight( painterPixmap );
        HandleMiddle( painterPixmap ); //central button
    }else{
        HandleRecord( painterPixmap,
                      frame_X - radius + penWidth/2,
                      frame_Y - radius + penWidth/2,
                        0 * 16,
                      270 * 16 );
        HandleRecord( painterPixmap,
                      frame_X + frame_Width - radius + penWidth/2,
                      frame_Y - radius + penWidth/2,
                      -90 * 16,
                      270 * 16 );
        HandleRecord( painterPixmap,
                      frame_X + frame_Width - radius + penWidth/2,
                      frame_Y + frame_height - radius + penWidth/2,
                      -180 * 16,
                       270 * 16 );
        HandleRecord( painterPixmap,
                      frame_X - radius + penWidth/2,
                      frame_Y + frame_height - radius + penWidth/2,
                         0 * 16,
                      -270 * 16 );        
    }
    drawFrame( painterPixmap );
    painterPixmap.end();
    QPainter painter;
    painter.begin( this );
    painter.drawPixmap( QPoint( 0, 0 ), pixmap );
    painter.end();

    setMask( pixmap.mask() );
}

/*////////////////////////////
// slots
////////////////////////////*/

void AreaSelector::slot_recordMode( bool value )
{
   recordemode = value;
   repaint();
   update();
}

void AreaSelector::slot_areaReset()
{
    frame_Width = frame_min_width;
    frame_height = frame_min_height;
    frame_X = (screenWidth/2 - frame_Width/2)-framePenWidth/2;
    frame_Y = (screenHeight/2 - frame_height/2)-framePenWidth/2;
    repaint();
    update();
}

/*////////////////////////////
// utilities
////////////////////////////*/
void AreaSelector::setGeometry( int x, int y, int with, int height  )
{
  frame_X = x;
  frame_Y = y;
  frame_Width = with;
  frame_height = height;
  update();
}

/*////////////////////////////
// getter and setter
////////////////////////////*/

QColor AreaSelector::getFrameColor()
{
    return frameColor;
}
void AreaSelector::setFrameColor( QColor color )
{
    frameColor = color;
}

QColor AreaSelector::getColorSelectedArrow()
{
    return colorSelectedArrow;
}
void AreaSelector::setColorSelectedArrow(QColor color)
{
    colorSelectedArrow = color;
}

void AreaSelector::setWidth( int width )
{
    frame_Width = width + framePenWidth;
    repaint();
    update();
}
void AreaSelector::setHeight( int height )
{
    frame_height = height + framePenWidth;
    repaint();
    update();
}
