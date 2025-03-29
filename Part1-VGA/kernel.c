typedef unsigned int uint32_t; // We don't have std libraries (bare-metal env)
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef unsigned long size_t;

/* Now the important bit! The text mode definitions for VGA. Typically, VGA text mode uses 80x25 resolution */
#define VGAWID 80 // VGA width (column)
#define VGAHI 25 // VGA height (row)
#define VGAMEM 0xB8000 // This is where VGA text mode is in the computer memory. For graphics, you will want to use
// the frame buffer (0xA0000 if I remember right!)

static uint16_t* vmem = VGAMEM; // This part is interesting. It establishes a 16 bit pointer to VGA, so VGA acts as a 16 bit value
static size_t cursorx = 0; // This is useful for I/O. Now, cursor is referring to the text cursor (google it), not the mouse cursor!
static size_t cursory = 0;
static uint8_t VGCOL = 0x9B; // This makes an 8-bit value that represents the color of VGA characters.
/* 
The reason it is 8-bit is because VGA requires 2 values: color and the character. VGA, here, is 16-bit. Meaning we need
2 8-bit values to go inside of a 16-bit array. One 16 bit value in VGA is an index, because VGA is an array of characters.
*/


// Now we'll need to lay a foundation for what is to come (string comparing, char to int)

int strcmp(const char *s1, const char *s2) {
    while(*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* strncmp: Compares up to n characters of two strings. */
int strncmp(const char *s1, const char *s2, size_t n) {
    while(n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if(n == 0)
        return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/* A simple atoi: converts a string of digits into an integer.
   Only handles positive numbers. */
int atoi(const char *s) {
    int num = 0;
    while(*s >= '0' && *s <= '9') {
        num = num * 10 + (*s - '0');
        s++;
    }
    return num;
}


/* Now we can get to the juicy bits - what you came here for!*/

void putchr(char c) {
    if (c == '\n') { // Checks if the character in the register (C is made in Assembly) is new line (\n)
        cursorx = 0; // Reset the cursor's x position to the far left of the screen
        if (++cursory >= VGAHI) { // checks if wheen y is increased, it exceeds or is equal to VGA height
            cursory = VGAHI - 1;
        }

        return;
    }
    size_t index = cursory * VGAWID + cursorx;
    vmem[index] = (uint16_t)c | (VGCOL << 8);
    /* Ok, that may be a lot to sink in — stay with me!  
   Remember how VGA text mode uses an array to store characters?  
   Unfortunately, we can’t just tell the computer "put this character at (x, y)."  
   Instead, we have to calculate the correct position in memory.  

   The formula for finding the index (position in the array) is:  
        (row * screen width) + column  

   Now for the second part:  
   - (uint16_t)c tells the computer to store the character as a **16-bit** value.  
   - (VGCOL << 8) shifts the color into the upper (high) byte.  
   - The bitwise OR (|) combines them into a single value, like this:  
        [ COLOR (high byte) | CHARACTER (low byte) ]  

   And that’s how we write text with color in VGA mode!
*/

    if (++cursorx >= VGAWID) {
        cursorx = 0;
        if (++cursory >= VGAHI) {
            cursory = VGAHI - 1;
        }
    }

}


void puts(const char* str) { // this is a loop to check if str exists, then print it to VGA and move to the next char
    while (*str) { // while the string exists:
        putchr(*str++); // put the character, then add another until the string DOESN'T exist (checked by the while loop)
    }
}

void clrscr() {
    for (size_t y = 0; y < VGAHI; y++) { // for loops 101: for every time the row is less than the value of total rows, add to y and do:
        for (size_t x = 0; x < VGAWID; x++) {
            vmem[y * VGAWID + x] = (uint16_t)' ' | (VGCOL << 8);
            // Remember that? That's the same thing you saw earlier! Except this time, we're hardwiring what character
            // we're writing to the screen, which is a blank!
        }
    }

    cursorx = 0;
    cursory = 0;
    // Now we're resetting the position of the text cursor to the top left, but below, we're writing a string. WILL IT OVERWRITE THE
    // STRING???
    // answer: no. in the putchr function, it automatically moves the cursor!

    puts("PLACEHOLDER TEXT <----- HERE YOU MIGHT PUT A WELCOME MSG OR SOMETHING\n");
}

void krnlMain() {
    clrscr();
    while (1) {
        // dead hang
    }
}

/* 
you may be thinking: why do we need a while loop? the answer is because once the screen is cleared, the computer has
nothing left to execute. therefore, it exits the kernel (NOT a good thing!), which results in PLACEHOLDER TEXT not being
printed! (or well, it is, just we can't see it!)
*/