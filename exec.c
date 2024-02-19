// Exec - perform a single forward-pass on a network.
//
// First line of stdin is the name of the network file.
// Remaining lines of stdin are input values.
// Output is newline-separated list of output values.

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nwclass.h"

void main(void)
{
  char    buffer[1027];
  Network net;
  int     i;
  double  value;

  fgets(buffer, sizeof(buffer), stdin);
  net.Open(buffer);
  net.SetupExec();

  for(i = 0; i < net.NumInput; i++)
  {
    fgets(buffer, sizeof(buffer), stdin);

    net.SetInput(i, atof(buffer));
  }

  net.ForwardPass();

  for(i = 0; i < net.NumOutput; i++)
  {
    net.ReadOutput(net.NumUnits - net.NumOutput + i, &value);
    fprintf(stdout, "%f\n", value);
  }
}

