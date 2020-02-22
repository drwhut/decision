/*
    Decision
    Copyright (C) 2019-2020  Benjamin Beddows

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ASSERT_H
#define ASSERT_H

#include <stdio.h>
#include <string.h>

// Globals used when asserting.
FILE *fp;
char *ptr, *str;
size_t fileLen, strLen;
int flag;

/**
 * \def START_CAPTURE_STDOUT
 * \brief Start redirecting output from STDOUT to a file called stdout.txt
 */
#define START_CAPTURE_STDOUT() \
    { freopen("stdout.txt", "w", stdout); }

/**
 * \def STOP_CAPTURE_STDOUT
 * \brief Stop redirecting output from STDOUT.
 */
#define STOP_CAPTURE_STDOUT() \
    { fclose(stdout); }

/**
 * \def OPEN_CAPTURED_STDOUT()
 * \brief Open stdout.txt
 */
#define OPEN_CAPTURED_STDOUT() \
    { fp = fopen("stdout.txt", "r"); }

/**
 * \def ASSERT_CAPTURED_STDOUT(against)
 * \brief If the captured STDOUT does not match `against`, return 1.
 */
#define ASSERT_CAPTURED_STDOUT(against)        \
    {                                          \
        OPEN_CAPTURED_STDOUT()                 \
        flag = 0;                              \
        if (fp != NULL) {                      \
            str     = against;                 \
            ptr     = str;                     \
            strLen  = strlen(str);             \
            fileLen = 0;                       \
            while (!(feof(fp) || *ptr == 0)) { \
                if (fgetc(fp) != *ptr) {       \
                    flag = 1;                  \
                    break;                     \
                }                              \
                ptr++;                         \
                fileLen++;                     \
            }                                  \
            if (fileLen != strLen) {           \
                flag = 1;                      \
            }                                  \
            fclose(fp);                        \
            if (flag == 1) {                   \
                return 1;                      \
            }                                  \
        } else {                               \
            return 1;                          \
        }                                      \
    }

/**
 * \def ASSERT_EQUAL(int1, int2)
 * \brief If `int1` != `int2`, return 1.
 */
#define ASSERT_EQUAL(int1, int2) \
    {                            \
        if (int1 != int2) {      \
            return 1;            \
        }                        \
    }

#endif // ASSERT_H