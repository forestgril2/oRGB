#include "TransformORGB.h"
#include <QDebug>
#include <QVector3D>
#include <cmath>
#include <algorithm>
#include <utility>

using namespace std;

TransformORGB::TransformORGB(QObject *parent) : QObject(parent)
{
}

vector<QVector3D> TransformORGB::extractFloatRGBPixels(const QImage& image)
{
    vector<QVector3D> pixels; //float should be enough
    pixels.resize(static_cast<size_t>(image.width() * image.height()));
    QRgb pixelRGB;
    for (int hy = 0; hy < image.height(); ++hy)
    {
       for (int wx = 0; wx < image.width(); ++wx)
       {
          pixelRGB = image.pixel(wx, hy);
          pixels[static_cast<size_t>(hy*image.width() + wx)] =
                  QVector3D({static_cast<float>(qRed(pixelRGB)),
                             static_cast<float>(qGreen(pixelRGB)),
                             static_cast<float>(qBlue(pixelRGB))});
       }
    }

    return pixels;
}

void TransformORGB::fillImageWithFloatPixels(QImage& image, const vector<QVector3D>& floatPixels)
{
    QVector3D floatPixelRGB;
    for (int hy = 0; hy < image.height(); ++hy)
    {
       for (int wx = 0; wx < image.width(); ++wx)
       {
          floatPixelRGB = floatPixels[static_cast<size_t>(hy*image.width() + wx)];
          image.setPixel(wx, hy, qRgb(static_cast<int>(round(floatPixelRGB.x())),
                                      static_cast<int>(round(floatPixelRGB.y())),
                                      static_cast<int>(round(floatPixelRGB.z()))));
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

    auto pixels = extractFloatRGBPixels(image);

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

    std::transform(pixels.begin(), pixels.end(), pixels.begin(),
                   [&](QVector3D p) {return QVector3D(toLCC.inverted() * p.toVector4D());} );

    fillImageWithFloatPixels(image, pixels);

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
