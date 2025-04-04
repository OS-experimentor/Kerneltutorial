typedef unsigned int uint32_t; // We don't have std libraries (bare-metal env)
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef unsigned long size_t;

/* Now the important bit! The text mode definitions for VGA. Typically, VGA text mode uses 80x25 resolution */
#define VGAWID 80 // VGA width (column)
#define VGAHI 25 // VGA height (row)
#define VGAMEM 0xB8000 // This is where VGA text mode is in the computer memory. For graphics, you will want to use
// the frame buffer (0xA0000 if I remember right!)

#define NULL 0

#define MAXBUFSZ 128 // Max buffer size (for readstr)




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


/* Hello! This is where we begin with part two. This part is all about input. */

// So first, we need to read a byte from an I/O port. We will do this using "inb" NOTE: inb can be used for more things

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Nice! Now we have a function we can use to read from the keyboard port. But, it may(WILL) just write random garbage.
// Thats where ASCII comes into place. We'll need to write a little map code for ASCII:


static const char asciimap[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/* But see, neither of them will do anything on their own. We need a convertor, and an input reader! */

uint8_t getscan() {
    while(!(inb(0x64) & 1)) { } // While the keyboard controller doesn't have a key in it, do nothing until there is a key pressed
    return inb(0x60);
}

/* For the code above, you're going to need to have knowledge of how ports work (nothing hard really)*/

char getch() {
    uint8_t scancode = getscan() // "scancode" contains the scancode returned by getscan()
    if (scancode < 128) { /* If the scancode fits in ASCII bounds */
        return asciimap[scancode] /* Looks inside the asciimap table and matches the scancode with the letter */
    }

    return 0;
}


/* Now for the toughest task of the input sector (yes, this is the toughest one, showing how easy kernel development really is lol)*/

void readstr(char* buffer, size_t bufsize) {
    size_t pos = 0;
    while (1) {
        char c = getch();
        if (c) {
            if (c == '\b' && pos > 0) { // if the key is backspace:
                pos--; // take 1 away from virtual position
                cursorx = (cursorx = 0) ? VGAWID - 1 : cursorx - 1; // new cursor x
                puts(" \b");
            }

            else if (c == '\n') {
                putchr('\n');
                buffer[pos] = '\0';
                break; // stop reading if enter is pressed!
            }

            else if (pos < bufsize - 1) {
                buffer[pos++] = c;
                putchr(c);
            }


        }
    }
}

void krnlMain() {
    clrscr();
    char ibuffer[MAXBUFSZ];
    while (1) {
        puts("PROMPT >>> ");
        readstr(ibuffer, sizeof(ibuffer));
        puts("\n");
        
    }
}
