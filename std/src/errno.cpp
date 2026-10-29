// <cerrno>
#include <cerrno>

static int errorNumber = 0;

int* karc_errno_location() { return &errorNumber; }
