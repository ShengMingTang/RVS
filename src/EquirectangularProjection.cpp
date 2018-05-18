#include "EquirectangularProjection.hpp"


namespace
{
    // Coordinates and indices are designed to sample into the disparity map
    // size == 4, wrap T: { 1/8, 3/8, 5/8, 7/8, 9/8 }      (size + 1), sample points { 1/8, 3/8, 5/8, 7/8, 1/8 }     , pixel indices { 0, 1, 2, 3, 0 }
    //          , wrap F: {  0 , 1/8, 3/8, 5/8, 7/8,  1  } (size + 2), sample points { 1/8, 1/8, 3/8, 5/8, 7/8, 7/8 }, pixel indices { 0, 0, 1, 2, 3, 3 }

    float gridPoint(int index, int size, bool wrap)
    {
        if (wrap) {
            return float((0.5 + index)/size);
        }
        else {
            return std::max(0.f, std::min(1.f, float((-0.5 + index)/size)));
        }
    }

    int gridIndex(int index, int size, bool wrap)
    {
        if (wrap) {
            return index == size ? 0 : index;
        }
        else {
            return std::max(0, std::min(size - 1, index - 1));
        }

    }
}


void MeshEquirectangular::CalculatePolarAngles(cv::Size size)
{
    if( polarAngles.size() == size )
        return;

    polarAngles.create(size);
    for (int i = 0; i != polarAngles.rows; ++i)
    {
        float v = gridPoint(i, size.height, wrap[1]);
        auto theta = float(CV_PI - CV_PI*v);

        for (int j = 0; j != polarAngles.cols; ++j)
        {
            float u = gridPoint(j, size.width, wrap[0]);
            auto phi = float(CV_2PI*u);

            polarAngles(i, j) = cv::Vec3f( std::sin(theta)*std::sin(phi),
                -std::cos(theta),
                -std::sin(theta)*std::cos(phi));
        }
    }
}




cv::Mat3f MeshEquirectangular::CalculateVertices( cv::Mat1f radiusMap)
{
    cv::Size size = radiusMap.size();

    CalculatePolarAngles(size);

    vertices.create(size);

    for (int i = 0; i < size.height; ++i)
    {
        int n = gridIndex(i, size.height, wrap[1]);

        for (int j = 0; j < size.width; ++j)
        {
            int m = gridIndex(j, size.width, wrap[0]);
            auto radius = radiusMap(n, m);
            vertices(i,j) = radius * polarAngles(i, j);
        }
    }

    return vertices;
}

