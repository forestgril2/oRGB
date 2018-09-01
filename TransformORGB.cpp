#include "TransformORGB.h"
#include <QDebug>
#include <QImage>
#include <QVector3D>

TransformORGB::TransformORGB(QObject *parent) : QObject(parent)
{

    float toLCCvals[]   = {0.299,  0.587, 0.114,
                              0.5,    0.5,  -1.0,
                            0.866, -0.866,   0.0};

    float fromLCCvals[] = {1.0,  0.114,  0.7436,
                            1.0,  0.114, -0.4111,
                            1.0, -0.866,  0.1663};

//    toLCC =   QGenericMatrix<3,3,float>(toLCCvals);
//    fromLCC = QGenericMatrix<3,3,float>(fromLCCvals);
}

void TransformORGB::transform(QString filePath)
{
    qDebug() << " ### file: " << filePath << " is being transformed";

    QImage image;
    image.load(filePath);

    for (int i = 0; i< image.width(); ++i)
    {
       for (int j = 0; j< image.height(); ++j)
       {
          QRgb pixel = image.pixel(i, j);

          QVector3D pixelRGB_vals = {qRed(pixel), qGreen(pixel), qBlue(pixel)};
          QVector3D pixelLCC_vals = QVector3D(toLCC*(pixelRGB_vals.toVector4D()));

          QRgb pixelLCC = qRgb(pixelLCC_vals.x(), pixelLCC_vals.y(), pixelLCC_vals.z());
          image.setPixel(i, j, pixelLCC);
       }
    }

    int dotPosition = filePath.indexOf(".");
    QString transformedPath =  filePath.insert(dotPosition, "_transformed");
    QImage imageLCC(image);
    if (!imageLCC.save(transformedPath.replace("file://", ""), "PNG"))
    {
        qDebug() << " ### Error saving file: " << transformedPath;
    }

    emit fileReady(transformedPath);
}
