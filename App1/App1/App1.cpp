#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/utils/trace.hpp>
#include <opencv2/core/opencl/opencl_info.hpp>
#include <iostream>
#include "spdlog/spdlog.h"

std::shared_ptr<spdlog::logger> logger = nullptr; // but in base lib
spdlog::logger& getLogger() { return *logger; }

int main(int argc, char** argv){
    CV_TRACE_FUNCTION();
    CV_TRACE_ARG(argc);
    CV_TRACE_ARG_VALUE(argv0, "argv0", argv[0]);
    CV_TRACE_ARG_VALUE(argv1, "argv1", argv[1]);
    logger = spdlog::stdout_color_mt("console");
    getLogger().info("Welcome to the LAB!");

    cv::CommandLineParser parser(argc, argv,
        "{ help h usage ? |      | show this help message }"
        "{ verbose v      |      | show build configuration log }"
        "{ opencl         |      | show information about OpenCL (available platforms/devices, default selected device) }"
        "{ hw             |      | show detected HW features (see cv::checkHardwareSupport() function). Use --hw=0 to show available features only }"
    );
    if (parser.has("help"))  {
        parser.printMessage();
        return 0;
    }
    if (parser.has("verbose"))  {
        std::cout << cv::getBuildInformation().c_str() << std::endl;
    }
    else {
        std::cout << CV_VERSION << std::endl;
    }
    if (parser.has("opencl"))
    {
        cv::dumpOpenCLInformation();
    }
    if (parser.has("hw"))   {
        bool showAll = parser.get<bool>("hw");
        std::cout << "OpenCV's HW features list:" << std::endl;
        int count = 0;
        for (int i = 0; i < CV_HARDWARE_MAX_FEATURE; i++)
        {
            cv::String name = cv::getHardwareFeatureName(i);
            if (name.empty())
                continue;
            bool enabled = cv::checkHardwareSupport(i);
            if (enabled)
                count++;
            if (enabled || showAll) {
                printf("    ID=%3d (%s) -> %s\n", i, name.c_str(), enabled ? "ON" : "N/A");
            }
        }
        std::cout << "Total available: " << count << std::endl;
    }

    cv::Mat image;
    image = cv::imread(argv[1], cv::IMREAD_COLOR); // Read the file

    if (!image.data) { // Check for invalid input
        getLogger().error("Could not open or find the image");
        return -1;
    }

    cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE); // Create a window for display.
    cv::imshow("Display window", image); // Show our image inside it.

    cv::waitKey(0); // Wait for a keystroke in the window
    return 0;
}
