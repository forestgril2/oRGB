#include "TransformORGB.h"
#include <QDebug>
#include <QImage>
#include <QVector3D>

TransformORGB::TransformORGB(QObject *parent) : QObject(parent)
{

    float toORGBVals[]   = {0.299,  0.587, 0.114,
                              0.5,    0.5,  -1.0,
                            0.866, -0.866,   0.0};

    float fromORGBVals[] = {1.0,  0.114,  0.7436,
                            1.0,  0.114, -0.4111,
                            1.0, -0.866,  0.1663};

    toORGB =   QGenericMatrix<3,3,float>(toORGBVals);
    fromORGB = QGenericMatrix<3,3,float>(fromORGBVals);
}

void TransformORGB::transform(QString filePath)
{
    qDebug() << " ### file: " << filePath << " is being transformed";

    QImage image;
    image.load(filePath);

    QVector<QRgb> imageLCC;
    QVector<QRgb> imageORGB;

    for (int i = 0; i< image.width(); ++i)
    {
       for (int j = 0; j< image.height(); ++j)
       {
          QRgb pixel = image.pixel(i, j);

          auto r = qRed(pixel);
          auto g = qGreen(pixel);
          auto b = qBlue(pixel);

          QVector3D pixelRGB;
          QVector3D pixelLLC;

          imageLCC.push_back(image.pixel(i,j));
       }
    }

    emit fileReady(filePath);
}
