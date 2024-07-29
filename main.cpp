#include "capture.h"

int main(int argc, char** argv) {

    // init tesseract engine - and set some data, which will make recognizing digits EASIER
    tesseract::TessBaseAPI tess;
    tess.Init(NULL, "eng", tesseract::OEM_TESSERACT_ONLY);
    tess.SetVariable("tessedit_char_whitelist", "123456789");
    tess.SetPageSegMode(tesseract::PSM_SINGLE_CHAR);

    if (argc == 1)
        return activeCamera(tess);

    // it makes testing EASIER
    else if (argc == 3 ) {
        std::string str(argv[1]);

        std::string path = std::filesystem::absolute(std::filesystem::current_path().parent_path().parent_path()).string() + "\\samples\\";
        std::string file = argv[2] + std::string(".jpg");
        
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
        file.erase(remove_if(file.begin(), file.end(), isspace), file.end());

        if ( str == "samples" || str == "s")
            return passiveImage(tess, path + file );
    }

    return passiveImage(tess, argv[1]);

}