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


public:
    typedef QVector3D Pixel3f; //float per color should be enough
    typedef std::pair<Pixel3f, Pixel3f> Edge;
    explicit TransformORGB(QObject *parent = nullptr);
    Q_INVOKABLE void run(QString filePath);

signals:
    void fileReady(QString filePath);

private:
    const QMatrix4x4 toLCC = {0.299f,  0.587f, 0.114f, 0,
                                 0.5,     0.5,   -1.0, 0,
                              0.866f, -0.866f,    0.0, 0,
                                   0,       0,      0, 1};

    std::vector<Pixel3f> extractPixels(const QImage& image);
    void fillImage(QImage& image, const std::vector<Pixel3f>& floatPixels);
    static double compressHueAngle(double theta);
    static double decompressHueAngle(double theta);
    static Pixel3f hueRotation(Pixel3f pixelLCC, std::function<double(double)> angleTransform);
    void clampHue(std::vector<Pixel3f>& pixelsLCC);
    std::vector<Pixel3f> hueBoundaryVertices(float luma);
    std::vector<int> activeEdges(float luma);
    static bool ascendingLuma(Pixel3f a, Pixel3f b);

    std::vector<Edge> edges; //to keep paralellepiped edges - sorted
    std::vector<Pixel3f> vertices; //to keep paralellepiped vertices - sorted
    const float rgbMax = 255.f;
};

#endif // TRANSFORMORGB_H
