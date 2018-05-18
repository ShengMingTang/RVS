#pragma once

#include "opencv2/core/core.hpp"


struct MeshEquirectangular
{
    cv::Mat3f vertices;
    cv::Mat3f polarAngles;

    cv::Mat3f CalculateVertices( cv::Mat1f radiusMap);
    void CalculatePolarAngles(cv::Size size);

    cv::Vec<bool, 2>      wrap = cv::Vec<bool, 2>(true, false);;
};