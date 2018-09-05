#ifndef TRANSFORMORGB_H
#define TRANSFORMORGB_H

#define _USE_MATH_DEFINES
#include <cmath>

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
    static std::vector<Pixel3f> extractPixels(const QImage& image);
    static void writeToImage(QImage& image, const std::vector<Pixel3f>& floatPixels,
                             unsigned startx = 0, unsigned starty = 0, unsigned wpix = 0);

    static void toORGB(const std::vector<Pixel3f>& source, std::vector<Pixel3f>& target);
    static void fromORGB(const std::vector<Pixel3f>& source, std::vector<Pixel3f>& target);

    static double compressHueAngle(double theta);
    static double decompressHueAngle(double theta);
    static Pixel3f hueRotation(Pixel3f pixelLCC, std::function<double(double)> angleTransform);

    static void prepareParalellepiped();
    static std::vector<unsigned> activeEdges(float pixelLuma);
    static std::vector<Pixel3f> hueBoundaryVertices(float pixelLuma);
    static float getPositiveAngle(const Pixel3f& pixel);
    static Pixel3f clampHue(const Pixel3f& pixel, float lumaParam, float angleParam);
    static Pixel3f clampPixelHue(const Pixel3f& pixel);

    static void hueScaling(const std::vector<Pixel3f>& source, std::vector<Pixel3f>& target);

    static float pixelLuma(Pixel3f p) { return p.x(); }
    static bool ascendingLuma(Pixel3f a, Pixel3f b);

    static const QMatrix4x4 toLCC;
    static const float rgbMax;
    static bool paralelepipedPrepared;
    static std::vector<Edge> edges; //to keep paralellepiped edges - sorted
    static std::vector<Pixel3f> vertices; //to keep paralellepiped vertices - sorted
};

#endif // TRANSFORMORGB_H
