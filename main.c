/*
 * main.c
 *
 *  Created on: Jun 22, 2012
 *      Author: hammer
 */

/*
 * If having problems with netbeans menu items and mouse cursor position:
 * Unmaximise NetBeans window, move it (top left corner of window) to the top 
 * left corner of the screen then maximise the window.  That's it.
 */

#include <stdio.h>
#include "thread.h"

//align_t heap[80000];
//
//int main() {
//    thStartVM(heap, sizeof (heap) / sizeof (heap[0]), 20000, 4000);
align_t heap[8000];

int main() {
    thStartVM(heap, sizeof (heap) / sizeof (heap[0]), 2000, 5000);

    // Unreachable:
    return 0;
}

void thinj_putchar(char ch) {
    putchar(ch);
}

void thinjvm_exit(int exit_code) {
    exit(exit_code);
}
