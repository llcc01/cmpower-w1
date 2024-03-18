#ifndef __SY7T609_CALIBRATION_H__
#define __SY7T609_CALIBRATION_H__

#include <Arduino.h>

/* On-Chip Calibration Routines
 * The SY7T609+S1 includes current and voltage and die temperature calibration
 * routines. These routines modify 'gain' and 'offset coefficients'.
 *
 * The user can set and start a calibration routine through the Command
 * register. When the calibrationi process completes, command register bits
 * 23:16 are cleared along with bits associated with channels that calibrated
 * successfully. Any channels that failed will have their corresponding bit left
 * set. After completion of the calibration, the new coefficients can be saved
 * into flash memory as defaults by issuing the Save to Flash Command.
 */

/* Voltage Gain Calibration using VRMS target
 *
 */

/* Current Gain Calibration using IRMS target
 *
 */

#endif  //__SY7T609_CALIBRATION_H__
