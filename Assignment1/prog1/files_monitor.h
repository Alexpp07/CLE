/**
 *  \file files_monitor.h (interface file)
 *
 *  \brief Problem name: Count Words.
 *
 *  In this file the functions to save/get the the counters of each file are defined.
 *  Synchronization based on monitors.
 *  Both threads and the monitor are implemented using the pthread library which enables the creation of a
 *  monitor of the Lampson / Redell type.
 *
 *  Data transfer region implemented as a monitor.
 *
 *  Definition of the operations carried out by the main thread:
 *     \li processFileName
 *     \li printResults
 * 
 *  Definition of the operations carried out by the workers:
 *     \li savePartialResults
 *
 *  \author Eduardo Rocha Fernandes - 98512
 *  \author Alexandre Pinto - 
 */

#ifndef FILES_MONITOR_H
#define FILES_MONITOR_H

/* 
    Function to store the filenames and start the counters for each file.
*/
void processFileName(int argc, char *argv[], char *fileNames[]);

#endif /* FILES_MONITOR_H */
