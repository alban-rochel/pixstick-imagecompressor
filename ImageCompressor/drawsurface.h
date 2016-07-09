#ifndef DRAWSURFACE_H
#define DRAWSURFACE_H

#include <QWidget>

uint16_t convertColor(uint16_t r, uint16_t g, uint16_t b);

QColor toQColor(uint16_t color);


class DrawSurface : public QWidget
{
    Q_OBJECT
public:
    explicit DrawSurface(QWidget *parent = 0);

    virtual ~DrawSurface();

    uint8_t* _data;
    uint32_t _dataSize;

signals:

protected:

    virtual void paintEvent(QPaintEvent*) override;

};

#endif // DRAWSURFACE_H
