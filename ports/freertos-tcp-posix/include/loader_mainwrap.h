#ifndef FREERTOS_LOADER_MAINWRAP_H
#define FREERTOS_LOADER_MAINWRAP_H

/**
 * @brief Declare main function to avoid the 'no previous prototype' warning
 *  when main function is wrapped (e.g. in the FreeRTOS port). In that case the
 *  main function is renamed to real_main by using a definition for main passed
 *  to the compiler on the command line (-D main=real_main).
 */
#ifdef main
int main(int argc, char **argv);
#endif

#endif /* FREERTOS_LOADER_MAINWRAP_H */
