#include "TransformORGB.h"
#include <QDebug>
#include <QImage>
#include <QVector3D>
#include <cmath>

TransformORGB::TransformORGB(QObject *parent) : QObject(parent)
{
}

void TransformORGB::transformPixels(QImage& image, QMatrix4x4 toLCC)
{
    for (int i = 0; i< image.width(); ++i)
    {//transorm to LCC
       for (int j = 0; j< image.height(); ++j)
       {
          QRgb pixel = image.pixel(i, j);
          QVector3D pixelRGB_vals = {static_cast<float>(qRed(pixel)),
                                     static_cast<float>(qGreen(pixel)),
                                     static_cast<float>(qBlue(pixel))};

          QVector3D pixelLCC_vals = QVector3D(toLCC*(pixelRGB_vals.toVector4D()));
          QRgb pixelLCC = qRgb(static_cast<int>(std::round(pixelLCC_vals.x())),
                               static_cast<int>(std::round(pixelLCC_vals.y())),
                               static_cast<int>(std::round(pixelLCC_vals.z())));

          image.setPixel(i, j, pixelLCC);
       }
    }
}

void TransformORGB::transform(QString filePath)
{
    qDebug() << " ### file: " << filePath << " is being transformed";

    QString truncatedPath = filePath;
    truncatedPath.replace("file:///", "");

    QImage image;
    if (!image.load(truncatedPath))
    {
        qDebug() << " ### Error loading file: " << truncatedPath;
    }

//    qDebug() << " ### Image format: " << image.format(); //Format_RGB32

    transformPixels(image, toLCC);
    transformPixels(image, fromLCC);

    QString transformedPath = truncatedPath;
    int dotPosition = truncatedPath.indexOf(".");

    transformedPath.insert(dotPosition, "_transformed");
    QImage imageLCC(image);
    if (!imageLCC.save(transformedPath))
    {
        qDebug() << " ### Error saving file: " << transformedPath;
    }


    emit fileReady(transformedPath);
}
