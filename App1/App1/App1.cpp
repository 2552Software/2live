#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/utils/trace.hpp>
#include <opencv2/core/opencl/opencl_info.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include "spdlog/spdlog.h"

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
cv::Mat src, src_gray;
cv::Mat dst, detected_edges;

int edgeThresh = 1;
int lowThreshold;
int const max_lowThreshold = 100;
int ratio = 3;
int kernel_size = 3;
const char* window_name = "Edge Map";
static void CannyThreshold(int, void*)
{
    //![reduce_noise]
    /// Reduce noise with a kernel 3x3
    blur(src_gray, detected_edges, cv::Size(3, 3));
    //![reduce_noise]

    //![canny]
    /// Canny detector
    Canny(detected_edges, detected_edges, lowThreshold, lowThreshold*ratio, kernel_size);
    //![canny]

    /// Using Canny's output as a mask, we display our result
    //![fill]
    dst = cv::Scalar::all(0);
    //![fill]

    //![copyto]
    src.copyTo(dst, detected_edges);
    //![copyto]

    //![display]
    imshow(window_name, dst);
    //![display]
}


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
    cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE); // Create a window for display.
    cv::imshow("Display window", src); // Show our image inside it.

    cv::waitKey(0); // Wait for a keystroke in the window

                    /// Create a matrix of the same type and size as src (for dst)
    dst.create(src.size(), src.type());
    //![create_mat]

    //![convert_to_gray]
    cvtColor(src, src_gray, cv::COLOR_BGR2GRAY);
    //![convert_to_gray]

    //![create_trackbar]
    /// Create a Trackbar for user to enter threshold
    cv::createTrackbar("Min Threshold:", window_name, &lowThreshold, max_lowThreshold, CannyThreshold);
    //![create_trackbar]

    /// Show the image
    CannyThreshold(0, 0);

    /// Wait until user exit program by pressing a key
    cv::waitKey(0);

     return 0;
}
