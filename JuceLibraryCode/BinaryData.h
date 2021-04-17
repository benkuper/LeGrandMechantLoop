/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   crash_png;
    const int            crash_pngSize = 78643;

    extern const char*   default_lgmllayout;
    const int            default_lgmllayoutSize = 2599;

    extern const char*   icon_png;
    const int            icon_pngSize = 106933;

    extern const char*   link_png;
    const int            link_pngSize = 1731;

    extern const char*   p_png;
    const int            p_pngSize = 605;

    extern const char*   play_png;
    const int            play_pngSize = 2007;

    extern const char*   stop_png;
    const int            stop_pngSize = 1978;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 7;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
