#include "drawsurface.h"
#include <QPainter>
#include <QDebug>

#define TFT_WIDTH 160
#define TFT_HEIGHT 128
#define SCALE 5

uint16_t convertColor(uint16_t r, uint16_t g, uint16_t b)
{
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

QColor toQColor(uint16_t color)
{
    return QColor((color >> 11) << 3, (color & 0x7E0) >> 3, (color & 0x1F) << 3);
}

DrawSurface::DrawSurface(QWidget *parent) : QWidget(parent),
    _data(),
    _dataSize(0)
{
uint32_t count(0);
uint8_t temp[100000];

#define PUSH_8(val) temp[count] = (uint8_t)val; ++count;
#define PUSH_16(val) *(uint16_t*)&(temp[count]) = (uint16_t)val; ++count; ++count;

PUSH_16(convertColor(0xFF, 0xFF, 0xFF)); // white bg
PUSH_8(3); // 3 colors

// color 1
PUSH_16(convertColor(0x00, 0xFF, 0x00)); // color 1 is yellow
PUSH_16(2); // 2 rects
PUSH_8(10); PUSH_8(10); PUSH_8(10); PUSH_8(10); // Rect 1
PUSH_8(20); PUSH_8(20); PUSH_8(20); PUSH_8(20); // Rect 2
PUSH_16(1); // 1 V Line
PUSH_8(10); PUSH_8(20); PUSH_8(20);
PUSH_16(1); // 1 H Line
PUSH_8(20); PUSH_8(10); PUSH_8(20);
PUSH_16(1); // 1 pix
PUSH_8(30); PUSH_8(30);

// color 2
PUSH_16(convertColor(0xFF, 0x00, 0x00)); // color 2 is red
PUSH_16(1); // 1 rects
PUSH_8(40); PUSH_8(40); PUSH_8(10); PUSH_8(10); // Rect 1
PUSH_16(0); // 0 V Line
PUSH_16(0); // 0 H Line
PUSH_16(0); // 0 pix

// color 3
PUSH_16(convertColor(0x00, 0x00, 0xFF)); // color 3 is blue
PUSH_16(0); // 0 rects
PUSH_16(0); // 1 V Line
PUSH_16(0); // 1 H Line
PUSH_16(1); // 1 pix
PUSH_8(15); PUSH_8(15);

#undef PUSH_16
#undef PUSH_8

_data = new uint8_t[count];
_dataSize = count;
memcpy(_data, temp, _dataSize);
delete[] temp;
}

DrawSurface::~DrawSurface()
{
    if(_data != nullptr)
        delete[] _data;
}

void DrawSurface::paintEvent(QPaintEvent*)
{
            //printf("0x%x\n", convertColor(255, 0, 255));
    QPainter p(this);

    //QPixmap pixmap(TFT_WIDTH*SCALE, TFT_HEIGHT*SCALE);

    if(_data != nullptr && _dataSize != 0)
    {
        qDebug() << "Data size" << _dataSize;
        uint8_t* pointer = _data;
        uint16_t current(0);
#define READ_8 current = *pointer; ++pointer;
#define READ_16 current = *((uint16_t*)pointer); ++pointer; ++pointer;

        READ_16;
        printf("bg %x %x %x %x\n", current, toQColor(current).red(), toQColor(current).green(), toQColor(current).blue());
        p.fillRect(rect(), toQColor(current));

        READ_8;
        uint32_t numColors(current);
        for(uint32_t currentColor = 0; currentColor < numColors; ++currentColor)
        {
            READ_16;
            QColor myColor(toQColor(current));
            QColor outlineColor = myColor.darker();
            if(myColor.toHsv().value() < 100)
            {
                outlineColor = myColor.lighter();
            }

            p.setPen(outlineColor);
            p.setBrush(QBrush(myColor));

            READ_16;
            uint32_t numRects(current);
            for(uint32_t currentRect = 0; currentRect < numRects; ++currentRect)
            {
                READ_8;
                uint32_t x(current);
                READ_8;
                uint32_t y(current);
                READ_8;
                uint32_t w(current);
                READ_8;
                uint32_t h(current);

                p.drawRect(x*SCALE, y*SCALE, w*SCALE, h*SCALE);
            }

            READ_16;
            uint32_t numVLines(current);
            for(uint32_t currentVLine = 0; currentVLine < numVLines; ++currentVLine)
            {
                READ_8;
                uint32_t x(current);
                READ_8;
                uint32_t y(current);
                READ_8;
                uint32_t l(current);

                p.drawRect(x*SCALE, y*SCALE, SCALE, l*SCALE);
            }

            READ_16;
            uint32_t numHLines(current);
            for(uint32_t currentHLine = 0; currentHLine < numHLines; ++currentHLine)
            {
                READ_8;
                uint32_t x(current);
                READ_8;
                uint32_t y(current);
                READ_8;
                uint32_t l(current);

                p.drawRect(x*SCALE, y*SCALE, l*SCALE, SCALE);
            }

            READ_16;
            uint32_t numPix(current);
            for(uint32_t currentPix = 0; currentPix < numPix; ++currentPix)
            {
                READ_8;
                uint32_t x(current);
                READ_8;
                uint32_t y(current);

                p.drawRect(x*SCALE, y*SCALE, SCALE, SCALE);
            }
        }

#undef READ_16
#undef READ_8
    }

    p.end();
}
