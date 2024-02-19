/*****************************************************************************
 File:     nwclass.h

   This file is Copyright 1996 by Scott C. Moonen.  All Rights Reserved.

 Purpose:  This file contains definitions for the neural-network object.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

// True/false.

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

// Processing unit types.

#define   UNIT_INPUT    0               // Input unit.
#define   UNIT_INTERNAL 1               // Internal unit.
#define   UNIT_OUTPUT   2               // Output unit.

enum NWErr                              // NetWorks error values.
{
  NW_SUCCESS = 0,                       // No error; successful operation.
  NW_ERR_NETOPEN,                       // A network is already open.
  NW_ERR_NONETOPEN,                     // No network is currently open.
  NW_ERR_NOFILEOPEN,                    // No network file is open.
  NW_ERR_CREATING,                      // Error creating file.
  NW_ERR_OPENING,                       // Error opening file.
  NW_ERR_FILEEXISTS,                    // File already exists.
  NW_ERR_READING,                       // Error reading file.
  NW_ERR_WRITING,                       // Error writing file.
  NW_ERR_BADFILE,                       // Bad or corrupt file.
  NW_ERR_MEMORY,                        // Not enough memory.
  NW_ERR_RECURSIVE,                     // Network contains recursive units.
  NW_ERR_BADPARAM,                      // Bad parameter.
  NW_ERR_CONNEXISTS,                    // Connection already exists.
  NW_ERR_CONNTOSELF,                    // Connect a unit to itself.
  NW_ERR_NOTCONN,                       // Units are not connected.
  NW_ERR_IOCONN,                        // Improper input/output unit conn.
  NW_ERR_NOUNITS,                       // No network units.
  NW_ERR_NOTINPUT,                      // Unit is not an input unit.
  NW_ERR_NOTOUTPUT                      // Unit is not an output unit.
};

struct NWFileHdr                        // Header for a Network file.
{
  short           MagicNum;             // Magic number ("NW" -- 0x574E).
  unsigned long   NumUnits;             // Number of processing units.
  char            _Rsvd[250];           // Reserved -- set to 0.

// Followed by the unit definitions themselves.
};

struct NWFileUnit                       // Unit structure in a file.
{
  unsigned long   X,Y;                  // Unit's coordinates.
  unsigned long   NumInput;             // Number of input connections.
  unsigned int    Type    : 2;          // Type -- input, internal, or output.
  unsigned int    Binary  : 1;          // If TRUE, unit is binary.
  unsigned int    Bias    : 1;          // If TRUE, unit has bias input.
  unsigned int    Sigmoid : 1;          // If output unit & TRUE, uses sigmoid.
  double          BiasWgt;              // Bias interconnection weight.

// If the unit is an input or output unit, it is followed by a short integer
//   specifying the length of the unit's name (including the terminating
//   NULL, then the string itelf (including the terminating NULL), and then
//   the minimum and maximum endpoints of the unit's range (doubles).
// Following is the list of units from which this unit receives input
//   (unsigned longs), and then the weights for each interconnection
//   (doubles).
};

struct NWIODef                          // Input or output unit definition.
{
  char   *Name;                         // Name of unit.
  double  Min,Max;                      // Ranges of values.
};

struct NWUnit                           // Network processing unit.
{
  unsigned long   X,Y;                  // Unit's coordinates.
  unsigned long   NumInput;             // Number of input connections.
  unsigned int    Type    : 2;          // Type -- input, internal, or output.
  unsigned int    Binary  : 1;          // If TRUE, unit is binary.
  unsigned int    Bias    : 1;          // If TRUE, unit has bias input.
  unsigned int    Sigmoid : 1;          // If output unit & TRUE, uses sigmoid.
  unsigned int    Flag1   : 1;          // First binary flag.
  unsigned int    Flag2   : 1;          // Second binary flag.
  unsigned int    Flag3   : 1;          // Third binary flag.
  double          BiasWgt;              // Bias interconnection weight.
  NWIODef        *IODef;                // I/O def'n (if input or output unit).
  unsigned long  *InputUnits;           // List of input interconnections.
  double         *InputWgts;            // Input interconnection weights.
};

class Network                           // Network object.
{
public:
  char            Path[PATH_MAX + 1];   // Network filename.
  FILE           *Handle;               // Handle for network file.
  unsigned long   NumUnits;             // Number of processing units.
  unsigned long   UnitSpace;            // Number of units there's room for.
  unsigned long   NumInput;             // Number of input units.
  unsigned long   NumOutput;            // Number of output units.
  NWUnit        **UnitList;             // List of network units.
  double         *Sum;                  // List of unit weighted sums.
  double         *ActLevel;             // List of unit activation levels.
  double         *Error;                // List of unit error values.
  unsigned long  *BackSeq;              // Back-pass processing sequence.
  double        **Accum;                // Accumulated weight changes.
  double        **Momentum;             // Last weight change.

  Network()
  {
    strcpy(Path,"");
    Handle = NULL;
    NumUnits = UnitSpace = NumInput = NumOutput = 0;
    UnitList = NULL;
    Sum = NULL;
    ActLevel = NULL;
    Error = NULL;
    BackSeq = NULL;
    Accum = NULL;
    Momentum = NULL;
  };

  char *ErrMsg(NWErr error);            // Get message for an error.
  unsigned long ULongSearch(            // Search for unsigned long value.
                  unsigned long key,unsigned long num,unsigned long *list);

  NWErr Open(const char *file);         // Open a network file.
  NWErr Close(void);                    // Close cur net, create new one.
  NWErr Save(const char *file);         // Save network to file.

  NWErr CreateUnit(unsigned long x,     // Create a processing unit.
                   unsigned long y,int type,int binary,int bias,int sigmoid,
                   char *name,double min,double max,unsigned long *index);
  NWErr DeleteUnit(unsigned long unit); // Delete a processing unit.

  NWErr CreateConnection(unsigned long source,  // Create an interconnection.
                         unsigned long dest);
  NWErr DeleteConnection(unsigned long source,  // Delete an interconnection.
                         unsigned long dest);

  NWErr SetupTrain(int accumulate,      // Prepare for training.
                   int momentum);
  NWErr SetupExec(void);                // Prepare for execution.
  NWErr EndTrain(void);                 // Release training resources.
  NWErr EndExec(void);                  // Release execution resources.

  NWErr SetInput(unsigned long unit,    // Set input value.
                 double value);
  NWErr ReadOutput(unsigned long unit,  // Read an output value.
                   double *value);
  NWErr ApplyTarget(unsigned long unit, // Apply target output value.
                    double target);

  NWErr ForwardPass(void);              // Perform forward pass on network.
  NWErr BackwardPass(double eta,        // Perform backward pass on network.
                     double momentum_coeff);

  NWErr ApplyAccum(void);               // Apply accumulated weight changes.
};

// Random-number routines.

void  Randomize32(unsigned long seed);  // Initialize random number sequence.
unsigned long   Rand32(void);           // Generate random number.

