#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>

#include <boost/filesystem.hpp>

#include "co-nfs/utils.h"

using namespace std;

namespace mutils {

string exec(string cmd)
{
    FILE * pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};
    char buffer[128];
    string res = {};
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL) {
            res += buffer;
        }
    }
    pclose(pipe);
    return res;
}

vector<string> split(string str, string delimiter)
{
    size_t pos = 0;
    vector<string> tokens;
    while ((pos = str.find(delimiter)) != string::npos) {
        tokens.push_back(str.substr(0, pos));
        str.erase(0, pos + delimiter.length());
    }
    if (!str.empty()) {
        tokens.push_back(str);
    }
    return tokens;
}

bool isSubdir(boost::filesystem::path dir, boost::filesystem::path subdir)
{
    int pos = 0;
    string str1 = dir.string();
    string str2 = subdir.string();
    if (boost::filesystem::is_directory(dir)
        && str1.length() < str2.length()
        && (pos = str2.find(str1)) == 0
        && (str1[str1.size()-1] == '/' || str2[str1.size()] == '/') ) {
        
        return true;
    }
    return false;
}

bool copyDir(boost::filesystem::path const & source,
    boost::filesystem::path const & destination)
{
    namespace fs = boost::filesystem;
    try
    {
        // Check whether the function call is valid
        if(
            !fs::exists(source) ||
            !fs::is_directory(source)
        )
        {
            std::cerr << "Source directory " << source.string()
                << " does not exist or is not a directory." << '\n'
            ;
            return false;
        }
        if(fs::exists(destination))
        {
            std::cerr << "Destination directory " << destination.string()
                << " already exists." << '\n'
            ;
            return false;
        }
        // Create the destination directory
        if(!fs::create_directory(destination))
        {
            std::cerr << "Unable to create destination directory"
                << destination.string() << '\n'
            ;
            return false;
        }
    }
    catch(fs::filesystem_error const & e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }
    // Iterate through the source directory
    for(
        fs::directory_iterator file(source);
        file != fs::directory_iterator(); ++file
    )
    {
        try
        {
            fs::path current(file->path());
            if(fs::is_directory(current))
            {
                // Found directory: Recursion
                if(
                    !copyDir(
                        current,
                        destination / current.filename()
                    )
                )
                {
                    return false;
                }
            }
            else
            {
                // Found file: Copy
                fs::copy_file(
                    current,
                    destination / current.filename()
                );
            }
        }
        catch(fs::filesystem_error const & e)
        {
            std:: cerr << e.what() << '\n';
        }
    }
    return true;
}

}