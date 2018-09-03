#ifndef TRANSFORMORGB_H
#define TRANSFORMORGB_H

#include <QObject>
#include <QMatrix4x4>
#include <QImage>
#include <functional>
#include <vector>
#include <utility>

class TransformORGB : public QObject
{
    Q_OBJECT

    typedef std::pair<QVector3D,QVector3D> Edge;

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
    static double compressHueAngle(double theta);
    static double decompressHueAngle(double theta);
    static QVector3D hueRotation(QVector3D pixelLCC, std::function<double(double)> angleTransform);
    void rescaleHue(std::vector<QVector3D>& pixelsLCC);
    std::vector<QVector3D> hueBoundary(std::vector<QVector3D> vertices, std::vector<Edge> edges, float luma);
    std::vector<int> activeEdges(const std::vector<QVector3D>& vertices, const std::vector<Edge>& edges, float luma);
    static bool ascendingLuma(QVector3D a, QVector3D b);
};

#endif // TRANSFORMORGB_H
