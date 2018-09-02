#include "TransformORGB.h"
#include <QDebug>
#include <QVector3D>
#include <cmath>
#include <algorithm>
#include <vector>

using namespace std;

TransformORGB::TransformORGB(QObject *parent) : QObject(parent)
{
}

void TransformORGB::matrixTransformPixel(QRgb& pixel, const QMatrix4x4& matrix)
{
    QVector3D pixelRGB_vals = {static_cast<float>(qRed(pixel)),
                               static_cast<float>(qGreen(pixel)),
                               static_cast<float>(qBlue(pixel))};

    QVector3D pixelLCC_vals = QVector3D(matrix * pixelRGB_vals.toVector4D());
    pixel = qRgb(static_cast<int>(round(pixelLCC_vals.x())),
                 static_cast<int>(round(pixelLCC_vals.y())),
                 static_cast<int>(round(pixelLCC_vals.z())));
}

void TransformORGB::transformPixels(QImage& image, function<void(QRgb&)> transform)
{
    QRgb pixel;
    for (int i = 0; i < image.width(); ++i)
    {
       for (int j = 0; j< image.height(); ++j)
       {
          pixel = image.pixel(i, j);
          transform(pixel);
          image.setPixel(i, j, pixel);
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

    transformPixels(image, [&](QRgb& pixel){matrixTransformPixel(pixel, toLCC);});

//    QRgb pixel;
//    auto luma = [](QRgb p){return qRed(p);};
//    for (int i = 0; i< image.width(); ++i)
//    {
//       for (int j = 0; j< image.height(); ++j)
//       {
//          pixel = image.pixel(i, j);
//          int L = luma(pixel);



//          image.setPixel(i, j, pixel);
//       }
//    }

    transformPixels(image, [&](QRgb& pixel){matrixTransformPixel(pixel, toLCC.inverted());});

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
