#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QFileDialog.h"
#include "drawsurface.h"
#include <QDebug>



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _currentPix()
{
    ui->setupUi(this);

    connect(ui->actionOpen_BMP, SIGNAL(triggered()), this, SLOT(openBmp()));
    connect(ui->actionOpen_PIX, SIGNAL(triggered()), this, SLOT(openPix()));
    connect(ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(save()));

}

MainWindow::~MainWindow()
{
    delete ui;
}

typedef struct
{
    uint16_t color;
    uint32_t count;
} ColorCount;

bool ccCompare (const ColorCount& first, const ColorCount& second)
{
    return (first.count > second.count);
}

void pushColor(std::list<ColorCount>& cc, uint16_t col)
{
    for(auto& ccItem : cc)
    {
        if(ccItem.color == col)
        {
            ++ccItem.count;
            return;
        }
    }

    ColorCount plop;
    plop.color = col;
    plop.count = 1;
    cc.push_back(plop);
}

typedef struct
{
    uint8_t x, y, w, h;
} RectEncoding;

typedef struct
{
    uint8_t x, y, h;
} VLineEncoding;

typedef struct
{
    uint8_t x, y, w;
} HLineEncoding;

typedef struct
{
    uint8_t x, y;
} PixEncoding;

typedef struct
{
    std::list<RectEncoding> rects;
    std::list<VLineEncoding> vLines;
    std::list<HLineEncoding> hLines;
    std::list<PixEncoding> pixes;
} ColorEncoding;

typedef struct
{
    uint16_t background;
    QMap<uint16_t, ColorEncoding> colors;
} ImageEncoding;

void findRectangle(const std::vector<uint16_t> pixColors,
              const std::vector<bool>& encodedColors,
              const uint32_t& w, const uint32_t& h,
              const uint32_t& cornerX, const uint32_t& cornerY,
              uint32_t& resW, uint32_t& resH
              )
{
    resW = 1;
    resH = 1;

#define IS_ENCODED(x,y) encodedColors[(cornerY+y)*w + (cornerX+x)]
#define COLOR(x,y) pixColors[(cornerY+y)*w + (cornerX+x)]
    uint16_t color = COLOR(0, 0);

    uint32_t currentXOffset(0);
    uint32_t currentYOffset(0);
    uint32_t maxXOffset(w-cornerX);
    uint32_t maxSurface(0);

    bool done(false);

    while(!done)
    {
        // look as far as possible on the line
        currentXOffset = 0;
        bool lineDone(false);

        while(!lineDone && currentXOffset < maxXOffset)
        {
            if(IS_ENCODED(currentXOffset, currentYOffset) || COLOR(currentXOffset, currentYOffset) != color)
            {
                // stop
                lineDone = true;
            }
            else
            {
                ++currentXOffset;
            }
        }
        // This is as far as we can go on this line
        maxXOffset = currentXOffset;

        uint32_t currentSurface((currentXOffset + 1)*(currentYOffset + 1));
        if(currentSurface > maxSurface)
        {
            maxSurface = currentSurface;
            resW = (currentXOffset /*+ 1*/);
            resH = (currentYOffset + 1);
        }

        ++currentYOffset;
        if(currentYOffset >= h)
        {
            done = true;
        }
        else if(IS_ENCODED(0, currentYOffset) || COLOR(0, currentYOffset) != color)
        {
            done = true;
        }

    }

#undef COLOR
#undef IS_ENCODED
//resW=1;
//resH=1;
}

void MainWindow::openBmp()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select file", QString(), QString());

    if(fileName.isEmpty())
        return;

    QImage image;
    if(!image.load(fileName))
        return;

//    QPixmap pixmap;
//    pixmap.convertFromImage(image);

    qDebug() << "Success rading file";

    // Convert to pix
    {
        qDebug() << "Converting to PIX";

        qDebug() << "Checking size < 160*128";

        if(image.width() > 160 || image.height() > 128)
        {
            qDebug() << "fail";
            return;
        }

        qDebug() << "Converting colors";



        uint32_t w = image.width();
        uint32_t h = image.height();
        std::vector<uint16_t> pixColors(w*h, 0);

        std::list<ColorCount> colorCount;

        for(uint32_t x = 0; x <  image.width(); ++x)
        {
            for(uint32_t y = 0; y <  image.height(); ++y)
            {
                QColor currentColor(image.pixelColor(x, y));
                uint16_t convertedColor = convertColor(currentColor.red(), currentColor.green(), currentColor.blue());
                pixColors[y*w+x] = convertedColor;
                pushColor(colorCount, convertedColor);
            }
        }

        qDebug() << "DONE Converting to PIX";

        qDebug() << "Reducing number of colors to max 8";

        // Sort colors
        colorCount.sort(ccCompare);

        for(const auto& cc: colorCount)
        {
            printf("Color %x count %d\n", cc.color, cc.count);
        }
        // TODO

        qDebug() << "DONE Reducing number of colors to max 8";

        ImageEncoding encoding;
        {
            std::vector<bool> encodedColors(w*h, false);
            encoding.background = colorCount.front().color;

            // Mark all pixels with bg color as encoded
            for(unsigned int ii = 0; ii < encodedColors.size(); ++ii)
            {
                if(pixColors[ii] == encoding.background)
                {
                    encodedColors[ii] = true;
                }
            }

            // Now, for all non-encoded pixel, look for the largest non-encoded rect surface
            // with this pixel as top-left corner
            for(unsigned int cornerY = 0; cornerY < h; ++cornerY)
            {
                for(unsigned int cornerX = 0; cornerX < w; ++cornerX)
                {
                    if(!encodedColors[cornerY*w+cornerX])
                    {
                        uint32_t resW(0), resH(0);
#if 1
                        findRectangle(pixColors,
                                      encodedColors,
                                      w, h,
                                      cornerX, cornerY,
                                      resW, resH
                                      );
#else
                        resH = 1;
                        resW = 1;
#endif

                        if(resW != 1)
                        {
                            if(resH != 1)
                            {
                                //rect
                                RectEncoding rect;
                                rect.x = cornerX;
                                rect.y = cornerY;
                                rect.w = resW;
                                rect.h = resH;

                                encoding.colors[pixColors[cornerY*w+cornerX]].rects.push_back(rect);
                            }
                            else // resW != 1 && resH == 1
                            {
                                // VLine
                                VLineEncoding line;
                                line.x = cornerX;
                                line.y = cornerY;
                                line.h = resW;

                                encoding.colors[pixColors[cornerY*w+cornerX]].vLines.push_back(line);
                            }
                        }
                        else // resW == 1
                        {
                            if(resH != 1)
                            {
                                //HLine
                                HLineEncoding line;
                                line.x = cornerX;
                                line.y = cornerY;
                                line.w = resH;

                                encoding.colors[pixColors[cornerY*w+cornerX]].hLines.push_back(line);
                            }
                            else // resW == 1 && resH == 1
                            {
                                //Pix
                                PixEncoding pix;
                                pix.x = cornerX;
                                pix.y = cornerY;

                                encoding.colors[pixColors[cornerY*w+cornerX]].pixes.push_back(pix);
                            }
                        }

                        for(unsigned int x = 0; x < resW; ++x)
                        {
                            for(unsigned int y = 0; y < resH; ++y)
                            {
                                encodedColors[(cornerY+y)*w+(cornerX+x)] = true;
                            }
                        }

                    } // end if(!encodedColors[cornerY*h+cornerX])
                } // end for(unsigned int cornerY = 0; cornerY < h; ++cornerY)
            }// end for(unsigned int cornerX = 0; cornerX < w; ++cornerX)
        }

        {
            // Now draw
            uint32_t count(0);
            uint8_t temp[100000];

#define PUSH_8(val) temp[count] = (uint8_t)val; ++count;
#define PUSH_16(val) *(uint16_t*)&(temp[count]) = (uint16_t)val; ++count; ++count;

            PUSH_16(encoding.background); // push background
            PUSH_8(encoding.colors.size());

            for(QMap<uint16_t, ColorEncoding>::const_iterator color = encoding.colors.cbegin();
                color != encoding.colors.cend();
                ++color)
            {
                PUSH_16(color.key());

                PUSH_16(color.value().rects.size());
                for(const auto& rect: color.value().rects)
                {
                    PUSH_8(rect.x);
                    PUSH_8(rect.y);
                    PUSH_8(rect.w);
                    PUSH_8(rect.h);
                }
                PUSH_16(color.value().hLines.size());
                for(const auto& hLine: color.value().hLines)
                {
                    PUSH_8(hLine.x);
                    PUSH_8(hLine.y);
                    PUSH_8(hLine.w);
                }

                PUSH_16(color.value().vLines.size());
                for(const auto& vLine: color.value().vLines)
                {
                    PUSH_8(vLine.x);
                    PUSH_8(vLine.y);
                    PUSH_8(vLine.h);
                }

                PUSH_16(color.value().pixes.size());
                for(const auto& pix: color.value().pixes)
                {
                    PUSH_8(pix.x);
                    PUSH_8(pix.y);
                }
            }

//                        PUSH_8(3); // 3 colors
//            // color 1
//            PUSH_16(convertColor(0x00, 0xFF, 0xFF)); // color 1 is yellow
//            PUSH_16(2); // 2 rects
//            PUSH_8(10); PUSH_8(10); PUSH_8(10); PUSH_8(10); // Rect 1
//            PUSH_8(20); PUSH_8(20); PUSH_8(20); PUSH_8(20); // Rect 2
//            PUSH_16(1); // 1 V Line
//            PUSH_8(10); PUSH_8(20); PUSH_8(20);
//            PUSH_16(1); // 1 H Line
//            PUSH_8(20); PUSH_8(10); PUSH_8(20);
//            PUSH_16(1); // 1 pix
//            PUSH_8(30); PUSH_8(30);

//            // color 2
//            PUSH_16(convertColor(0xFF, 0x00, 0x00)); // color 2 is red
//            PUSH_16(1); // 1 rects
//            PUSH_8(40); PUSH_8(40); PUSH_8(10); PUSH_8(10); // Rect 1
//            PUSH_16(0); // 0 V Line
//            PUSH_16(0); // 0 H Line
//            PUSH_16(0); // 0 pix

//            // color 3
//            PUSH_16(convertColor(0x00, 0x00, 0xFF)); // color 3 is blue
//            PUSH_16(0); // 0 rects
//            PUSH_16(0); // 1 V Line
//            PUSH_16(0); // 1 H Line
//            PUSH_16(1); // 1 pix
//            PUSH_8(15); PUSH_8(15);

#undef PUSH_16
#undef PUSH_8

            if(ui->centralWidget->_data)
            {
                delete[] ui->centralWidget->_data;
            }
            ui->centralWidget->_data = new uint8_t[count];
            memcpy(ui->centralWidget->_data, temp, count);
            ui->centralWidget->_dataSize = count;
        }






    }
ui->centralWidget->update();
}

void MainWindow::openPix()
{
    ui->centralWidget->update();
}

void MainWindow::save()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save as");

    if(fileName.isEmpty())
        return;

    QFile outFile(fileName);
    outFile.open(QIODevice::WriteOnly);

    outFile.write((const char*)(ui->centralWidget->_data), ui->centralWidget->_dataSize);

    outFile.close();
}

