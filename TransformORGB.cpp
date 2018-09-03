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

QVector3D TransformORGB::matrixTransformFloatPixel(QVector3D& pixel, const QMatrix4x4& matrix)
{
    return QVector3D(matrix * pixel.toVector4D());
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

    using pixelLCC = QVector3D;
    vector<pixelLCC> pixels;
    pixels.resize(static_cast<size_t>(image.width() * image.height()));
    for (int hy = 0; hy < image.height(); ++hy)
    {
       for (int wx = 0; wx < image.width(); ++wx)
       {
          pixels[static_cast<size_t>(hy*image.width() + wx)].setX(qRed(image.pixel(wx, hy)));
          pixels[static_cast<size_t>(hy*image.width() + wx)].setY(qGreen(image.pixel(wx, hy)));
          pixels[static_cast<size_t>(hy*image.width() + wx)].setZ(qBlue(image.pixel(wx, hy)));
       }
    }

    std::transform(pixels.begin(), pixels.end(), pixels.begin(),
                   [&](QVector3D p) {return QVector3D(toLCC * p.toVector4D());} );

//    auto luma = [](QRgb p){return qRed(p);};
//    auto minMax = minmax_element(pixels.begin(), pixels.end(),
//                                 [&](QRgb a, QRgb b){return luma(a) < luma(b);});
//    auto lMin = luma(*minMax.first);
//    auto lMax = luma(*minMax.second);

//    double aveLuma = (pow(pixels.size(), -1) *
//                      accumulate(pixels.begin(), pixels.end(), 0, [&](int sum, QRgb add){return sum + luma(add);}));

//    auto compressLuma = [&](QRgb& p) {
//        double l = luma(p);
//        int c1 = qGreen(p);
//        int c2 = qBlue(p);
//        double beta = 2.0/3.0;

//        if ((l > aveLuma) && (lMax > 255))
//        {
//            l = aveLuma + (1.0 - aveLuma) * pow((l - aveLuma) / (lMax - aveLuma), beta);
//            return qRgb(static_cast<int>(round(l)), c1, c2);
//        }
//        else if ((l <= aveLuma) && (lMin < 255))
//        {
//            l = aveLuma * (1.0 - pow((l - aveLuma) / (lMin - aveLuma), beta));
//            return qRgb(static_cast<int>(round(l)), c1, c2);
//        }
//        return p;
//    };
//    std::transform(pixels.begin(), pixels.end(), pixels.begin(), compressLuma);

//    for (int y = 0; y < image.height(); ++y)
//    {
//       for (int x = 0; x < image.width(); ++x)
//       {
//          image.setPixel(x, y, pixels[static_cast<size_t>(y*image.width() + x)]);
//       }
//    }

    std::transform(pixels.begin(), pixels.end(), pixels.begin(),
                   [&](QVector3D p) {return QVector3D(toLCC.inverted() * p.toVector4D());} );

    for (int hy = 0; hy < image.height(); ++hy)
    {
       for (int wx = 0; wx < image.width(); ++wx)
       {
          auto pixelRGB = pixels[static_cast<size_t>(hy*image.width() + wx)];
          image.setPixel(wx, hy, qRgb(static_cast<int>(round(pixelRGB.x())),
                                      static_cast<int>(round(pixelRGB.y())),
                                      static_cast<int>(round(pixelRGB.z()))));
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

    emit fileReady("file:///" + transformedPath);
}
