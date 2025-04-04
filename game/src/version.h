#pragma once


#define DAX_MAJOR 0
#define DAX_MINOR 3
#define DAX_PATCH 1
#define DAX_STR(x) DAX_STR2(x)
#define DAX_STR2(x) #x
#define DAX_VERSION DAX_STR(DAX_MAJOR) "." DAX_STR(DAX_MINOR) "." DAX_STR(DAX_PATCH)
#define DAX_VERSION_TEXT DAX_VERSION " (BETA)"
#define DAX_VERSION_FULL_TEXT "Daxanadu " DAX_VERSION_TEXT
