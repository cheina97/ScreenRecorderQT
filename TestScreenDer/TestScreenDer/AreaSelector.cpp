#include "AreaSelector.h"
#include <QDebug>
#include <QPainter>
#include <QGuiApplication>
#include <QBitmap>
#include <QPaintEvent>
#include <QIcon>
#include <QRect>
#include <QPen>

AreaSelector::AreaSelector(Ui::MainWindow *ui)
{
    this->ui = ui;
    this->screen = QGuiApplication::primaryScreen();
    qDebug()<<"creato";
}

AreaSelector::~AreaSelector(){}

void AreaSelector::paintEvent( QPaintEvent *event ){
    qDebug()<<"paint event";
    (void)event;
    QRect geom = screen->geometry();
    QPixmap pixmap( geom.height(), geom.width());

    QPainter painter;
    painter.begin( &pixmap );
    QPen pen;
    pen.setColor(Qt::green);
    pen.setWidth(5);
    painter.setPen(pen);
    painter.drawRect(QRect(80, 120, 200, 100));
}

void AreaSelector::slot_init()
{
    qDebug()<<"SIGNAL OK!";
}
