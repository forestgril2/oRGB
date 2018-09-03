#ifndef TRANSFORMORGB_H
#define TRANSFORMORGB_H

#include <QObject>
#include <QMatrix4x4>
#include <QImage>
#include <functional>
#include <vector>

class TransformORGB : public QObject
{
    Q_OBJECT
public:
    explicit TransformORGB(QObject *parent = nullptr);
    Q_INVOKABLE void transform(QString filePath);

signals:
    void fileReady(QString filePath);

private:
    const QMatrix4x4 toLCC = {0.299,  0.587,   0.114, 0,
                                0.5,    0.5,    -1.0, 0,
                              0.866, -0.866,     0.0, 0,
                                  0,      0,       0, 1};

    std::vector<QVector3D> extractFloatRGBPixels(const QImage& image);
    void fillImageWithFloatPixels(QImage& image, const std::vector<QVector3D>& floatPixels);
};

#endif // TRANSFORMORGB_H
