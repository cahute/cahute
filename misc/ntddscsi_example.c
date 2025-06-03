/* Example file to try including <ddk/ntddscsi.h>.
 *
 * While some definitions of the file are self-sufficient, others, like with
 * OpenWatcom, require including <windows.h> first, which means the inclusion
 * cannot be tested using `check_include_file()` in CMake. */
#include <windows.h>
#include <ddk/ntddscsi.h>

int main(void) {
    return 0;
}
