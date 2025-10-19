/**
 * @file progress_bar.h
 * @brief Header file for the progress bar function.
 *
 * This file contains the function prototype for updating and displaying the
 * progress bar.
 */

#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

/**
 * @brief Updates and displays the progress bar.
 *
 * @param done The amount of data transferred so far.
 * @param total The total size of the file.
 */
void progressBarUpdate(int done, int total);

#endif // PROGRESS_BAR_H