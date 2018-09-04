#include "TransformORGB.h"
#include <QDebug>
#include <QVector3D>
#include <cmath>
#include <algorithm>
#include <set>
#include <unordered_set>
#include <QFile>

using namespace std;

bool TransformORGB::paralelepipedPrepared = false;
std::vector<TransformORGB::Edge> TransformORGB::edges =
{{{0,0,0}, {1,0,0}}, {{0,0,0}, {0,1,0}}, {{1,0,0}, {1,1,0}}, {{0,1,0}, {1,1,0}},  //lower rgb cube base
 {{0,0,0}, {0,0,1}}, {{1,0,0}, {1,0,1}}, {{0,1,0}, {0,1,1}}, {{1,1,0}, {1,1,1}},  //bases' connections
 {{0,0,1}, {1,0,1}}, {{0,0,1}, {0,1,1}}, {{1,0,1}, {1,1,1}}, {{0,1,1}, {1,1,1}}}; //upper rgb cube base

std::vector<TransformORGB::Pixel3f> TransformORGB::vertices =
{{0,0,0}, {1,0,0}, {0,1,0}, {1,1,0},  //lower rgb cube base
{0,0,1}, {1,0,1}, {0,1,1}, {1,1,1}}; //upper rgb cube base

const float TransformORGB::rgbMax = 255.f;

const QMatrix4x4 TransformORGB::toLCC = {0.299f,  0.587f, 0.114f, 0,
                                            0.5,     0.5,   -1.0, 0,
                                         0.866f, -0.866f,    0.0, 0,
                                              0,       0,      0, 1};

TransformORGB::TransformORGB(QObject *parent) : QObject(parent)
{
}

QString vertexForScatterPlot(const QVector3D& v)
{
    return QString("ListElement{ xPos: ") + QString::number(v.x()) + " ; yPos: " + QString::number(v.y()) + " ; zPos: " + QString::number(v.z()) + "}";
}

void TransformORGB::prepareParalellepiped()
{// prepare parallelepiped' edges and vertices for hue clamping and scaling
    transform(edges.begin(), edges.end(), edges.begin(),
                   [&](Edge e){return make_pair(toLCC*(e.first), toLCC*(e.second));}); // LCC parallelepiped edges
    transform(edges.begin(), edges.end(), edges.begin(),
                   [&](Edge e)
    {// let lower luma vertice always come first in edge
        if (!ascendingLuma(e.first, e.second))
        {
            return make_pair(e.second, e.first);
        }
        return e;
    });
    //finally sort in ascending first vertex luma order
    sort(edges.begin(), edges.end(), [](Edge a, Edge b){return ascendingLuma(a.first, b.first);});

    transform(vertices.begin(), vertices.end(), vertices.begin(),
              [&](Pixel3f v){return toLCC*v;}); // LCC parallelepiped vertices

    sort(vertices.begin(), vertices.end(), ascendingLuma); // sort by luma
    paralelepipedPrepared = true;
}

std::vector<int> TransformORGB::activeEdges(float luma)
{//edge, crossing given luma plane, create vertices, which bound the LCC gamut
    if (qFuzzyCompare(luma,0)) return {};
    else if (luma < vertices[1].x()) return {0,1,2};   //up to 0.114
    else if (luma < vertices[2].x()) return {0,1,3,4}; //up to 0.299
    else if (luma < vertices[3].x()) return {1,3,4,5,6}; //up to 0.413
    else if (luma < vertices[4].x()) return {1,4,5,7}; //up to 0.587
    else if (luma < vertices[5].x()) return {4,5,7,8,9}; //up to 0.701
    else if (luma < vertices[6].x()) return {5,7,8,10}; //up to 0.886
    else if (luma < vertices[7].x()) return {7,10,11}; //up to 1.
    else return {};
}

vector<TransformORGB::Pixel3f> TransformORGB::hueBoundaryVertices(float luma)
{//returns a set of vertices, defining LCC hue boundary for a given luma[0,1]
    vector<Pixel3f> ret;
    Pixel3f diff;
    float t = 0;
    auto edgeIndexes = activeEdges(luma);
    for (auto k : edgeIndexes)
    {
        diff = edges[k].second - edges[k].first;
        t = (luma - edges[k].first.x())/(diff.x());
        ret.push_back(edges[k].first + t*(diff));
    }
    return ret;
}

TransformORGB::Pixel3f TransformORGB::clampHue(const Pixel3f& pixel)
{
    if (!paralelepipedPrepared)
    {
        prepareParalellepiped();

        QFile file("E:\\DaneIProgramy\\GRZESIA\\PROGRAMOWANIE\\Projects\\oRGB_Sopic_Task\\edges_for_luma.txt");
        file.open(QIODevice::ReadWrite);
        QTextStream stream( &file );
        for (int i = 0; i <= rgbMax; i+=1)
        {
            auto vertices = hueBoundaryVertices(float(i)/rgbMax);
            for (int j = 0; j < vertices.size(); ++j)
            {
                stream << vertexForScatterPlot(vertices[j]) << "\r\n";
            }
        }
        file.close();
    }

    return pixel;
}

vector<TransformORGB::Pixel3f> TransformORGB::extractPixels(const QImage& image)
{
    vector<Pixel3f> pixels;
    pixels.resize(static_cast<size_t>(image.width() * image.height()));
    QRgb pixelRGB;
    for (int hy = 0; hy < image.height(); ++hy)
    {
       for (int wx = 0; wx < image.width(); ++wx)
       {
          pixelRGB = image.pixel(wx, hy);
          pixels[static_cast<size_t>(hy*image.width() + wx)] =
                  QVector3D(qRed(pixelRGB)/rgbMax, qGreen(pixelRGB)/rgbMax, qBlue(pixelRGB)/rgbMax);
       }
    }

    return pixels;
}

void TransformORGB::fillImage(QImage& image, const vector<QVector3D>& floatPixels)
{
    QVector3D pixelRGB;
    for (int hy = 0; hy < image.height(); ++hy)
    {
       for (int wx = 0; wx < image.width(); ++wx)
       {
          pixelRGB = floatPixels[static_cast<size_t>(hy*image.width() + wx)];
          image.setPixel(wx, hy, qRgb(static_cast<int>(round(pixelRGB.x()*rgbMax)),
                                      static_cast<int>(round(pixelRGB.y()*rgbMax)),
                                      static_cast<int>(round(pixelRGB.z()*rgbMax))));
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

TransformORGB::Pixel3f TransformORGB::hueRotation(Pixel3f pixelLCC, function<double(double)> angleTransform)
{
    double theta = static_cast<double>(atan2(pixelLCC.z(), pixelLCC.y())); //(-Pi, Pi)
    int sign = signbit(theta) ? -1 : 1;
    theta *= sign;

    double newTheta = angleTransform(theta);

    theta *= sign;
    newTheta *= sign;

    QMatrix4x4 rot;
    rot.rotate(static_cast<float>(theta - newTheta), Pixel3f(1.,0.,0.));

    return rot*pixelLCC;
};

bool TransformORGB::ascendingLuma(Pixel3f a, Pixel3f b)
{
    return a.x() < b.x();
}

void TransformORGB::run(QString filePath)
{
    qDebug() << " ### file: " << filePath << " is being transformed";

    QString truncatedPath = filePath;
    truncatedPath.replace("file:///", "");

    QImage image;
    if (!image.load(truncatedPath))
    {
        qDebug() << " ### Error loading file: " << truncatedPath;
    }

    auto pixels = extractPixels(image);

    //Transform to LCC
    transform(pixels.begin(), pixels.end(), pixels.begin(),
                   [&](Pixel3f p) {return Pixel3f(toLCC * p.toVector4D());} );

    auto luma = [](Pixel3f p){return p.x();};
    auto c1 = [](Pixel3f p){return p.y();};
    auto c2 = [](Pixel3f p){return p.z();};

    auto lumaMinMax = minmax_element(pixels.begin(), pixels.end(),
         [](const Pixel3f& a, const Pixel3f& b){ return a.x() < b.x();});

    auto lMin = luma(*lumaMinMax.first);
    auto lMax = luma(*lumaMinMax.second);

    auto c1MinMax = minmax_element(pixels.begin(), pixels.end(),
                                 [&](const Pixel3f& a, const Pixel3f& b){return (c1(a) < c1(b));});
    auto c1Min = c1(*c1MinMax.first);
    auto c1Max = c1(*c1MinMax.second);

    auto c2MinMax = minmax_element(pixels.begin(), pixels.end(),
                                 [&](const Pixel3f& a, const Pixel3f& b){return (c2(a) < c2(b));});
    auto c2Min = c2(*c2MinMax.first);
    auto c2Max = c2(*c2MinMax.second);

    //Transform to oRGB
    transform(pixels.begin(), pixels.end(), pixels.begin(),
                   bind(hueRotation, placeholders::_1, compressHueAngle));

    //Adjust hue
    transform(pixels.begin(), pixels.end(), pixels.begin(),
                   [&](Pixel3f p) {return Pixel3f(p.x(), p.y(), p.z());});

    //Transform back to LCC
    transform(pixels.begin(), pixels.end(), pixels.begin(),
                   bind(hueRotation, placeholders::_1, decompressHueAngle));


    transform(pixels.begin(), pixels.end(), pixels.begin(), clampHue);

    lumaMinMax = minmax_element(pixels.begin(), pixels.end(),
                                 [&](Pixel3f a, Pixel3f b){return luma(a) < luma(b);});
    lMin = luma(*lumaMinMax.first);
    lMax = luma(*lumaMinMax.second);

    c1MinMax = minmax_element(pixels.begin(), pixels.end(),
                                 [&](Pixel3f a, Pixel3f b){return c1(a) < c1(b);});
    c1Min = c1(*c1MinMax.first);
    c1Max = c1(*c1MinMax.second);

    c2MinMax = minmax_element(pixels.begin(), pixels.end(),
                                 [&](Pixel3f a, Pixel3f b){return c2(a) < c2(b);});
    c2Min = c2(*c2MinMax.first);
    c2Max = c2(*c2MinMax.second);

    float aveLuma = (static_cast<float>(pow(pixels.size(), -1)) *
                      accumulate(pixels.begin(), pixels.end(), 0,
                                 [&](int sum, Pixel3f add){return sum + luma(add);}));

    auto compressLuma = [&](Pixel3f& p) {
        float l = luma(p);
        float c1 = p.y();
        float c2 = p.z();
        float beta = 2.f/3.f;

        if ((l > aveLuma) && (lMax > 1))
        {
            l = aveLuma + (1 - aveLuma) * static_cast<float>(pow((l - aveLuma)/(lMax - aveLuma), beta));
            return Pixel3f(l, c1, c2);
        }
        else if ((l <= aveLuma) && (lMin < 1))
        {
            l = aveLuma * (1 - static_cast<float>(pow((l - aveLuma)/(lMin - aveLuma), beta)));
            return Pixel3f(l, c1, c2);
        }
        return p;
    };
    //Make sure luma is in [0,1]
    transform(pixels.begin(), pixels.end(), pixels.begin(), compressLuma);

    //Transform back to RGB
    transform(pixels.begin(), pixels.end(), pixels.begin(),
                   [&](Pixel3f p) {return Pixel3f(toLCC.inverted() * p.toVector4D());} );


    fillImage(image, pixels);

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
