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
    static const QMatrix4x4 toLCC;

    std::vector<Pixel3f> extractPixels(const QImage& image);
    void fillImage(QImage& image, const std::vector<Pixel3f>& floatPixels);
    static const float rgbMax;

    static double compressHueAngle(double theta);
    static double decompressHueAngle(double theta);
    static Pixel3f hueRotation(Pixel3f pixelLCC, std::function<double(double)> angleTransform);
    static Pixel3f clampHue(const Pixel3f& pixel);
    static std::vector<Pixel3f> hueBoundaryVertices(float pixelLuma);
    static std::vector<unsigned> activeEdges(float pixelLuma);
    static float pixelLuma(Pixel3f p) { return p.x(); }
    static bool ascendingLuma(Pixel3f a, Pixel3f b);
    static void prepareParalellepiped();

    static bool paralelepipedPrepared;
    static std::vector<Edge> edges; //to keep paralellepiped edges - sorted
    static std::vector<Pixel3f> vertices; //to keep paralellepiped vertices - sorted
};

#endif // TRANSFORMORGB_H
