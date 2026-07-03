/** Physics simulation demo */
#include "simulation/physics.h"
#include <stdio.h>

int main(void) {
    Particle p = {.pos = {0, 10, 0}, .vel = {1, 0, 0}, .mass = 1.0f};
    Vec3     gravity = {0, -9.81f, 0};

    printf("t=0: pos=(%.2f, %.2f, %.2f)\n", (double)p.pos.x, (double)p.pos.y, (double)p.pos.z);
    for (int i = 1; i <= 10; i++) {
        physics_step_euler(&p, 1, 0.1f, gravity);
        printf("t=%.1f: pos=(%.2f, %.2f, %.2f)\n", (double)((float)i * 0.1f), (double)p.pos.x,
               (double)p.pos.y, (double)p.pos.z);
    }
    return 0;
}
