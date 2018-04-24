//==============================================================================
//
//  File        : version.h
//  Description : Header File for the Firmware Core Version Control
//  Author      : Alterman
//  Revision    : 1.0
//
//==============================================================================

#ifndef _VERSION_H_
#define _VERSION_H_

//==============================================================================
//
//                              CONSTANTS
//
//==============================================================================

/* Version maintained by human beings:
 * Increase Major number when new chip supported;
 * Increase Minor number when new features added;
 * Increase Patch number when bug-fix committed.
 */
#define VERSION_MAJOR   (1)
#define VERSION_MINOR   (0)
#define VERSION_PATCH   (0)

//==============================================================================
//
//                              FUNCTION PROTOTYPES
//
//==============================================================================

extern void DVR_Version(MMP_USHORT *major, MMP_USHORT *minor, MMP_USHORT *patch);

#endif //_VERSION_H_
