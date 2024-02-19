/*****************************************************************************
  File:     nwclass.cpp

    This file is Copyright 1996 by Scott C. Moonen.  All Rights Reserved.

  Purpose:  This file contains the neural-network object's code.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "nwclass.h"

/*****************************************************************************
  Function:   Network::ErrMsg()
  Purpose:    This function returns the error message corresponging with the
              given error value.
  Parameters: NWErr error               The error value.
  Returns:    A string (not terminated by punctuation) containing a message
              describing the given error.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

char *Network::ErrMsg(NWErr error)
{
  char *err_msgs[] =
        {
          "No error",
          "A network is open",
          "No network is open",
          "No network file is open",
          "Error creating file",
          "Error opening file",
          "File already exists",
          "Error reading file",
          "Error writing file",
          "Bad or corrupt file",
          "Not enough memory to complete operation",
          "Network contains a recursive unit chain",
          "Bad parameter",
          "Connection already exists",
          "Cannot connect a unit to itself",
          "Units are not connected",
          "Improper input/output unit interconnection",
          "No units in network",
          "Unit is not an input unit",
          "Unit is not an output unit",
        };

  if(error >= 0 && error <= 19)
    return(err_msgs[error]);
  else
    return("Unknown error");
}

/*****************************************************************************
  Function:   Network::ULongSearch()
  Purpose:    This function searches for an unsigned long entry in the
              given list of sorted entries, using the binary search
              algorithm.
  Parameters: unsigned long key         Value to search for.
              unsigned long num         Number of entries in the list.
              unsigned long *list       List of unsigned long values.
  Returns:    The index of the found entry, or -1 (0xFFFFFFFF) on error.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

unsigned long Network::ULongSearch(unsigned long key, unsigned long num,
                                   unsigned long *list)
{
  unsigned long   base = 0,index;

  while(num)                            // Do while we have some left.
  {
    index = num >> 1;                   // Halfway in.

    if(list[base + index] == key)       // Got it.
      return(base + index);
    else if(list[base + index] > key)   // Before search point.
      num = index;                      // Truncate window.
    else                                // After search point.
    {
      base += index + 1;                // Move up front of window.
      num  -= index + 1;
    }
  }

  return((unsigned long)(-1));
}

/*****************************************************************************
  Function:   Network::Open()
  Purpose:    This function opens the given network file.
  Parameters: char *file                The file to be opened.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
NWErr Network::Open(const char *file)
{
  FILE           *handle;
  unsigned long   ix;
  short           word;
  NWFileHdr       hdr;
  NWUnit        **units;
  NWFileUnit      unit_def;

  if(NumUnits)                          // We have a network.
    Close();                            // Close it first.

  if((handle = fopen(file,"r+b")) == NULL)  // Open file.
    return(NW_ERR_OPENING);             // Error opening file.

  if(fread(&hdr,1,sizeof(NWFileHdr),handle) < sizeof(NWFileHdr))
  {
    fclose(handle);
    return(NW_ERR_READING);             // Error reading file.
  }

  if(hdr.MagicNum != 0x574E)            // Verify magic number.
  {
    fclose(handle);
    return(NW_ERR_BADFILE);             // Bad or corrupt file.
  }

  if((units = new NWUnit *[hdr.NumUnits + 512]) == NULL)
  {
    fclose(handle);
    return(NW_ERR_MEMORY);              // Not enough memory.
  }
  memset(units,0,(hdr.NumUnits + 512) * sizeof(NWUnit *));

  for(ix = 0;ix < hdr.NumUnits;ix++)    // Read in the units.
  {
    if(fread(&unit_def,1,sizeof(NWFileUnit),handle) < sizeof(NWFileUnit))
    {
ReadErr:
      for(;;ix--)
      {
        delete units[ix]->IODef->Name;
        delete units[ix]->IODef;
        delete units[ix]->InputUnits;
        delete units[ix]->InputWgts;
        delete units[ix];
        if(ix == 0)
          break;
      }
      delete[] units;
      fclose(handle);
      return(NW_ERR_READING);           // Error reading file.
    }

    if((units[ix] = new NWUnit) == NULL)
    {
MemErr:
      for(;;ix--)
      {
        delete units[ix]->IODef->Name;
        delete units[ix]->IODef;
        delete units[ix]->InputUnits;
        delete units[ix]->InputWgts;
        delete units[ix];
        if(ix == 0)
          break;
      }
      delete[] units;
      fclose(handle);
      return(NW_ERR_MEMORY);            // Out of memory.
    }
    memset(units[ix],0,sizeof(NWUnit));

    if(unit_def.Type != UNIT_INTERNAL)  // Input/output unit.
    {
      if((units[ix]->IODef = new NWIODef) == NULL)
        goto MemErr;

      if(fread(&word,1,sizeof(short),handle) < sizeof(short))
        goto ReadErr;
      if((units[ix]->IODef->Name = new char[word]) == NULL)
        goto MemErr;

      if(fread(units[ix]->IODef->Name,1,word,handle) < word)
        goto ReadErr;
      if(fread(&units[ix]->IODef->Min,1,sizeof(double),handle) < sizeof(double))
        goto ReadErr;
      if(fread(&units[ix]->IODef->Max,1,sizeof(double),handle) < sizeof(double))
        goto ReadErr;
    }

// Populate unit structure.

    units[ix]->X         = unit_def.X;         // Coordinates.
    units[ix]->Y         = unit_def.Y;
    units[ix]->NumInput  = unit_def.NumInput;  // Number of input conn.'s
    units[ix]->Type      = unit_def.Type;      // Unit type.
    units[ix]->Binary    = unit_def.Binary;    // Binary flag.
    units[ix]->Sigmoid   = unit_def.Sigmoid;   // Sigmoid function flag.
    units[ix]->Bias      = unit_def.Bias;      // Bias input flag.
    units[ix]->BiasWgt   = unit_def.BiasWgt;   // Bias weight.

    if(unit_def.NumInput > 0)           // Some input connections.
    {
      if((units[ix]->InputUnits = new unsigned long[units[ix]->NumInput]) == NULL)
        goto MemErr;
      if((units[ix]->InputWgts = new double[units[ix]->NumInput]) == NULL)
        goto MemErr;

// Read backwards interconnections.

      if(fread(units[ix]->InputUnits,1,unit_def.NumInput * sizeof(unsigned long),handle) < unit_def.NumInput * sizeof(unsigned long))
        goto ReadErr;
      if(fread(units[ix]->InputWgts,1,unit_def.NumInput * sizeof(double),handle) < unit_def.NumInput * sizeof(double))
       goto ReadErr;
    }
  }

  Handle = handle;
  NumUnits = hdr.NumUnits;
  UnitSpace = hdr.NumUnits + 512;       // Room for 512 extra units.
  UnitList = units;
  realpath(file, Path);

// Determine the number of input and output units.

  NumInput = NumOutput = 0;
  for(ix = 0;ix < NumUnits;ix++)
  {
    if(UnitList[ix]->Type == UNIT_INPUT)
      NumInput++;
    else if(UnitList[ix]->Type == UNIT_OUTPUT)
      NumOutput++;
  }

  return(NW_SUCCESS);
}

/*****************************************************************************
  Function:   Network::Close()
  Purpose:    This function closes the currently open network and creates a
              new, empty network.
  Parameters: None.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::Close(void)
{
  unsigned long ix;
  for(ix = 0;ix < NumUnits;ix++)        // Free unit list.
  {
    if(UnitList[ix] == NULL)            // Not allocated.
      continue;

    if(UnitList[ix]->IODef != NULL)
    {
      delete UnitList[ix]->IODef->Name;
      delete UnitList[ix]->IODef;
    }

    if(UnitList[ix]->NumInput)
    {
      delete UnitList[ix]->InputUnits;
      delete UnitList[ix]->InputWgts;
    }

    delete UnitList[ix];
  }

  delete[] UnitList;                    // Free list itself.

  strcpy(Path, "");
  if(Handle != NULL)
  {
    fclose(Handle);
    Handle = NULL;
  }

  NumUnits = UnitSpace = NumInput = NumOutput = 0;
  UnitList = NULL;
  Sum = NULL;
  ActLevel = NULL;
  Error = NULL;
  BackSeq = NULL;
  Accum = NULL;
  Momentum = NULL;

  return(NW_SUCCESS);
}

/*****************************************************************************
  Function:   Network::Save()
  Purpose:    This function saves the current network.
  Parameters: char *file                If NULL, then the file will be saved
                                        under its current name (if it has
                                        one).  If a string, then the file will
                                        be saved under the given name.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::Save(const char *file)
{
  FILE           *handle = Handle;
  unsigned long   ix;
  NWFileHdr       hdr;
  NWFileUnit      unit;

  if(file == NULL && Handle == NULL)    // No open file.
    return(NW_ERR_NOFILEOPEN);

  if(file != NULL)                      // Open a new file.
    if((handle = fopen(file,"w+")) == NULL)
      return(NW_ERR_CREATING);          // Error creating file.

// Write out the header.

  memset(&hdr,0,sizeof(NWFileHdr));
  hdr.MagicNum = 0x574E;                // Magic number.
  hdr.NumUnits = NumUnits;              // Number of units.
  if(fwrite(&hdr,1,sizeof(NWFileHdr),handle) < sizeof(NWFileHdr))
  {
WriteErr:
    if(file != NULL)                    // Opened a new file.
      fclose(handle);
    return(NW_ERR_WRITING);             // Error writing file.
  }

  for(ix = 0;ix < NumUnits;ix++)        // Write out the units.
  {
    unit.X        = UnitList[ix]->X;    // Coordinates.
    unit.Y        = UnitList[ix]->Y;
    unit.NumInput = UnitList[ix]->NumInput;  // Number of input units.
    unit.Type     = UnitList[ix]->Type;      // Unit type.
    unit.Binary   = UnitList[ix]->Binary;    // Binary flag.
    unit.Sigmoid  = UnitList[ix]->Sigmoid;   // Sigmoid function flag.
    unit.Bias     = UnitList[ix]->Bias;      // Bias unit flag.
    unit.BiasWgt  = UnitList[ix]->BiasWgt;   // Bias weight.

    if(fwrite(&unit,1,sizeof(NWFileUnit),handle) < sizeof(NWFileUnit))
      goto WriteErr;
    if(unit.Type != UNIT_INTERNAL)      // Input or output unit.
    {                                   // Write input/output def.
      short word = strlen(UnitList[ix]->IODef->Name) + 1;

      if(fwrite(&word,1,sizeof(short),handle) < sizeof(short))
        goto WriteErr;
      if(fwrite(UnitList[ix]->IODef->Name,1,word,handle) < word)
        goto WriteErr;
      if(fwrite(&UnitList[ix]->IODef->Min,1,sizeof(double),handle) < sizeof(double))
        goto WriteErr;
      if(fwrite(&UnitList[ix]->IODef->Max,1,sizeof(double),handle) < sizeof(double))
        goto WriteErr;
    }

    if(UnitList[ix]->NumInput > 0)      // Write out backwards connections.
    {
      if(fwrite(UnitList[ix]->InputUnits,1,UnitList[ix]->NumInput * sizeof(unsigned long),handle) < UnitList[ix]->NumInput * sizeof(unsigned long))
        goto WriteErr;
      if(fwrite(UnitList[ix]->InputWgts,1,UnitList[ix]->NumInput * sizeof(double),handle) < UnitList[ix]->NumInput * sizeof(double))
       goto WriteErr;
    }
  }

  fflush(handle);                       // Flush output.

  if(file != NULL)                      // Saved in new file, close orig.
  {
    if(Handle != NULL)
      fclose(Handle);
    realpath(file, Path);
  }
  Handle = handle;

  return(NW_SUCCESS);
}

/*****************************************************************************
  Function:   Network::CreateUnit()
  Purpose:    This function creates a processing unit in the network.
  Parameters: unsigned long x           X-coordinate of unit.
              unsigned long y           Y-coordinate of unit.
              int type                  Unit type (0=input, 1=internal,
                                        2=output).
              int binary                If TRUE, unit is binary.
              int bias                  If TRUE, unit has bias input.
              int sigmoid               If an output unit, and TRUE, unit uses
                                        sigmoid function.
              char *name                If an input or output unit, the name
                                        for the unit.
              double min                If an input or output unit, the
                                        minimum endpoint of the unit's range.
              double max                If an input or output unit, the
                                        maximum endpoint of the unit's range.
              unsigned long *index      If not NULL, used to return the index
                                        of the created unit.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::CreateUnit(unsigned long x,unsigned long y,int type,int binary,
                          int bias,int sigmoid,char *name,double min,
                          double max,unsigned long *index)
{
  void *temp;

  if(NumUnits >= UnitSpace)             // No more prealloced entries.
  {
    if((temp = realloc(UnitList,(NumUnits + 512) * sizeof(NWUnit *))) == NULL)
      return(NW_ERR_MEMORY);            // Out of memory.
    UnitList = (NWUnit **)temp;
    memset(&UnitList[NumUnits],0,512 * sizeof(NWUnit *));
    UnitSpace = NumUnits + 512;
  }

  if(type < 0 || type > 2)              // Illegal unit type.
    type = UNIT_INTERNAL;
  else if(type == UNIT_OUTPUT && !sigmoid && binary)  // Cannot be both.
    return(NW_ERR_BADPARAM);

  if((UnitList[NumUnits] = new NWUnit) == NULL)
    return(NW_ERR_MEMORY);
  memset(UnitList[NumUnits],0,sizeof(NWUnit));

  if(type != UNIT_INTERNAL)             // An input or output unit.
  {
    if((UnitList[NumUnits]->IODef = new NWIODef) == NULL)
    {
      delete UnitList[NumUnits];
      return(NW_ERR_MEMORY);
    }
    memset(UnitList[NumUnits]->IODef,0,sizeof(NWIODef));

    if((UnitList[NumUnits]->IODef->Name = new char[strlen(name) + 1]) == NULL)
    {
      delete UnitList[NumUnits]->IODef;
      delete UnitList[NumUnits];
      return(NW_ERR_MEMORY);
    }
    strcpy(UnitList[NumUnits]->IODef->Name,name);
    UnitList[NumUnits]->IODef->Min = min;
    UnitList[NumUnits]->IODef->Max = max;
  }

  if(type != UNIT_OUTPUT)               // Not an output unit.
    sigmoid = TRUE;

  UnitList[NumUnits]->X       = x;      // Unit's coordinates.
  UnitList[NumUnits]->Y       = y;
  UnitList[NumUnits]->Type    = type;     // Type of unit.
  UnitList[NumUnits]->Binary  = binary;   // Binary flag.
  UnitList[NumUnits]->Sigmoid = sigmoid;  // Sigmoid function flag.
  UnitList[NumUnits]->Bias    = bias;     // Bias flag.
  UnitList[NumUnits]->BiasWgt =           // Bias weight, from -1 to +1.
    (double)((double)Rand32() - LONG_MAX) / LONG_MAX;

  NumUnits++;                           // Have a new unit.

  if(type == UNIT_INPUT)                // New input unit.
    NumInput++;
  else if(type == UNIT_OUTPUT)          // New output unit.
    NumOutput++;

  if(index != NULL)
    *index = NumUnits - 1;

  return(NW_SUCCESS);                   // Successful operation.
}

/*****************************************************************************
  Function:   Network::DeleteUnit()
  Purpose:    This function deletes the given processing unit.
  Parameters: unsigned long unit        ID of unit to be deleted.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::DeleteUnit(unsigned long unit)
{
  unsigned long   ix,jx;

  if(unit >= NumUnits)                  // Illegal unit id.
    return(NW_ERR_BADPARAM);

  if(UnitList[unit]->Type == UNIT_INPUT)
    NumInput--;
  else if(UnitList[unit]->Type == UNIT_OUTPUT)
    NumOutput--;

// Remove all interconnections to the unit.

  for(ix = 0;ix < NumUnits;ix++)        // Remove interconn's to the unit.
  {
    if((jx = ULongSearch(unit,UnitList[ix]->NumInput,
                         UnitList[ix]->InputUnits)) != (unsigned long)(-1))
    {
      memmove(&UnitList[ix]->InputUnits[jx],
              &UnitList[ix]->InputUnits[jx + 1],
              (--UnitList[ix]->NumInput - jx));
      UnitList[ix]->InputUnits = (unsigned long *)realloc(UnitList[ix]->InputUnits,UnitList[ix]->NumInput * sizeof(unsigned long));

      memmove(&UnitList[ix]->InputWgts[jx],
              &UnitList[ix]->InputWgts[jx + 1],
              (UnitList[ix]->NumInput - jx));
      UnitList[ix]->InputWgts = (double *)realloc(UnitList[ix]->InputWgts,UnitList[ix]->NumInput * sizeof(double));
    }
  }

// Remove the unit from the units list.

  delete UnitList[unit]->InputUnits;
  delete UnitList[unit]->InputWgts;
  if(UnitList[unit]->Type != UNIT_INTERNAL)
  {
    delete[] UnitList[unit]->IODef->Name;
    delete UnitList[unit]->IODef;
  }
  delete UnitList[unit];

  memmove(&UnitList[unit],&UnitList[unit + 1],
          (--NumUnits - unit) * sizeof(NWUnit *));

// Decrement connection ids for units after the deleted one.

  for(ix = 0;ix < NumUnits;ix++)
    for(jx = 0;jx < UnitList[ix]->NumInput;jx++)
      if(UnitList[ix]->InputUnits[jx] > unit)
        UnitList[ix]->InputUnits[jx]--;

// Don't realloc the unit list -- we'll leave an extra free, prealloc'ed
//   entry for future units.

  return(NW_SUCCESS);                   // Successful operation.
}

/*****************************************************************************
  Function:   CreateConnection()
  Purpose:    This function creates a interconnection between the two given
              processing units.
  Parameters: unsigned long source      Source unit for interconnection.
              unsigned long dest        Destination unit for interconnection.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::CreateConnection(unsigned long source,unsigned long dest)
{
  void           *temp;
  unsigned long   ix;

  if(source >= NumUnits || dest >= NumUnits)  // Illegal unit ids.
    return(NW_ERR_BADPARAM);
  if(source == dest)                    // Can't connect to self.
    return(NW_ERR_CONNTOSELF);
  if(UnitList[source]->Type == UNIT_OUTPUT)  // Can't have a fwd conn.
    return(NW_ERR_IOCONN);
  if(UnitList[dest]->Type == UNIT_INPUT)     // Can't have a bwd conn.
    return(NW_ERR_IOCONN);

  for(ix = 0;ix < UnitList[dest]->NumInput;ix++)
  {
    if(UnitList[dest]->InputUnits[ix] == source)
      return(NW_ERR_CONNEXISTS);        // Connection already exists.
    else if(UnitList[dest]->InputUnits[ix] > source)
      break;
  }

  if((temp = realloc(UnitList[dest]->InputUnits,(UnitList[dest]->NumInput + 1) * sizeof(unsigned long))) == NULL)
    return(NW_ERR_MEMORY);
  UnitList[dest]->InputUnits = (unsigned long *)temp;

  if((temp = realloc(UnitList[dest]->InputWgts,(UnitList[dest]->NumInput + 1) * sizeof(double))) == NULL)
  {
    UnitList[dest]->InputUnits = (unsigned long *)realloc(UnitList[dest]->InputUnits,UnitList[dest]->NumInput * sizeof(unsigned long));
    return(NW_ERR_MEMORY);
  }
  UnitList[dest]->InputWgts = (double *)temp;

  memmove(&UnitList[dest]->InputUnits[ix + 1],
          &UnitList[dest]->InputUnits[ix],
          (UnitList[dest]->NumInput - ix) * sizeof(unsigned long));
  UnitList[dest]->InputUnits[ix] = source;

  memmove(&UnitList[dest]->InputWgts[ix + 1],
          &UnitList[dest]->InputWgts[ix],
          (UnitList[dest]->NumInput - ix) * sizeof(double));
  UnitList[dest]->InputWgts[ix] =       // Create random weight.
    (double)((double)Rand32() - LONG_MAX) / LONG_MAX;
  UnitList[dest]->NumInput++;

  return(NW_SUCCESS);                   // Succesful operation.
}

/*****************************************************************************
  Function:   DeleteConnection()
  Purpose:    This function deletes the interconnection between the two given
              processing units.
  Parameters: unsigned long source      Source unit for interconnection.
              unsigned long dest        Destination unit for interconnection.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::DeleteConnection(unsigned long source,unsigned long dest)
{
  unsigned long ix;

  if(source >= NumUnits || dest >= NumUnits)  // Illegal unit id.
    return(NW_ERR_BADPARAM);

  if((ix = ULongSearch(source,UnitList[dest]->NumInput,UnitList[dest]->InputUnits)) == (unsigned long)(-1))
    return(NW_ERR_NOTCONN);             // Units aren't connected.

  memmove(&UnitList[dest]->InputUnits[ix],
          &UnitList[dest]->InputUnits[ix + 1],
          --UnitList[dest]->NumInput - ix);
  UnitList[dest]->InputUnits = (unsigned long *)realloc(UnitList[dest]->InputUnits, UnitList[dest]->NumInput * sizeof(unsigned long));
  memmove(&UnitList[dest]->InputWgts[ix],
          &UnitList[dest]->InputWgts[ix + 1],
          UnitList[dest]->NumInput - ix);
  UnitList[dest]->InputWgts = (double *)realloc(UnitList[dest]->InputWgts, UnitList[dest]->NumInput * sizeof(double));

  return(NW_SUCCESS);                   // Successful operation.
}

/*****************************************************************************
  Function:   Network::SetupTrain()
  Purpose:    This function prepares the current network for training by
              initializing required values, allocating memory, etc.
              This function automatically calls SetupExec().
  Parameters: int accumulate            Whether to accumulate weights (store
                                        changes and apply only at end of
                                        iterations).
              int momentum              Whether to implement momentum (add
                                        small amount of previous weight
                                        change to current one).

              Note that ``accumulate and ``momentum are mutually
              exclusive; both cannot be set to TRUE.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::SetupTrain(int accumulate,int momentum)
{
  NWErr         nwErr;
  unsigned long ix,num;

  if(NumUnits == 0)                     // No units to be trained.
    return(NW_SUCCESS);

  if(accumulate && momentum)            // Can't accumulate *and* gain.
    return(NW_ERR_BADPARAM);

  if(Error != NULL)                     // Previously allocated; free it.
    delete[] Error;
  if(BackSeq != NULL)
    delete[] BackSeq;
  if(Accum != NULL)
  {
    for(ix = 0;ix < NumUnits;ix++)
      delete[] Accum[ix];
    delete[] Accum;
  }
  if(Momentum != NULL)
  {
    for(ix = 0;ix < NumUnits;ix++)
      delete[] Momentum[ix];
    delete[] Momentum;
  }

  if((Error = new double[NumUnits]) == NULL)
    return(NW_ERR_MEMORY);
  if((BackSeq = new unsigned long[NumUnits]) == NULL)
  {
    delete[] Error;
    return(NW_ERR_MEMORY);
  }

  if(accumulate)                        // Accumulate weights.
  {
    if((Accum = new double *[NumUnits]) == NULL)
    {
      delete[] Error;
      delete[] BackSeq;
      return(NW_ERR_MEMORY);
    }
    for(ix = 0;ix < NumUnits;ix++)
    {
      num = UnitList[ix]->NumInput;
      if(UnitList[ix]->Bias)            // Space for bias momentum.
        num++;

      if((Accum[ix] = new double[num]) == NULL)
      {
        delete[] Error;
        delete[] BackSeq;
        while(ix > 0)
          delete[] Accum[--ix];
        delete[] Accum;
        return(NW_ERR_MEMORY);
      }
      memset(Accum[ix],0,num * sizeof(double));
    }
  }

  else if(momentum)                     // Employ weight momentum.
  {
    if((Momentum = new double *[NumUnits]) == NULL)
    {
      delete[] Error;
      delete[] BackSeq;
      return(NW_ERR_MEMORY);
    }
    for(ix = 0;ix < NumUnits;ix++)
    {
      num = UnitList[ix]->NumInput;
      if(UnitList[ix]->Bias)            // Space for bias momentum.
        num++;

      if((Momentum[ix] = new double[num]) == NULL)
      {
        delete[] Error;
        delete[] BackSeq;
        while(ix > 0)
          delete[] Momentum[--ix];
        delete[] Momentum;
        return(NW_ERR_MEMORY);
      }
      memset(Momentum[ix],0,num * sizeof(double));
    }
  }

  if((nwErr = SetupExec()) != NW_SUCCESS) // Setup for execution.
  {
    delete[] Error;
    delete[] BackSeq;
    if(accumulate)                      // Accumulate weights.
    {
      for(ix = 0;ix < NumUnits;ix++)
        delete[] Accum[ix];
      delete[] Accum;
    }
    else if(momentum)                   // Employ weight momentum.
    {
      for(ix = 0;ix < NumUnits;ix++)
        delete[] Momentum[ix];
      delete[] Momentum;
    }

    return(nwErr);
  }

  return(NW_SUCCESS);
}

/*****************************************************************************
  Function:   Network::EndTrain()
  Purpose:    This function releases resources allocated for training.
              It automatically calls EndExec().
  Parameters: None.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::EndTrain(void)
{
  unsigned long ix;

  if(Error != NULL)
  {
    delete[] Error;
    Error = NULL;
  }
  if(BackSeq != NULL)
  {
    delete[] BackSeq;
    BackSeq = NULL;
  }

  if(Accum != NULL)                     // Free accumulated weight values.
  {
    for(ix = 0;ix < NumUnits;ix++)
      delete[] Accum[ix];
    delete[] Accum;
    Accum = NULL;
  }
  else if(Momentum != NULL)             // Free momentum weight values.
  {
    for(ix = 0;ix < NumUnits;ix++)
      delete[] Momentum[ix];
    delete[] Momentum;
    Momentum = NULL;
  }

  EndExec();                            // Release execution resources.

  return(NW_SUCCESS);
}

/*****************************************************************************
  Function:   Network::SetupExec()
  Purpose:    This function prepares the current network for execution.
  Parameters: None.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::SetupExec(void)
{
  if(NumUnits == 0)                     // No units in network.
    return(NW_SUCCESS);

  if(Sum != NULL)                       // Free sum/act. level arrays.
    delete[] Sum;
  if(ActLevel != NULL)
    delete[] ActLevel;

  if((Sum = new double[NumUnits]) == NULL)
    return(NW_ERR_MEMORY);
  memset(Sum,0,NumUnits * sizeof(double));
  if((ActLevel = new double[NumUnits]) == NULL)
  {
    delete[] Sum;
    return(NW_ERR_MEMORY);
  }
  memset(ActLevel,0,NumUnits * sizeof(double));

  return(NW_SUCCESS);                   // Successful operation.
}

/*****************************************************************************
  Function:   Network::EndExec()
  Purpose:    This function releases resources allocated for execution.
  Parameters: None.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::EndExec(void)
{
  if(Sum != NULL)                      // Free sum, act. level arrays.
  {
    delete[] Sum;
    Sum = NULL;
  }
  if(ActLevel != NULL)
  {
    delete[] ActLevel;
    ActLevel = NULL;
  }

  return(NW_SUCCESS);
}

/*****************************************************************************
  Function:   Network::SetInput()
  Purpose:    This function sets the given input entry to the specified
              value.
  Parameters: unsigned long unit        The input unit to be set.
              double value              The value to be applied.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::SetInput(unsigned long unit,double value)
{
  if(unit >= NumUnits)                  // Bad unit index.
    return(NW_ERR_BADPARAM);
  if(UnitList[unit]->Type != UNIT_INPUT)  // Not an input unit.
    return(NW_ERR_NOTINPUT);

// Truncate and scale input value.

  if(value > UnitList[unit]->IODef->Max)
    value = UnitList[unit]->IODef->Max;
  else if(value < UnitList[unit]->IODef->Min)
    value = UnitList[unit]->IODef->Min;

  value -= UnitList[unit]->IODef->Min;
  value /= UnitList[unit]->IODef->Max - UnitList[unit]->IODef->Min;

  Sum[unit] = ActLevel[unit] = value;    // Apply input value.

  return(NW_SUCCESS);
}

/*****************************************************************************
  Function:   Network::ReadOutput()
  Purpose:    This function reads the given output entry.
  Parameters: unsigned long unit        The output unit to be read.
              double *value             The variable in which to store the
                                        value.  The result will be correctly
                                        scaled to the unit's output range.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::ReadOutput(unsigned long unit,double *value)
{
  if(unit >= NumUnits)                  // Bad unit index.
    return(NW_ERR_BADPARAM);
  if(UnitList[unit]->Type != UNIT_OUTPUT) // Not an output unit.
    return(NW_ERR_NOTOUTPUT);

// Truncate and scale output value.

  *value = ActLevel[unit];

  if(!UnitList[unit]->Sigmoid)          // Does not use sigmoid fn.
    *value += 0.5;

  *value *= UnitList[unit]->IODef->Max - UnitList[unit]->IODef->Min;
  *value += UnitList[unit]->IODef->Min;

  return(NW_SUCCESS);
}

/*****************************************************************************
  Function:   Network::ApplyTarget()
  Purpose:    This function applies an output target value to the given unit,
              thus setting the unit's error value.
  Parameters: unsigned long unit        The output unit to be evaluated.
              double target             The target value (in the range of
                                        [Min, Max]) for the unit.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::ApplyTarget(unsigned long unit,double target)
{
  if(unit >= NumUnits)                  // Bad unit index.
    return(NW_ERR_BADPARAM);
  if(UnitList[unit]->Type != UNIT_OUTPUT) // Not an output unit.
    return(NW_ERR_NOTOUTPUT);

// Truncate and scale target output value.

  if(target > UnitList[unit]->IODef->Max)
    target = UnitList[unit]->IODef->Max;
  else if(target < UnitList[unit]->IODef->Min)
    target = UnitList[unit]->IODef->Min;
  target -= UnitList[unit]->IODef->Min;
  target /= UnitList[unit]->IODef->Max - UnitList[unit]->IODef->Min;

  if(!UnitList[unit]->Sigmoid)          // Doesn't use the sigmoid function.
    target -= 0.5;

  Error[unit] = target - ActLevel[unit];  // Compute error value.

  return(NW_SUCCESS);
}

/*****************************************************************************
  Function:   Network::ForwardPass()
  Purpose:    This function performs a forward pass on the current network.
              It is the program's responsibility to ensure that the input
              units' input values have been set.
  Parameters: None.
  Returns:    A NetWorks error value (0 on success).
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::ForwardPass(void)
{
  unsigned long   ix,jx;
  unsigned long   num_processed;        // Count of units done this iter.
  unsigned long   tot_processed = 0;    // Total # of units processed.

  if(NumUnits == 0)                     // No units.
    return(NW_SUCCESS);

  for(ix = 0;ix < NumUnits;ix++)        // Reset "processed" flag.
  {
    if(UnitList[ix]->Type == UNIT_INPUT)  // Input unit.
    {
      UnitList[ix]->Flag1 = TRUE;       // It has been processed.
      tot_processed++;
      if(BackSeq != NULL)               // Store back-pass sequence.
        BackSeq[ix] = NumUnits - tot_processed;
    }
    else
      UnitList[ix]->Flag1 = FALSE;
  }

// Process all units.

  for(;;)                               // Do until all units processed.
  {
    num_processed = 0;                  // None processed yet this iter.

    for(ix = 0;ix < NumUnits;ix++)      // Scan for unprocessed units.
    {
      if(!UnitList[ix]->Flag1)          // Not processed yet.
      {
        for(jx = 0;jx < UnitList[ix]->NumInput;jx++)  // Check inp. units.
          if(!UnitList[UnitList[ix]->InputUnits[jx]]->Flag1)
            break;

        if(jx < UnitList[ix]->NumInput) // Some inp. units not processed.
          continue;

        if(UnitList[ix]->Bias)          // Has bias input.
          Sum[ix] = UnitList[ix]->BiasWgt;  // Apply bias input.
        else                            // No bias input.
          Sum[ix] = 0.0;                // Zero out the sum.

        for(jx = 0;jx < UnitList[ix]->NumInput;jx++)  // Other inputs.
          Sum[ix] += ActLevel[UnitList[ix]->InputUnits[jx]] * UnitList[ix]->InputWgts[jx];

// Compute the activation level of this unit.

        if(UnitList[ix]->Binary)        // Unit is binary.
          ActLevel[ix] = Sum[ix] > 0.0 ? 1.0 : 0.0;
        else if(UnitList[ix]->Type == UNIT_OUTPUT && !UnitList[ix]->Sigmoid)
          ActLevel[ix] = (Sum[ix] > 0.5 ? 0.5 : (Sum[ix] < -0.5 ? -0.5 : Sum[ix]));
        else                            // Normal unit, use sigmoid fnc.
          ActLevel[ix] = 1.0 / (1.0 + exp(-Sum[ix]));

        UnitList[ix]->Flag1 = TRUE;     // It has been processed.
        num_processed++;                // One more activated this pass.
        tot_processed++;
        if(BackSeq != NULL)             // Store back-pass sequence.
          BackSeq[ix] = NumUnits - tot_processed;
      }
    }

    if(tot_processed == NumUnits)       // All units processed.
      break;
    else if(num_processed == 0)         // Some left, but 0 done this iter.
    {
      for(ix = 0;ix < NumUnits;ix++)    // Reset activated flags.
        UnitList[ix]->Flag1 = FALSE;
      memset(Sum,0,NumUnits * sizeof(double));
      memset(ActLevel,0,NumUnits * sizeof(double));

      return(NW_ERR_RECURSIVE);         // Net has recursive unit chain.
    }
  }

  for(ix = 0;ix < NumUnits;ix++)        // Reset activated flags.
    UnitList[ix]->Flag1 = FALSE;

  return(NW_SUCCESS);                   // Successful operation.
}

/*****************************************************************************
  Function:   Network::BackwardPass()
  Purpose:    This function performs a backward pass on the current network.
              It is the program's responsibility to ensure that the target
              output values for the output units have been applied.
  Parameters: double eta                Learning parameter.
              double momentum_coeff     Momentum coefficient.
  Returns:    NW_SUCCESS on success, or an error value on error.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::BackwardPass(double eta,double momentum_coeff)
{
  unsigned long ix,jx,idx;
  double        basic_err,change;

  if(NumUnits == 0)                     // No units.
    return(NW_SUCCESS);

  for(ix = 0;ix < NumUnits;ix++)        // Reset error values.
    if(UnitList[ix]->Type != UNIT_OUTPUT)  // Not an output unit.
      Error[ix] = 0.0;

// Propagate error values backwards.

  for(idx = 0;;)                        // Keep doing it 'til we're done.
  {
    for(ix = NumUnits - 1;;ix--)        // Scan for unprocessed units.
    {
      if(BackSeq[ix] == idx)            // Next unit to be processed.
      {
        for(jx = 0;jx < UnitList[ix]->NumInput;jx++)
        {
          Error[UnitList[ix]->InputUnits[jx]] +=
            Error[ix] * UnitList[ix]->InputWgts[jx];
        }

        idx++;                          // Increment sequence index.
      }

      if(ix == 0)                       // Just processed first unit.
        break;                          // Start again @ end.
    }

    if(idx == NumUnits)                 // All units processed.
      break;
  }

// Error values have now been propagated backwards; now we must update
//   the weights for each interconnection.

  for(ix = 0;ix < NumUnits;ix++)        // Perform for each unit.
  {
// Pre-compute the 'basic' delta-weight value for this unit.

    if(UnitList[ix]->Binary ||          // Unit is binary, or linear output.
       (UnitList[ix]->Type == UNIT_OUTPUT && !UnitList[ix]->Sigmoid))
      basic_err = eta * Error[ix];
    else                                // Normal unit, use derivative.
      basic_err = eta * Error[ix] * (ActLevel[ix] * (1 - ActLevel[ix]));

    if(UnitList[ix]->Bias)              // Update bias weight.
    {
      if(Momentum != NULL)              // Implementing weight momentum.
      {
        change = basic_err + momentum_coeff * Momentum[ix][UnitList[ix]->NumInput];
        UnitList[ix]->BiasWgt += change;
        Momentum[ix][UnitList[ix]->NumInput] = change;
      }
      else if(Accum != NULL)            // Implementing weight accumulation.
        Accum[ix][UnitList[ix]->NumInput] += basic_err;
      else                              // Normal update strategy.
        UnitList[ix]->BiasWgt += basic_err;
    }

    for(jx = 0;jx < UnitList[ix]->NumInput;jx++)  // Update each weight.
    {
      change = basic_err * ActLevel[UnitList[ix]->InputUnits[jx]];

      if(Momentum != NULL)              // Implementing weight momentum.
      {
        change += momentum_coeff * Momentum[ix][jx];
        UnitList[ix]->InputWgts[jx] += change;
        Momentum[ix][jx] = change;
      }
      else if(Accum != NULL)            // Implementing weight accumulation.
        Accum[ix][jx] += change;
      else                              // Normal update strategy.
        UnitList[ix]->InputWgts[jx] += change;
    }
  }

  return(NW_SUCCESS);                   // Successful operation.
}

/*****************************************************************************
  Function:   Network::ApplyAccum()
  Purpose:    This function applies all accumulated weight changes.
  Parameters: None.
  Returns:    A NetWorks error value (0 on success).

- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

NWErr Network::ApplyAccum(void)
{
  unsigned long ix,jx;

  if(Accum == NULL)                     // Not accumulating.
    return(NW_SUCCESS);

  for(ix = 0;ix < NumUnits;ix++)
  {
    for(jx = 0;jx < UnitList[ix]->NumInput;jx++)
    {
      UnitList[ix]->InputWgts[jx] += Accum[ix][jx];
      Accum[ix][jx] = 0;
    }

    if(UnitList[ix]->Bias)              // Apply weight change to bias wgt.
    {
      UnitList[ix]->BiasWgt += Accum[ix][UnitList[ix]->NumInput];
      Accum[ix][UnitList[ix]->NumInput] = 0;
    }
  }

  return(NW_SUCCESS);                   // Successful operation.
}

