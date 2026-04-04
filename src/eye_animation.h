#ifndef EYE_ANIMATION_H
#define EYE_ANIMATION_H

#include <stdbool.h>

extern int current_anim;
extern int animation_count;

bool parse_manifest(void);
void start_eye_animation(void);

#endif
