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
    /******* EDGES *******/
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

    /******* VERTICES *******/
    transform(vertices.begin(), vertices.end(), vertices.begin(),
              [&](Pixel3f v){return toLCC*v;}); // LCC parallelepiped vertices

    sort(vertices.begin(), vertices.end(), ascendingLuma); // sort by luma
    paralelepipedPrepared = true;

//     I LOVE THIS DEBUG TOO MUCH ;)
//
//    QFile file("E:\\DaneIProgramy\\GRZESIA\\PROGRAMOWANIE\\Projects\\oRGB_Sopic_Task\\edges_for_luma.txt");
//    file.open(QIODevice::ReadWrite);
//    QTextStream stream( &file );
//    for (int i = 0; i <= 1024; i+=1)
//    {
//        auto vertices = hueBoundaryVertices(float(i)/1024);
//        for (int j = 0; j < vertices.size(); ++j)
//        {
//            stream << vertexForScatterPlot(vertices[j]) << "\r\n";
//        }
//    }
//    file.close();
}

std::vector<unsigned> TransformORGB::activeEdges(float luma)
{// edge, crossing given luma plane, create vertices, which bound the LCC gamut
 // could be created automatically ... but this is good enough for this task
    if (qFuzzyCompare(luma, 0.f)) return {};
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
{//returns a set of vertices, defining LCC hue boundary for a given luma<0,1>
    if (!paralelepipedPrepared)
    {
        prepareParalellepiped();
    }

    if (qFuzzyCompare(luma, 0.f))
    {
        return {{0,0,0}};
    }
    else if (qFuzzyCompare(luma, 1.f))
    {
        return {{1,0,0}};
    }
    vector<Pixel3f> ret;
    Pixel3f diff;
    float t = 0;
    auto edgeIndexes = activeEdges(luma);
    for (auto k : edgeIndexes)
    {
        diff = edges[k].second - edges[k].first;
        t = (luma - edges[k].first.x()) / pixelLuma(diff);
        ret.push_back(edges[k].first + t*diff);
    }
    return ret;
}

TransformORGB::Pixel3f TransformORGB::clampHue(const Pixel3f& pixel)
{
    if (qFuzzyCompare(pixel.y(), 0) && qFuzzyCompare(pixel.y(), 0))
    {//handle special case, neutral grey hue, always inside the paralellepiped
        return pixel;
    }

    if (qFuzzyCompare(pixelLuma(pixel), 1.f))
    {//for maximal luma, we have full white, neutral
        return Pixel3f(1,0,0);
    }

    if (qFuzzyCompare(pixelLuma(pixel), 0.f))
    {//for minimal luma, we have total black, neutral
        return Pixel3f(0,0,0);
    }

    auto vertices = hueBoundaryVertices(pixel.x());
    using angleVertexPair = pair<float, Pixel3f>;
    vector<angleVertexPair> pairs(vertices.size());
    auto getPositiveAngle = [](float alpha) { return alpha >= 0 ? alpha : static_cast<float>(2*M_PI) + alpha; };
    transform(vertices.begin(), vertices.end(), pairs.begin(),
              [&](const Pixel3f& p) { return make_pair(getPositiveAngle(atan2(p.z(), p.y())), p); });
    sort(pairs.begin(), pairs.end(),
         [](const angleVertexPair& a, const angleVertexPair& b) { return a.first < b.first; });

    float angle = getPositiveAngle(atan2(pixel.z(), pixel.y()));
    angleVertexPair p1;
    angleVertexPair p2;
    bool debugTestPairFound = false;

    if (angle < pairs[0].first || angle > pairs[pairs.size() -1].first)
    {// angle around 0 between last and first vertice
        p1 = pairs[0];
        p2 = pairs[pairs.size() -1];
        debugTestPairFound = true;
    }
    else
    {// search for the right section
        for (unsigned i = 0; i < pairs.size()-1; ++i)
        {
            p1 = pairs[i];
            p2 = pairs[i+1];

            if (p1.first <= angle && angle < p2.first)
            {//pixel has angle between these two vertices
                debugTestPairFound = true;
                break;
            }
        }
    }

    if (!debugTestPairFound)
    { // pixel should always fall to one of angle ranges
        qDebug() << " ### ERROR: Should never be here!";
        return pixel;
    }

    //we need some simple maths for line section crossings
    auto v = p1.second;
    auto diff = p2.second - p1.second;
    Pixel3f ret(pixel);

    if (qFuzzyCompare(pixel.y(), 0))
    {//handle special case, red-green axis only
        float boundaryZ = pixel.z() * ((v.z() - diff.z() * (v.y()/diff.y())) / pixel.z());
        if (pixel.z() > boundaryZ)
        {
            ret.setZ(boundaryZ);
        }
    }
    else if (qFuzzyCompare(pixel.z(), 0))
    {//handle special case, blue-yellow axis only
        float boundaryY = pixel.y() * ((v.y() - diff.y() * (v.z()/diff.z())) / pixel.y());
        if (pixel.y() > boundaryY)
        {
            ret.setY(boundaryY);
        }
    }
    else
    {
        // otherwise get parameters from full linear solution
        float tBound = (pixel.z()*v.y() - pixel.y()*v.z()) / (pixel.y()*diff.z() - pixel.z()*diff.y());
        float tRadius = (v.y() + tBound*diff.y()) / pixel.y();

        if (tRadius < 1)
        {//need to scale down to paralelepiped boundary
            ret.setY(ret.y()*tRadius);
            ret.setZ(ret.z()*tRadius);
        }
    }
    return ret;
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

void TransformORGB::toORGB(const vector<Pixel3f>& source, std::vector<Pixel3f>& target)
{
    //Transform to LCC
    transform(source.begin(), source.end(), target.begin(),
                   [&](Pixel3f p) {return Pixel3f(toLCC * p.toVector4D());} );

    //Transform to oRGB
    transform(source.begin(), source.end(), target.begin(),
                   bind(hueRotation, placeholders::_1, compressHueAngle));
}

void TransformORGB::fromORGB(const vector<Pixel3f>& source, vector<Pixel3f>& target)
{
    //Transform back to LCC
    transform(source.begin(), source.end(), target.begin(),
                   bind(hueRotation, placeholders::_1, decompressHueAngle));

    {//Make sure luma is in <0,1>
        auto lumaMinMax = minmax_element(target.begin(), target.end(), ascendingLuma);

        auto lMin = pixelLuma(*lumaMinMax.first);
        auto lMax = pixelLuma(*lumaMinMax.second);

        float aveLuma = (static_cast<float>(pow(target.size(), -1)) *
                          accumulate(target.begin(), target.end(), 0,
                                     [&](int sum, Pixel3f add){return sum + pixelLuma(add);}));

        auto compressLuma = [&](Pixel3f& p) {
            float l = pixelLuma(p);
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
        transform(target.begin(), target.end(), target.begin(), compressLuma);
    }

    //clamp or scale hue, clamp for the beginning...
    transform(target.begin(), target.end(), target.begin(), clampHue);

    //Transform back to RGB
    transform(target.begin(), target.end(), target.begin(),
                   [&](Pixel3f p) {return Pixel3f(toLCC.inverted() * p.toVector4D());} );
}

void TransformORGB::writeToImage(QImage& image, const vector<Pixel3f>& pixels,
                                 unsigned startx, unsigned starty, unsigned wpix)
{
    unsigned w = static_cast<unsigned>(image.width());
    unsigned h = static_cast<unsigned>(image.height());
    unsigned nx = wpix;
    unsigned ny = pixels.size()/wpix;
    nx = (nx > w - startx) ? w - startx : nx;
    ny = (ny > h - starty) ? h - starty : ny;

    QVector3D pixelRGB;
    for (unsigned y = starty; y < starty + ny; ++y)
    {
       for (unsigned x = startx; x < startx + nx; ++x)
       {
          pixelRGB = pixels[static_cast<size_t>((y-starty)*wpix + (x-startx))];

          image.setPixel(static_cast<int>(x), static_cast<int>(y),
                         qRgb(static_cast<int>(round(pixelRGB.x()*rgbMax)),
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
    return pixelLuma(a) < pixelLuma(b);
}

void TransformORGB::run(QString filePath)
{
    QString truncatedPath = filePath;
    truncatedPath.replace("file:///", "");

    QImage srcImg;
    if (!srcImg.load(truncatedPath))
    {
        qDebug() << " ### Error loading file: " << truncatedPath;
    }

    auto pixels = extractPixels(srcImg);
    toORGB(pixels, pixels);

    QImage targetImg(srcImg.width()*3, srcImg.height()*3, srcImg.format());

    vector<Pixel3f> hueModifiedPixels(pixels.size());
    const auto hueShift = 0.15f;
    for (int gr = -1; gr <= 1; ++gr)
    {//from green to red
        for (int by = -1; by <= 1; ++by)
        {//from blue to yellow
            transform(pixels.begin(), pixels.end(), hueModifiedPixels.begin(),
                           [&](Pixel3f p) {return Pixel3f(p.x(), p.y() + by*hueShift, p.z() - gr*hueShift);});
            fromORGB(hueModifiedPixels, hueModifiedPixels);
            writeToImage(targetImg, hueModifiedPixels, srcImg.width()*(1+by), srcImg.height()*(1+gr), srcImg.width());
        }
    }

    QString transformedPath = truncatedPath;
    int dotPosition = truncatedPath.indexOf(".");

    transformedPath.insert(dotPosition, "_transformed");
    if (!targetImg.save(transformedPath))
    {
        qDebug() << " ### Error saving file: " << transformedPath;
    }

    emit fileReady("file:///" + transformedPath);
}

//
