#include "TransformORGB.h"
#include <QDebug>
#include <QVector3D>
#include <cmath>
#include <algorithm>
#include <vector>
#include <utility>

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
       for (int j = 0; j < image.height(); ++j)
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

    transformPixels(image, [&](QRgb& pixel){matrixTransformPixel(pixel, toLCC);});

    vector<QRgb> pixels;
    pixels.resize(static_cast<size_t>(image.width() * image.height()));
    for (int y = 0; y < image.height(); ++y)
    {
       for (int x = 0; x < image.width(); ++x)
       {
          pixels[static_cast<size_t>(y*image.width() + x)] = image.pixel(x, y);
       }
    }


    auto luma = [](QRgb p){return qRed(p);};
    auto minMax = minmax_element(pixels.begin(), pixels.end(),
                                 [&](QRgb a, QRgb b){return luma(a) < luma(b);});
    auto lMin = luma(*minMax.first);
    auto lMax = luma(*minMax.second);

    double aveLuma = (pow(pixels.size(), -1) *
                      accumulate(pixels.begin(), pixels.end(), 0, [&](int sum, QRgb add){return sum + luma(add);}));

    auto compressLuma = [&](QRgb& p) {
        double l = luma(p);
        int c1 = qGreen(p);
        int c2 = qBlue(p);
        double beta = 2.0/3.0;

        if ((l > aveLuma) && (lMax > 1.0))
        {
            l = aveLuma + (1.0 - aveLuma) * pow((l - aveLuma) / (lMax - aveLuma), beta);
            return qRgb(static_cast<int>(round(l)), c1, c2);
        }
        else if ((l <= aveLuma) && (lMin < 1.0))
        {
            l = aveLuma * (1.0 - pow((l - aveLuma) / (lMin - aveLuma), beta));
            return qRgb(static_cast<int>(round(l)), c1, c2);
        }
        return p;
    };
    std::transform(pixels.begin(), pixels.end(), pixels.begin(), compressLuma);

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

    for (int y = 0; y < image.height(); ++y)
    {
       for (int x = 0; x < image.width(); ++x)
       {
          image.setPixel(x, y, pixels[static_cast<size_t>(y*image.width() + x)]);
       }
    }

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
