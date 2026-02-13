// tilt.h
#ifndef TILT_H
#define TILT_H

#include <stdbool.h>

/**
 * @brief Start a non-blocking tilt move to the given degree.
 *
 * Example: Tilt_Start(70) → target ≈ 7.0 V (7000 counts).
 */
void Tilt_Start(int degree);

/**
 * @brief Update the tilt controller.
 *
 * This must be called periodically from the machine daemon loop.
 *
 * @param paused 0 = running, 1 = paused (EN off, state preserved).
 */
void Tilt_Update(int paused);

/**
 * @brief Immediately stop the tilt motion and finish the controller.
 */
void Tilt_Stop(void);

/**
 * @brief Check if the tilt controller has finished (reached target or stopped).
 *
 * @return true if done, false otherwise.
 */
bool Tilt_IsDone(void);

#endif /* TILT_H */
