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
{//float should be enough
    vector<QVector3D> pixels;
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

double TransformORGB::compressHueAngle(double theta)
{
    return theta < (M_PI/3) ? (3.*theta/2.) :
                              M_PI_2 + 0.75*(theta - M_PI/3);
};

double TransformORGB::decompressHueAngle(double theta)
{
    return theta < (M_PI_2) ? (2.*theta/3.) :
                              M_PI/3. + (4./3.)*(theta - M_PI_2);
};

QVector3D TransformORGB::hueRotation(QVector3D pixelLCC, function<double(double)> angleTransform)
{
    double theta = static_cast<double>(atan2(pixelLCC.z(), pixelLCC.y())); //(-Pi, Pi)
    int sign = signbit(theta) ? -1 : 1;
    theta *= sign;

    double newTheta = angleTransform(theta);

    theta *= sign;
    newTheta *= sign;

    QMatrix4x4 rot;
    rot.rotate(static_cast<float>(theta - newTheta), QVector3D(1.,0.,0.));

    return rot*pixelLCC;
};

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

    //Transform to LCC
    std::transform(pixels.begin(), pixels.end(), pixels.begin(),
                   [&](QVector3D p) {return QVector3D(toLCC * p.toVector4D());} );


    //Transform to oRGB
    std::transform(pixels.begin(), pixels.end(), pixels.begin(),
                   bind(hueRotation, placeholders::_1, compressHueAngle));

    //Adjust hue
    std::transform(pixels.begin(), pixels.end(), pixels.begin(),
                   [&](QVector3D p) {return QVector3D(p.x(), 100, 0);});

    auto luma = [](QVector3D p){return p.x();};
    auto minMax = minmax_element(pixels.begin(), pixels.end(),
                                 [&](QVector3D a, QVector3D b){return luma(a) < luma(b);});
    auto lMin = luma(*minMax.first);
    auto lMax = luma(*minMax.second);

    float aveLuma = (static_cast<float>(pow(pixels.size(), -1)) *
                      accumulate(pixels.begin(), pixels.end(), 0,
                                 [&](int sum, QVector3D add){return sum + luma(add);}));

    auto compressLuma = [&](QVector3D& p) {
        float l = luma(p);
        float c1 = p.y();
        float c2 = p.z();
        float beta = static_cast<float>(2./3.);

        if ((l > aveLuma) && (lMax > 255))
        {
            l = aveLuma + (1 - aveLuma) * static_cast<float>(pow((l - aveLuma)/(lMax - aveLuma), beta));
            return QVector3D(l, c1, c2);
        }
        else if ((l <= aveLuma) && (lMin < 255))
        {
            l = aveLuma * (1 - static_cast<float>(pow((l - aveLuma)/(lMin - aveLuma), beta)));
            return QVector3D(l, c1, c2);
        }
        return p;
    };
    //Make sure luma is in [0,1]
    std::transform(pixels.begin(), pixels.end(), pixels.begin(), compressLuma);

    //Transform back to LCC
    std::transform(pixels.begin(), pixels.end(), pixels.begin(),
                   bind(hueRotation, placeholders::_1, decompressHueAngle));

    //Transform back to RGB
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

//
