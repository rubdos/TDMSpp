#include <iostream>
#include <vector>

#include <tdms.hpp>
#include <log.hpp>

#include "optionparser.h"

// Define options
enum optionIndex {UNKNOWN, HELP, PROPERTIES, DEBUG};

const option::Descriptor usage[] = 
{
    {UNKNOWN,    0, "" , "",           option::Arg::None, "USAGE: tdmsppinfo [options] filename ...\n\n"
                                                          "Options:"},
    {HELP,       0, "h", "help",       option::Arg::None, "  --help, \tPrint usage and exit."},
    {PROPERTIES, 0, "p", "properties", option::Arg::None, "  --properties, \tPrint channel properties."},
    {DEBUG,      0, "d", "debug",      option::Arg::None, "  --debug, \tPrint debugging information to stderr."},
    {0, 0, 0, 0, 0, 0}
};

int main(int argc, char** argv)
{
    // Parse options
    argc -= (argc>0); argv+=(argc>0); // Skip the program name if present
    option::Stats stats(usage, argc, argv);
    option::Option options[stats.options_max], buffer[stats.buffer_max];
    option::Parser parse(usage, argc, argv, options, buffer);

    if(parse.error())
    {
        std::cerr << "parse.error() != 0" << std::endl;
        return 1;
    }
    if(options[HELP] || argc == 0 || options[UNKNOWN])
    {
        option::printUsage(std::cout, usage);
        return 0;
    }
    if(options[DEBUG])
    {
        TDMS::log::debug.debug_mode = true;
    }

    std::vector<std::string> _filenames;
    for(size_t i = 0; i < parse.nonOptionsCount(); ++i)
    {
        _filenames.push_back(parse.nonOption(i));
    }
    for(std::string filename : _filenames)
    {
        if(_filenames.size() > 1)
            std::cout << filename << ":" << std::endl;
        TDMS::file f(filename);
        for(TDMS::object* o : f)
        {
            std::cout << o->get_path() << std::endl;
            if(options[PROPERTIES])
            {
                for(auto p: o->get_properties())
                {
                    std::cout << "  " << p.first << ": " << "value" << std::endl;
                    // TODO: implement value for C++ usage.
                }
            }
        }
    }
}
