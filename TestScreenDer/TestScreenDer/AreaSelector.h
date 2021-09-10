#ifndef AREASELECTOR_H
#define AREASELECTOR_H

#include <QWidget>
#include <QScreen>

//#include "ui_mainwindow.h"

class AreaSelector: public QWidget
{
Q_OBJECT
public:
    AreaSelector();
    virtual ~AreaSelector();

    //getter/setter for frame coordinates
    int getX();
    int getY();
    void setX( int x );
    void setY( int y );

    //getter for recorded area properties
    qreal getHeight();
    qreal getWidth();
    qreal getXRecordArea();
    qreal getYRecordArea();
    void setWidth( int width );
    void setHeight( int height );

    QColor getFrameColor();


  public slots:
    void slot_areaReset();
    void slot_init(); //executed when it's time to initialise the window
    void slot_recordMode(bool value);

  private slots:


  protected:
    void paintEvent( QPaintEvent *event );
    void mouseMoveEvent( QMouseEvent *event );
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent * event );

  private:
//    Ui::MainWindow *ui;
    QScreen *screen;
    int screenWidth; //current screen dimension
    int screenHeight;

    enum Handle { NoHandle, TopLeft, TopMiddle, TopRight, RightMiddle, BottomRight, BottomMiddle, BottomLeft, LeftMiddle, Middle };
    Handle handlePressed;
    Handle handleUnderMouse;

    //parameters needed to resize properly
    int mouse_delta_X; //click coordinates with respect to the frame
    int mouse_delta_Y;
    int old_mouse_X;
    int old_mouse_Y;
    int old_frame_width;
    int old_frame_height;
    int old_frame_X2; //bottom right corner
    int old_frame_Y2;

    //buttons parameters
    int framePenWidth;
    int radius;
    int penWidth;

    //frame properties
    int frame_X;
    int frame_Y;
    int frame_width;
    int frame_height;
    int frame_min_width;
    int frame_min_height;

    //needed to print frame size
    int pixelWidth;
    int pixelHeight;

    QColor frameColor;

    void setFrameColor( QColor color );

    void drawFrame( QPainter &painter );
    void printSize(QPainter &painter);
    void HandleRecord( QPainter &painter, int x, int y, int startAngle, int spanAngle );
    void HandleTopLeft(QPainter &painter );
    void HandleTopMiddle(QPainter &painter);
    void HandleTopRight( QPainter &painter );
    void HandleRightMiddle(QPainter &painter);
    void HandleBottomRight(QPainter &painter);
    void HandleBottomMiddle(QPainter &painter);
    void HandleBottomLeft(QPainter &painter);
    void HandleLeftMiddle(QPainter &painter);
    void HandleMiddle(QPainter &painter);

    void setGeometry( int x, int y, int with, int height );

//    enum vk_platform { x11, wayland, windows };
//    vk_platform platform;

    bool recordemode = false;

  };
  #endif
