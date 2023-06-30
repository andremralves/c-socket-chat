#ifndef UTILS_H
#define UTILS_H

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

#undef max
#define max(x,y) ((x) > (y) ? (x) : (y))

char *get_current_time();

#endif // !UTILS_H
