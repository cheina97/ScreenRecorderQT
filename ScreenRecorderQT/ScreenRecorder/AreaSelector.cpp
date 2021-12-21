#include "AreaSelector.h"
#include "AreaSelectorButtons.h"

#include <QBitmap>
#include <QDebug>
#include <QGuiApplication>
#include <QIcon>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>

AreaSelector::AreaSelector()
    : handlePressed(NoHandle), handleUnderMouse(NoHandle),
      framePenWidth(4), // framePenWidth must be an even number
      radius(20), penWidth(2),
      frame_width(320 + framePenWidth), // default dimension
      frame_height(200 + framePenWidth), frame_min_width(320 + framePenWidth),
      frame_min_height(200 + framePenWidth), frameColor(Qt::black) {
    setWindowTitle("ScreenCapture");
    QIcon icon(QString::fromUtf8(":/icons/mainIcon.jpg"));
    setWindowIcon(icon);

    // window settings
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                   Qt::ToolTip);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    hide(); // setVisibile(false);
}

AreaSelector::~AreaSelector() {}

/*////////////////////////////
// handling the things to be drawn
////////////////////////////*/

void AreaSelector::drawFrame(QPainter &painter) {
    QPen pen(getFrameColor(), framePenWidth);
    pen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(pen);
    QBrush brush(Qt::transparent, Qt::SolidPattern);
    painter.setBrush(brush);
    painter.drawRect(frame_X, frame_Y, frame_width, frame_height);
}

void AreaSelector::HandleMiddle(QPainter &painter) {
    AreaSelectorButtons buttonArrow;

    painter.drawPixmap(frame_X + frame_width / 2 - buttonArrow.getWithHalf(),
                       frame_Y + frame_height / 2 - buttonArrow.getWithHalf(),
                       buttonArrow.getButton());

    painter.drawPixmap(
        frame_X + frame_width / 2 - buttonArrow.getWithHalf(),
        frame_Y + frame_height / 2 - buttonArrow.getWithHalf(),
        buttonArrow.getArrow(buttonArrow.degreeArrow::topMiddle));

    painter.drawPixmap(
        frame_X + frame_width / 2 - buttonArrow.getWithHalf(),
        frame_Y + frame_height / 2 - buttonArrow.getWithHalf(),
        buttonArrow.getArrow(buttonArrow.degreeArrow::rightMiddle));

    painter.drawPixmap(
        frame_X + frame_width / 2 - buttonArrow.getWithHalf(),
        frame_Y + frame_height / 2 - buttonArrow.getWithHalf(),
        buttonArrow.getArrow(buttonArrow.degreeArrow::bottomMiddle));

    painter.drawPixmap(
        frame_X + frame_width / 2 - buttonArrow.getWithHalf(),
        frame_Y + frame_height / 2 - buttonArrow.getWithHalf(),
        buttonArrow.getArrow(buttonArrow.degreeArrow::leftMiddle));
}

void AreaSelector::HandleTopLeft(QPainter &painter) {
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap(frame_X - radius, frame_Y - radius,
                       buttonArrow.getPixmapHandle(buttonArrow.topLeft));
}

void AreaSelector::HandleTopMiddle(QPainter &painter) {
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap(frame_X + frame_width / 2 - buttonArrow.getWithHalf(),
                       frame_Y - buttonArrow.getWithHalf(),
                       buttonArrow.getPixmapHandle(buttonArrow.topMiddle));
}

void AreaSelector::HandleTopRight(QPainter &painter) {
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap(frame_X + frame_width - buttonArrow.getWithHalf(),
                       frame_Y - buttonArrow.getWithHalf(),
                       buttonArrow.getPixmapHandle(buttonArrow.topRight));
}

void AreaSelector::HandleRightMiddle(QPainter &painter) {
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap(frame_X + frame_width - buttonArrow.getWithHalf(),
                       frame_Y + frame_height / 2 - buttonArrow.getWithHalf(),
                       buttonArrow.getPixmapHandle(buttonArrow.rightMiddle));
}

void AreaSelector::HandleBottomLeft(QPainter &painter) {
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap(frame_X - buttonArrow.getWithHalf(),
                       frame_Y + frame_height - buttonArrow.getWithHalf(),
                       buttonArrow.getPixmapHandle(buttonArrow.bottomLeft));
}

void AreaSelector::HandleBottomMiddle(QPainter &painter) {
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap(frame_X + frame_width / 2 - buttonArrow.getWithHalf(),
                       frame_Y + frame_height - buttonArrow.getWithHalf(),
                       buttonArrow.getPixmapHandle(buttonArrow.bottomMiddle));
}

void AreaSelector::HandleBottomRight(QPainter &painter) {
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap(frame_X + frame_width - buttonArrow.getWithHalf(),
                       frame_Y + frame_height - buttonArrow.getWithHalf(),
                       buttonArrow.getPixmapHandle(buttonArrow.bottomRight));
}

void AreaSelector::HandleLeftMiddle(QPainter &painter) {
    AreaSelectorButtons buttonArrow;
    painter.drawPixmap(frame_X - buttonArrow.getWithHalf(),
                       frame_Y + frame_height / 2 - buttonArrow.getWithHalf(),
                       buttonArrow.getPixmapHandle(buttonArrow.leftMiddle));
}

void AreaSelector::HandleRecord(QPainter &painter, int x, int y, int startAngle,
                                int spanAngle) {
    // this draws on top of the previous buttons
    QBrush brush;
    brush.setColor(Qt::darkGray);
    brush.setStyle(Qt::SolidPattern);
    painter.setBrush(brush);
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(penWidth);
    painter.setPen(pen);
    QRectF rectangle = QRectF(x, y, radius * 2, radius * 2);

    painter.drawPie(rectangle, startAngle, spanAngle);
}

void AreaSelector::printSize(QPainter &painter) {
    QString widthHeigtSize =
        QString::number(
            static_cast<int>(getWidth() / screen->devicePixelRatio())) +
        " x " +
        QString::number(
            static_cast<int>(getHeight() / screen->devicePixelRatio()));

    QFont font;
    font.setPointSize(14);
    painter.setFont(font);

    QFontMetrics fontMetrics(font);
    pixelWidth = fontMetrics.horizontalAdvance(widthHeigtSize);
    pixelHeight = fontMetrics.height();
    QRect rect(frame_X + frame_width / 2 - pixelWidth / 2 - 5,
               frame_Y + frame_height / 2 - pixelHeight / 2 - 50,
               pixelWidth + 10, pixelHeight);

    painter.setBrush(QBrush(Qt::yellow, Qt::SolidPattern));
    painter.setPen(QPen(Qt::black, 2));

    painter.drawRoundedRect(rect, 7, 7);

    painter.drawText(rect, Qt::AlignCenter, widthHeigtSize);
}

/*////////////////////////////
// reacting to events
////////////////////////////*/

void AreaSelector::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }

    switch (handleUnderMouse) {
    case NoHandle:
        handlePressed = NoHandle;
        break;
    case TopLeft: {
        handlePressed = TopLeft;
        break;
    }
    case TopMiddle: {
        handlePressed = TopMiddle;
        break;
    }
    case TopRight: {
        handlePressed = TopRight;
        break;
    }
    case RightMiddle: {
        handlePressed = RightMiddle;
        break;
    }
    case BottomRight: {
        handlePressed = BottomRight;
        break;
    }
    case BottomMiddle: {
        handlePressed = BottomMiddle;
        break;
    }
    case BottomLeft: {
        handlePressed = BottomLeft;
        break;
    }
    case LeftMiddle: {
        handlePressed = LeftMiddle;
        break;
    }
    case Middle: {
        handlePressed = Middle;
        break;
    }
    }

    mouse_delta_X = event->x() - frame_X;
    mouse_delta_Y = event->y() - frame_Y;

    old_mouse_X = event->x();
    old_mouse_Y = event->y();
    old_frame_width = frame_width;
    old_frame_height = frame_height;

    old_frame_X2 = frame_X + frame_width;
    old_frame_Y2 = frame_Y + frame_height;

    repaint();
    update();
}

void AreaSelector::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        return;
    }

    handlePressed = NoHandle;

    update();
}

void AreaSelector::mouseMoveEvent(QMouseEvent *event) {
    if (recordemode == true) {
        unsetCursor();
        return;
    }
    switch (handlePressed) {
    case NoHandle:
        break;
    case TopLeft: { // Move
        frame_X = event->x() - mouse_delta_X;
        frame_Y = event->y() - mouse_delta_Y;
        frame_width = old_mouse_X - event->x() + old_frame_width;
        frame_height = old_mouse_Y - event->y() + old_frame_height;

        // Limit min
        if (frame_width < frame_min_width) {
            frame_X = old_frame_X2 - frame_min_width;
            frame_width = frame_min_width;
        }

        if (frame_height < frame_min_height) {
            frame_Y = old_frame_Y2 - frame_min_height;
            frame_height = frame_min_height;
        }

        // Limit max
        if (frame_Y <= 0 - framePenWidth / 2) {
            frame_Y = 0 - framePenWidth / 2;
            frame_height = old_frame_Y2 + framePenWidth / 2;
        }

        if (frame_X <= 0 - framePenWidth / 2) {
            frame_X = 0 - framePenWidth / 2;
            frame_width = old_frame_X2 + framePenWidth / 2;
        }

        break;
    }
    case TopMiddle: { // Move
        frame_Y = event->y() - mouse_delta_Y;
        frame_height = old_mouse_Y - event->y() + old_frame_height;

        // Limit min
        if (frame_height < frame_min_height) {
            frame_Y = old_frame_Y2 - frame_min_height;
            frame_height = frame_min_height;
        }

        // Limit max
        if (frame_Y <= 0 - framePenWidth / 2) {
            frame_Y = 0 - framePenWidth / 2;
            frame_height = old_frame_Y2 + framePenWidth / 2;
        }

        break;
    }
    case TopRight: { // Move
        frame_Y = event->y() - mouse_delta_Y;
        frame_width = event->x() - old_mouse_X + old_frame_width;
        frame_height = old_mouse_Y - event->y() + old_frame_height;
        ;

        // Limit min
        if (frame_width < frame_min_width) {
            frame_width = frame_min_width;
        }

        if (frame_height < frame_min_height) {
            frame_Y = old_frame_Y2 - frame_min_height;
            frame_height = frame_min_height;
        }

        // Limit max
        if (frame_Y <= 0 - framePenWidth / 2) {
            frame_Y = 0 - framePenWidth / 2;
            frame_height = old_frame_Y2 + framePenWidth / 2;
        }

        if ((frame_X + frame_width - framePenWidth / 2) > screenWidth) {
            frame_width = screenWidth + framePenWidth / 2 - frame_X;
        }

        break;
    }
    case RightMiddle: { // Move
        frame_width = event->x() - old_mouse_X + old_frame_width;

        // Limit min
        if (frame_width < frame_min_width) {
            frame_width = frame_min_width;
        }

        // Limit max
        if ((frame_X + frame_width - framePenWidth / 2) > screenWidth) {
            frame_width = screenWidth + framePenWidth / 2 - frame_X;
        }

        break;
    }
    case BottomRight: { // Move
        frame_width = event->x() - old_mouse_X + old_frame_width;
        frame_height = event->y() - old_mouse_Y + old_frame_height;

        // Limit min
        if (frame_width < frame_min_width) {
            frame_width = frame_min_width;
        }

        if (frame_height < frame_min_height) {
            frame_height = frame_min_height;
        }

        // Limit max
        if ((frame_X + frame_width - framePenWidth / 2) > screenWidth) {
            frame_width = screenWidth + framePenWidth / 2 - frame_X;
        }

        if ((frame_Y + frame_height - framePenWidth / 2) > screenHeight) {
            frame_height = screenHeight + framePenWidth / 2 - frame_Y;
        }

        break;
    }
    case BottomMiddle: { // Move
        frame_height = event->y() - old_mouse_Y + old_frame_height;

        // Limit min
        if (frame_height < frame_min_height) {
            frame_height = frame_min_height;
        }

        // Limit max
        if ((frame_Y + frame_height - framePenWidth / 2) > screenHeight) {
            frame_height = screenHeight + framePenWidth / 2 - frame_Y;
        }

        break;
    }
    case BottomLeft: { // Move
        frame_X = event->x() - mouse_delta_X;
        frame_height = event->y() - old_mouse_Y + old_frame_height;
        frame_width = old_mouse_X - event->x() + old_frame_width;

        // Limit min
        if (frame_width < frame_min_width) {
            frame_X = old_frame_X2 - frame_min_width;
            frame_width = frame_min_width;
        }

        if (frame_height < frame_min_height) {
            frame_height = frame_min_height;
        }

        // Limit max
        if (frame_X <= 0 - framePenWidth / 2) {
            frame_X = 0 - framePenWidth / 2;
            frame_width = old_frame_X2 + framePenWidth / 2;
        }

        if ((frame_Y + frame_height - framePenWidth / 2) > screenHeight) {
            frame_height = screenHeight + framePenWidth / 2 - frame_Y;
        }

        break;
    }
    case LeftMiddle: { // Move
        frame_X = event->x() - mouse_delta_X;
        frame_width = old_mouse_X - event->x() + old_frame_width;

        // Limit min
        if (frame_width < frame_min_width) {
            frame_X = old_frame_X2 - frame_min_width;
            frame_width = frame_min_width;
        }

        // Limit max
        if (frame_X <= 0 - framePenWidth / 2) {
            frame_X = 0 - framePenWidth / 2;
            frame_width = old_frame_X2 + framePenWidth / 2;
        }

        break;
    }
    case Middle: { // Move
        int deltaX =
            (old_frame_X2 - framePenWidth / 2 - frame_width / 2) - old_mouse_X;
        int deltaY =
            (old_frame_Y2 - framePenWidth / 2 - frame_height / 2) - old_mouse_Y;
        frame_X = event->x() - frame_width / 2 + framePenWidth / 2 + deltaX;
        frame_Y = event->y() - frame_height / 2 + framePenWidth / 2 + deltaY;

        // Limit Top
        if (frame_Y <= 0 - framePenWidth / 2) {
            frame_Y = 0 - framePenWidth / 2;
        }

        // Limit Left
        if (frame_X <= 0 - framePenWidth / 2) {
            frame_X = 0 - framePenWidth / 2;
        }

        // Limit Right
        if ((frame_X + frame_width - framePenWidth / 2) > screenWidth) {
            frame_X = screenWidth - frame_width + framePenWidth / 2;
        }

        // Limit Bottom
        if ((frame_Y + frame_height - framePenWidth / 2) > screenHeight) {
            frame_Y = screenHeight - frame_height + framePenWidth / 2;
        }

        break;
    }
    } // end switch

    repaint();
    update();

    if (handlePressed != NoHandle)
        return;

    QRect regionTopLeft(frame_X - radius - 1, frame_Y - radius - 1,
                        radius * 2 + 2, radius * 2 + 2);
    if (regionTopLeft.contains(event->pos())) {
        setCursor(Qt::ClosedHandCursor);
        handleUnderMouse = TopLeft;
        return;
    }

    QRect regionTopMiddle(frame_X + frame_width / 2 - radius - 1,
                          frame_Y - radius - 1, radius * 2 + 2, radius * 2 + 2);
    if (regionTopMiddle.contains(event->pos())) {
        setCursor(Qt::ClosedHandCursor);
        handleUnderMouse = TopMiddle;
        return;
    }

    QRect regionTopRight(frame_X + frame_width - radius - 1,
                         frame_Y - radius - 1, radius * 2 + 2, radius * 2 + 2);
    if (regionTopRight.contains(event->pos())) {
        setCursor(Qt::ClosedHandCursor);
        handleUnderMouse = TopRight;
        return;
    }

    QRect regionRightMiddle(frame_X + frame_width - radius - 1,
                            frame_Y + frame_height / 2 - radius - 1,
                            radius * 2 + 2, radius * 2 + 2);
    if (regionRightMiddle.contains(event->pos())) {
        setCursor(Qt::ClosedHandCursor);
        handleUnderMouse = RightMiddle;
        return;
    }

    QRect regionMiddle(frame_X + frame_width / 2 - radius - penWidth / 2,
                       frame_Y + frame_height / 2 - radius - penWidth / 2,
                       2 * radius + penWidth, 2 * radius + penWidth);
    if (regionMiddle.contains(event->pos())) {
        setCursor(Qt::ClosedHandCursor);
        handleUnderMouse = Middle;
        return;
    }

    AreaSelectorButtons buttonArrow;
    QRect regionBottomRight(frame_X + frame_width - buttonArrow.getWithHalf(),
                            frame_Y + frame_height - buttonArrow.getWithHalf(),
                            buttonArrow.getWithHalf() * 2,
                            buttonArrow.getWithHalf() * 2);
    if (regionBottomRight.contains(event->pos())) {
        setCursor(Qt::ClosedHandCursor);
        handleUnderMouse = BottomRight;
        return;
    }

    QRect regionBottomMiddle(
        frame_X + frame_width / 2 - buttonArrow.getWithHalf(),
        frame_Y + frame_height - buttonArrow.getWithHalf(),
        buttonArrow.getWithHalf() * 2, buttonArrow.getWithHalf() * 2);
    if (regionBottomMiddle.contains(event->pos())) {
        setCursor(Qt::ClosedHandCursor);
        handleUnderMouse = BottomMiddle;
        return;
    }

    QRect regionBottomLeft(frame_X - buttonArrow.getWithHalf(),
                           frame_Y + frame_height - buttonArrow.getWithHalf(),
                           buttonArrow.getWithHalf() * 2,
                           buttonArrow.getWithHalf() * 2);
    if (regionBottomLeft.contains(event->pos())) {
        setCursor(Qt::ClosedHandCursor);
        handleUnderMouse = BottomLeft;
        return;
    }

    QRect regionLeftMiddle(
        frame_X - buttonArrow.getWithHalf(),
        frame_Y + frame_height / 2 - buttonArrow.getWithHalf(),
        buttonArrow.getWithHalf() * 2, buttonArrow.getWithHalf() * 2);
    if (regionLeftMiddle.contains(event->pos())) {
        setCursor(Qt::ClosedHandCursor);
        handleUnderMouse = LeftMiddle;
        return;
    }

    unsetCursor();
    handleUnderMouse = NoHandle;
}

void AreaSelector::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPixmap pixmap(screenWidth, screenHeight);
    pixmap.fill(Qt::transparent);
    QPainter painterPixmap;
    painterPixmap.begin(&pixmap);
    painterPixmap.setRenderHints(QPainter::Antialiasing, true);

    if (!recordemode) {
        // drawing buttons and frame
        HandleTopLeft(painterPixmap);
        HandleTopMiddle(painterPixmap);
        HandleTopRight(painterPixmap);
        HandleRightMiddle(painterPixmap);
        HandleBottomLeft(painterPixmap);
        HandleBottomMiddle(painterPixmap);
        HandleBottomRight(painterPixmap);
        HandleLeftMiddle(painterPixmap);
        HandleMiddle(painterPixmap); // central button
        printSize(painterPixmap);
    } else {
        HandleRecord(painterPixmap, frame_X - radius + penWidth / 2,
                     frame_Y - radius + penWidth / 2, 0 * 16, 270 * 16);
        HandleRecord(painterPixmap,
                     frame_X + frame_width / 2 - radius + penWidth / 2,
                     frame_Y - radius + penWidth / 2, 0 * 16, 180 * 16);
        HandleRecord(painterPixmap,
                     frame_X + frame_width - radius + penWidth / 2,
                     frame_Y - radius + penWidth / 2, -90 * 16, 270 * 16);
        HandleRecord(painterPixmap,
                     frame_X + frame_width - radius + penWidth / 2,
                     frame_Y + frame_height / 2 - radius + penWidth / 2,
                     -90 * 16, 180 * 16);
        HandleRecord(painterPixmap,
                     frame_X + frame_width - radius + penWidth / 2,
                     frame_Y + frame_height - radius + penWidth / 2, -180 * 16,
                     270 * 16);
        HandleRecord(
            painterPixmap, frame_X + frame_width / 2 - radius + penWidth / 2,
            frame_Y + frame_height - radius + penWidth / 2, 0 * 16, -180 * 16);
        HandleRecord(painterPixmap, frame_X - radius + penWidth / 2,
                     frame_Y + frame_height - radius + penWidth / 2, 0 * 16,
                     -270 * 16);
        HandleRecord(painterPixmap, frame_X - radius + penWidth / 2,
                     frame_Y + frame_height / 2 - radius + penWidth / 2,
                     -90 * 16, -180 * 16);
    }
    drawFrame(painterPixmap);
    painterPixmap.end();
    QPainter painter;
    painter.begin(this);
    painter.drawPixmap(QPoint(0, 0), pixmap);
    painter.end();

    setMask(pixmap.mask());
}

/*////////////////////////////
// slots
////////////////////////////*/
void AreaSelector::slot_init() {
    qDebug() << "Eseguo slot init";
    screen = QGuiApplication::primaryScreen();
    resize(screen->size().width(), screen->size().height());
    screenWidth = screen->size().width();
    screenHeight = screen->size().height();
    move(screen->geometry().x(), screen->geometry().y());
    qDebug() << "screen: " << screenWidth << "x" << screenHeight;
    // positioning the rectange in the middle of the screen:
    frame_width = frame_min_width;
    frame_height = frame_min_height;
    frame_X = (screenWidth / 2 - frame_width / 2) - framePenWidth / 2;
    frame_Y = (screenHeight / 2 - frame_height / 2) - framePenWidth / 2;
    qDebug() << "frame coordinates: " << frame_X << "," << frame_Y;
}

void AreaSelector::slot_recordMode(bool value) {
    recordemode = value;
    repaint();
    update();
}

void AreaSelector::slot_areaReset() {
    frame_width = frame_min_width;
    frame_height = frame_min_height;
    frame_X = (screenWidth / 2 - frame_width / 2) - framePenWidth / 2;
    frame_Y = (screenHeight / 2 - frame_height / 2) - framePenWidth / 2;
    repaint();
    update();
}

/*////////////////////////////
// getter and setter
////////////////////////////*/

QColor AreaSelector::getFrameColor() { return frameColor; }
void AreaSelector::setFrameColor(QColor color) { frameColor = color; }

int AreaSelector::getX() { return frame_X; }
void AreaSelector::setX(int x) {
    frame_X = x - framePenWidth / 2;
    repaint();
    update();
}
int AreaSelector::getY() { return frame_Y; }
void AreaSelector::setY(int y) {
    frame_Y = y - framePenWidth / 2;
    repaint();
    update();
}

qreal AreaSelector::getHeight() {
    qreal xReal =
        ((frame_Y + framePenWidth / 2) + (frame_height - framePenWidth)) *
        screen->devicePixelRatio();
    int xInt = static_cast<int>(
        ((frame_Y + framePenWidth / 2) + (frame_height - framePenWidth)) *
        screen->devicePixelRatio());

    if (xReal > xInt) {
        xReal = static_cast<int>((frame_height - framePenWidth) *
                                 screen->devicePixelRatio()) -
                1;
    } else {
        xReal = static_cast<int>((frame_height - framePenWidth) *
                                 screen->devicePixelRatio());
    }
    return xReal;
}
void AreaSelector::setHeight(int height) {
    frame_height = height + framePenWidth;
    repaint();
    update();
}
qreal AreaSelector::getWidth() {
    qreal xReal =
        ((frame_X + framePenWidth / 2) + (frame_width - framePenWidth)) *
        screen->devicePixelRatio();
    int xInt = static_cast<int>(
        ((frame_X + framePenWidth / 2) + (frame_width - framePenWidth)) *
        screen->devicePixelRatio());

    if (xReal > xInt) {
        xReal = static_cast<int>((frame_width - framePenWidth) *
                                 screen->devicePixelRatio()) -
                1;
    } else {
        xReal = static_cast<int>((frame_width - framePenWidth) *
                                 screen->devicePixelRatio());
    }
    return xReal;
}
void AreaSelector::setWidth(int width) {
    frame_width = width + framePenWidth;
    repaint();
    update();
}
qreal AreaSelector::getXRecordArea() {
    qreal xReal = (frame_X + framePenWidth / 2) * screen->devicePixelRatio();
    int xInt = static_cast<int>((frame_X + framePenWidth / 2) *
                                screen->devicePixelRatio());

    if (xReal > xInt) {
        xReal = xInt + 1;
    }

    return xReal;
}
qreal AreaSelector::getYRecordArea() {
    qreal xReal = (frame_Y + framePenWidth / 2) * screen->devicePixelRatio();
    int xInt = static_cast<int>((frame_Y + framePenWidth / 2) *
                                screen->devicePixelRatio());

    if (xReal > xInt) {
        xReal = xInt + 1;
    }

    return xReal;
}

/*////////////////////////////
// utilities
////////////////////////////*/
void AreaSelector::setGeometry(int x, int y, int width, int height) {
    setX(x);
    setY(y);
    setWidth(width);
    setHeight(height);
    repaint();
    update();
}
