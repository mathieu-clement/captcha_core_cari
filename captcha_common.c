#include <stdlib.h>
#include "captcha_common.h"

Coord get_north_coord (Coord c)      { return (Coord){ c.x     , c.y - 1     };  }
Coord get_south_coord (Coord c)      { return (Coord){ c.x     , c.y + 1     };  }
Coord get_west_coord  (Coord c)      { return (Coord){ c.x - 1 , c.y         };  }
Coord get_east_coord (Coord c)       { return (Coord){ c.x + 1 , c.y         };  }
Coord get_south_east_coord (Coord c) { return (Coord){ c.x + 1 , c.y + 1     };  }
Coord get_south_west_coord (Coord c) { return (Coord){ c.x + 1 , c.y - 1     };  }
Coord get_north_east_coord (Coord c) { return (Coord){ c.x - 1 , c.y + 1     };  }
Coord get_north_west_coord (Coord c) { return (Coord){ c.x - 1 , c.y - 1     };  }
bool is_out_coord (Coord c) { 
    return c.x < 0 || c.y < 0 || c.y >= IMG_HEIGHT || c.x >= IMG_WIDTH; 
}
int get_index (int x, int y)  { return y * IMG_WIDTH + x; }
int get_coord_index (Coord c) { return get_index(c.x, c.y); }


// Read text file from remove_noise program and return an array of pixels
// whose value is the group ID, starting from 1 (instead of 17 28 42...)
// The returned array must be free'd by the user of the method.
// PRE: inputf is readable (not NULL)
pixels_struct convert_txt_to_1dim_array(FILE* inputf)
{
    // TODO Replace with bitset
    uint8_t *pixels = malloc(IMG_WIDTH * IMG_HEIGHT * sizeof(uint8_t));
    for (int i=0; i < IMG_WIDTH * IMG_HEIGHT; i++) {
        pixels[i] = 0;
    }

    uint16_t group, x, y;

    // In this method we'll also try to use the smallest group ID as
    // possible.
    uint16_t nbGroups = 0;
    // Index is new groupID, value is old groupID
    // Pixels array contains new groupID + 1
    uint16_t *assoc = malloc(CAPTCHA_ARR_SIZE * sizeof(uint16_t));
    
    {
        char readBuffer[1]; // fread() buffer
        char numberBuffer[4]; // buffer containing chars of read number
        int charCount = 0; // number of chars in number
        bool groupSeen = false; // true until groupID seen, false after that

        // Read one character at a time, until \n or EOF
        while (fread(readBuffer, 1, 1, inputf)) {

            charCount++;

            if (readBuffer[0] == '\n' || feof(inputf)) { // end of file or line
                // store y from last line
                y = char_to_int(numberBuffer, charCount-1);
                charCount = 0;
                groupSeen = false;

                //printf("%u %u %u\n", group, x, y);

                // group is in array?
                int array_index = -1;
                for (int i=0; i < nbGroups; i++) {
                    if (assoc[i] == group) {
                        array_index = i;
                        break;
                    }
                }

                // Get from array or insert into array
                if (array_index != -1) {
                    group = array_index+1;
                } else {
                    assoc[nbGroups] = group;
                    group = nbGroups+1;
                    nbGroups++;
                } // end if

                // Set new group ID in pixels array
                pixels[get_index(x, y)] = group;

                if (feof(inputf)) break;
                else continue;
            } // end if feof

            switch (readBuffer[0]) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    numberBuffer[charCount-1] = readBuffer[0];
                    break;

                case ' ':
                    if (!groupSeen) {
                        group = char_to_int(numberBuffer, charCount-1);
                        groupSeen = true;
                    } else {
                        x = char_to_int(numberBuffer, charCount-1);
                    }
                    charCount = 0;
                    break;

                default:
                    fprintf(stderr, "Read character '%c'. What's that?\n", readBuffer[0]);
                    charCount = 0;
                    break;
            } // end switch
        } // end while fread

    } // end character reading block

    free(assoc);

    return (pixels_struct) { nbGroups, pixels };
} // end txt to 1dim array()

// Convert array of chars to integer
int char_to_int(char *char_array, size_t len)
{
    int result = 0;
    int ten_factor = 1;
    for (int i = len-1; i >= 0; i--) {
        result += (char_array[i] - '0') * ten_factor;
        ten_factor *= 10;
    } // end for

    return result;
} // end char_to_int()


/*
 * The following code is public domain.
 * Algorithm by Torben Mogensen, implementation by N. Devillard.
 * This code in public domain.
 *
 * http://ndevilla.free.fr/median/median/index.html
 */
median_elem_type median(median_elem_type m[], int n)
{
    int         i, less, greater, equal;
    median_elem_type  min, max, guess, maxltguess, mingtguess;

    min = max = m[0] ;
    for (i=1 ; i<n ; i++) {
        if (m[i]<min) min=m[i];
        if (m[i]>max) max=m[i];
    }

    while (1) {
        guess = (min+max)/2;
        less = 0; greater = 0; equal = 0;
        maxltguess = min ;
        mingtguess = max ;
        for (i=0; i<n; i++) {
            if (m[i]<guess) {
                less++;
                if (m[i]>maxltguess) maxltguess = m[i] ;
            } else if (m[i]>guess) {
                greater++;
                if (m[i]<mingtguess) mingtguess = m[i] ;
            } else equal++;
        }
        if (less <= (n+1)/2 && greater <= (n+1)/2) break ; 
        else if (less>greater) max = maxltguess ;
        else min = mingtguess;
    }
    if (less >= (n+1)/2) return maxltguess;
    else if (less+equal >= (n+1)/2) return guess;
    else return mingtguess;
}

median_elem_type mean(median_elem_type m[], int n)
{
    median_elem_type total = 0;

    for (int i=0; i < n; i++) {
        total += m[i];
    }

    return total / n;
}
