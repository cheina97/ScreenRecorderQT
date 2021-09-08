#ifndef AREASELECTOR_H
#define AREASELECTOR_H

#include <QWidget>
#include <QScreen>

#include "ui_mainwindow.h"

class AreaSelector: public QWidget
{
Q_OBJECT
public:
    AreaSelector();
    virtual ~AreaSelector();

//    int getX();
//    int getY();
//    qreal getHeight();
//    qreal getWidth();
//    qreal getXRecordArea();
//    qreal getYRecordArea();

//    void setX( int x );
//    void setY( int y );
//    void setWidth( int width );
//    void setHeight( int height );

    QColor getFrameColor();
    QColor getColorSelectedArrow();


  public slots:
//    void slot_areaReset();
    void slot_init(); //executed when it's time to initialise the window
    void slot_recordMode(bool value);

  private slots:


  protected:
    void paintEvent( QPaintEvent *event );
    void mouseMoveEvent( QMouseEvent *event );
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent * event );
//    void leaveEvent( QEvent *event );
//    void keyPressEvent( QKeyEvent * event );

  private:
//    Ui::MainWindow *ui;
    QScreen *screen;
    int screenWidth; //screen dimension
    int screenHeight;

    enum Handle { NoHandle, TopLeft, TopMiddle, TopRight, RightMiddle, BottomRight, BottomMiddle, BottomLeft, LeftMiddle, Middle };
    Handle handlePressed;
    Handle handleUnderMouse;
    Handle HandleSelected;

//    QColor HandleColorBackground;
//    QColor HandleColorBackgroundSize;
//    QColor HandleColorByMousePressed;
//    void setHandleColorBackground( QColor color );
//    void setHandleColorByMousePressed( QColor color );
//    void setHandleColorBackgroundSize( QColor color );

    int mous_delta_X_to_blueline;
    int mous_delta_Y_to_blueline;

    int old_Mouse_X;
    int old_Mouse_Y;
    int old_Frame_Width;
    int old_Frame_Height;
    int old_Frame_X2;
    int old_Frame_Y2;

    int framePenWidth;
    int radius;
    int penWidth;

    int frame_X;
    int frame_Y;
    int frame_Width;
    int frame_height;
    int frame_min_width;
    int frame_min_height;

//    int pixelWidth;
//    int pixelHeight;

    QColor frameColor;
    QColor colorSelectedArrow;

    void setFrameColor( QColor color ); //this frame is all around the selected area
    void setColorSelectedArrow (QColor color);

    void drawFrame( QPainter &painter );
//    void printSize(QPainter &painter);
    void HandleRecord( QPainter &painter, int x, int y, int startAngle, int spanAngle );
    void HandleTopLeft(QPainter &painter );
//    void HandleTopLeftSize(QPainter &painter);
//    void HandleTopMiddle(QPainter &painter);
//    void HandleTopMiddleSize(QPainter &painter);
    void HandleTopRight( QPainter &painter );
//    void HandleTopRightSize(QPainter &painter);
//    void HandleRightMiddle(QPainter &painter);
//    void HandleRightMiddleSize(QPainter &painter);
    void HandleBottomRight(QPainter &painter);
//    void HandleBottomRightSize(QPainter &painter);
//    void HandleBottomMiddle(QPainter &painter);
//    void HandleBottomMiddleSize( QPainter &painter );
    void HandleBottomLeft(QPainter &painter);
//    void HandleBottomLeftSize( QPainter &painter );
//    void HandleLeftMiddle(QPainter &painter);
//    void HandleLeftMiddleSize( QPainter &painter );
    void HandleMiddle(QPainter &painter);

//    void vk_setGeometry( int x, int y, int with, int height );

//    enum vk_platform { x11, wayland, windows };
//    vk_platform platform;

    bool recordemode = false;

  };
  #endif
