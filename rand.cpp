/*****************************************************************************
  File:     rand.cpp

  This file is Copyright (C) 1996 by Scott C. Moonen.  All Rights Reserved.

  Purpose:  This file contains a 32-bit random number generation system.

  This random number generator uses a Fibonacci-like (actually,
  Deka-Hepta-bonacci :-) ) sequence of numbers to generate a random number.

  The basic idea was presented in

    The Encyclopedia of Computer Science and Engineering, 2nd ed.
    Ralston, Anthony, editor.  New York: Van Nostrand Reinhold, 1983.

  Basically, given a number sequence

    x0, x1, x2, x3, ... xn

  the following equation is used to compute xn:

    xn = (x(n - 5) + x(n - 17)) modulo 2^(variable size in bits).

  This system uses a variable size of 32 bits.  The computer automatically
  performs the modulo operation (any overflow bits are discarded).

  The last 17 numbers are seeded with a number (typically the current time,
  of the type time_t, which is already 32 bits), which uses a semi-
  congruential sequence (incrementing the result each time).

  This algorithm is believed to have absolutely NO statistical patterns.

  The period of this generator is as follows (n specifies the number of bits
  in the random number variable):

    2^(n - 1) * (2^17 - 1)

  For 32-bit random number variables, this evaluates to:

    281,472,829,227,008

  which is approximately 2.8 x 10^14.  Amazing!
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static  unsigned long   RandNums[17];   /* Numbers for randomization.       */
static  int             RandIx1 = 16;   /* Index 1 into random number array.*/
static  int             RandIx2 = 4;    /* Index 2 into random number array.*/

/*****************************************************************************
  Function:   Randomize32()
  Purpose:    This function initializes the random number generator with the
              given seed value.
  Parameters: unsigned long seed        Value to be used to initialize the
                                        sequence.
  Returns:    Nothing.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void Randomize32(unsigned long seed)
{
  int ix;

// Square the seed.  This ensures that, if the seed is a slowly-changing
//   time value (in seconds), seeds that are close together will generate
//   vastly different results.

  seed *= seed;

// For this randomization algorithm to work, we need some of the initial
//   numbers to be odd.  The algorithm below will fill the initial list
//   evenly with even and odd numbers.

  for(ix = 0;ix < 17;ix++)
  {
    RandNums[ix] = seed;
    seed *= 4226497;                    // Semi-randomize it.
    seed++;                             // Increment it.
  }

// However, an alternating distribution will NOT work quite correctly.
//   Therefore, we will ensure that the first 2 and last 2 have the same
//   parity, and that the middle 3 also have an identical parity, opposite
//   from the other group.

  RandNums[1]++;
  RandNums[8]++;
  RandNums[15]++;

// Reset the random number indices.

  RandIx1 = 16;
  RandIx2 = 4;
}

/*****************************************************************************
  Function:   Rand32()
  Purpose:    This function generates a random number.
  Parameters: None.
  Returns:    The random number, in the range of 0 - 2^32.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

unsigned long Rand32(void)
{
  unsigned long result;

  result = RandNums[RandIx1] + RandNums[RandIx2];
  RandNums[RandIx1] = result;
  if((--RandIx1) < 0)
    RandIx1 = 16;
  if((--RandIx2) < 0)
    RandIx2 = 16;

  return(result);
}

