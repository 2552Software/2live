#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/utils/trace.hpp>
#include <opencv2/core/opencl/opencl_info.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <map>
#include "spdlog/spdlog.h"
#include "opencv2/photo.hpp"

spdlog::logger& getLogger() { return *spdlog::get("console"); }

// true to continue
bool signon(cv::CommandLineParser& parser) {
    if (parser.has("help")) {
        parser.printMessage();
        return false;
    }
    if (parser.has("verbose")) {
        std::cout << cv::getBuildInformation().c_str() << std::endl;
    }
    else {
        std::cout << CV_VERSION << std::endl;
    }
    cv::dumpOpenCLInformation();
    std::cout << "OpenCV's HW features list:" << std::endl;
    int count = 0;
    for (int i = 0; i < CV_HARDWARE_MAX_FEATURE; i++) {
        cv::String name = cv::getHardwareFeatureName(i);
        if (name.empty())
            continue;
        bool enabled = cv::checkHardwareSupport(i);
        if (enabled)
            count++;
        if (enabled) {
            printf("    ID=%3d (%s) -> %s\n", i, name.c_str(), enabled ? "ON" : "N/A");
        }
    }
    std::cout << "Total available: " << count << std::endl;
    return true;
}

// https://stackoverflow.com/a/34734939/5008845
void reduceColor_Quantization(const cv::Mat3b& src, cv::Mat3b& dst)
{
    uchar N = 64;
    dst = src / N;
    dst *= N;
}

// https://stackoverflow.com/a/34734939/5008845
void reduceColor_kmeans(const cv::Mat& src, cv::Mat& dst)
{
    int K = 8;
    int n = src.rows * src.cols;
    cv::Mat data = src.reshape(1, n);
    data.convertTo(data, CV_32F);

    std::vector<int> labels;
    cv::Mat1f colors;
    kmeans(data, K, labels, cv::TermCriteria(), 1, cv::KMEANS_PP_CENTERS, colors);

    for (int i = 0; i < n; ++i)
    {
        data.at<float>(i, 0) = colors(labels[i], 0);
        data.at<float>(i, 1) = colors(labels[i], 1);
        data.at<float>(i, 2) = colors(labels[i], 2);
    }

    cv::Mat reduced = data.reshape(3, src.rows);
    reduced.convertTo(dst, CV_8U);
}

void reduceColor_Stylization(const cv::Mat& src, cv::Mat& dst)
{
    stylization(src, dst);
}

void reduceColor_EdgePreserving(const cv::Mat& src, cv::Mat& dst)
{
    edgePreservingFilter(src, dst);
}


struct lessVec3b
{
    bool operator()(const cv::Vec3b& lhs, const cv::Vec3b& rhs) {
        return (lhs[0] != rhs[0]) ? (lhs[0] < rhs[0]) : ((lhs[1] != rhs[1]) ? (lhs[1] < rhs[1]) : (lhs[2] < rhs[2]));
    }
};

const int w = 500;
cv::Mat src;
//Extract the contours so that
std::vector<std::vector<cv::Point> > contours;
std::vector<cv::Vec4i> hierarchy;

int edgeThresh = 1;
const char* window_name = "Edge Map";

int main(int argc, char** argv){
    CV_TRACE_FUNCTION();
    CV_TRACE_ARG(argc);
    CV_TRACE_ARG_VALUE(argv0, "argv0", argv[0]);
    CV_TRACE_ARG_VALUE(argv1, "argv1", argv[1]);
    spdlog::stdout_color_mt("console");
    getLogger().info("Welcome to the LAB!");

    cv::CommandLineParser parser(argc, argv, 
        "{@input | ../data/fruits.jpg | input image}"
        "{ help h usage ? |      | show this help message }"
        "{ verbose v      |      | show build configuration log }"
    );
    if (!signon(parser)) {
        return 0;
    }
    src = cv::imread(parser.get<cv::String>("@input"), cv::IMREAD_COLOR); // Load an image
    if (src.empty())    {
        std::cout << "Could not open or find the image!\n" << std::endl;
        std::cout << "Usage: " << argv[0] << " <Input image>" << std::endl;
        return -1;
    }
    cv::Mat dst, src_gray, detected_edges, basicBlur, Gaussed, image;

    cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE); // Create a window for display.
    cv::imshow(window_name, src); // Show our image inside it.
    cv::waitKey(10); // Wait for a keystroke in the window
    cvtColor(src, src_gray, cv::COLOR_BGR2GRAY);
    dst.create(src.size(), src.type());// same size as source

                                       // Reduce color
    cv::GaussianBlur(src_gray, Gaussed, cv::Size(5, 5), 1.5);
    imshow(window_name, Gaussed);
    cv::waitKey(100);

    for (int i = 0; i <= 12; ++i) {
        applyColorMap(src_gray, image, (cv::ColormapTypes)i);
        // Show the result:
        cv::imshow(window_name, image);
        cv::waitKey(2000);
    }
  
    int const max_lowThreshold = 100;
    int ratio = 3;
    for (int lowThreshold = max_lowThreshold; lowThreshold > 0; --lowThreshold) {
        Canny(Gaussed, detected_edges, lowThreshold, lowThreshold*ratio, 3);
        dst = cv::Scalar::all(0);
        src.copyTo(dst, detected_edges);
        imshow(window_name, dst);
        cv::waitKey(10);
    }
    //https://stackoverflow.com/questions/35479344/how-to-get-color-palette-from-image-using-opencv
    cv::waitKey(100);
    findContours(detected_edges, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
    /// Draw last shown contours
    cv::Mat drawing = cv::Mat::zeros(detected_edges.size(), CV_8UC3);
    cv::RNG rng(12345);
    for (size_t i = 12000; i< contours.size(); ++i){
        if (contours[i].size() > 0 && (i % 1) == 0 ) {
            cv::Scalar color = cv::Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
            drawContours(drawing, contours, (int)i, color, 2, 8, hierarchy, 0, cv::Point());
            imshow(window_name, drawing);
            cv::waitKey(1);
        }
    }
    cv::waitKey();

    // de color is slow
    cv::Mat gray = cv::Mat(src.size(), CV_8UC1);
    cv::Mat color_boost = cv::Mat(src.size(), CV_8UC3);
    cv::decolor(src, gray, color_boost);
    imshow(window_name, gray);
    cv::waitKey(0);
    imshow(window_name, color_boost);
    cv::waitKey(0);

    cv::Mat3b reduced;
    reduceColor_Quantization(src, reduced);
    imshow(window_name, reduced);
    cv::waitKey();
    reduceColor_kmeans(src, reduced);
    imshow(window_name, reduced);
    cv::waitKey();
    reduceColor_Stylization(src, reduced);
    imshow(window_name, reduced);
    cv::waitKey();
    reduceColor_EdgePreserving(src, reduced);
    imshow(window_name, reduced);
    cv::waitKey();


    //![create_trackbar]
    /// Create a Trackbar for user to enter threshold
    //cv::createTrackbar("Min Threshold:", window_name, &lowThreshold, max_lowThreshold, CannyThreshold);
    //![create_trackbar]
      return 0;
}
