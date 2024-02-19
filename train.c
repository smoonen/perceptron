// Training driver for neural network.
//
// The first line of stdin specifies the network file to load.
// The second line of stdin specifies the number of training iterations.
// The remaining lines of stdin contain the following, whitespace-separated:
//   Learning coefficient for this datum.
//   Data for input units, in order of their definition.
//   Target data for output units, in order of their definition.

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "nwclass.h"

void main(void)
{
  char      buffer[1027];
  char      filename[1027];
  char     *ptr;
  int       iter_cnt, data_cnt = 0, i, j, k, l;
  int      *touched = NULL;
  double   *eta = NULL;
  double  **input = NULL;
  double  **output = NULL;
  double    rms;
  Network   net;

  setpriority(PRIO_PROCESS, 0, 2);

  Randomize32(time(NULL));

  fgets(filename, sizeof(filename), stdin);
  *strchr(filename, '\n') = '\0';
  net.Open(filename);

  fgets(buffer, sizeof(buffer), stdin);
  iter_cnt = atoi(buffer);

  while(fgets(buffer, sizeof(buffer), stdin))
  {
    touched = (int *)realloc(touched, sizeof(int) * (data_cnt + 1));
    eta = (double *)realloc(eta, sizeof(double) * (data_cnt + 1));
    input = (double **)realloc(input, sizeof(double *) * (data_cnt + 1));
    output = (double **)realloc(output, sizeof(double *) * (data_cnt + 1));
    input[data_cnt] = (double *)malloc(net.NumInput * sizeof(double));
    output[data_cnt] = (double *)malloc(net.NumOutput * sizeof(double));

    if((ptr = strtok(buffer, " \t\n")) == NULL)
      { fprintf(stderr, "Badly formatted input.\n"); exit(1); }
    eta[data_cnt] = atof(ptr);
    touched[data_cnt] = FALSE;

    for(i = 0; i < net.NumInput; i++)
    {
      if((ptr = strtok(NULL, " \t\n")) == NULL)
        { fprintf(stderr, "Badly formatted input.\n"); exit(1); }
      input[data_cnt][i] = atof(ptr);
    }

    for(i = 0; i < net.NumOutput; i++)
    {
      if((ptr = strtok(NULL, " \t\n")) == NULL)
        { fprintf(stderr, "Badly formatted input.\n"); exit(1); }
      output[data_cnt][i] = atof(ptr);
    }

    data_cnt++;
  }

  net.SetupTrain(FALSE, FALSE);

  for(i = 0; i < iter_cnt; i++)
  {
    rms = 0;

    for(j = 0; j < data_cnt; j++)
    {
      k = Rand32() % (data_cnt - j);
      for(l = 0; k >= 0; k--)
      {
        while(touched[l])
          l++;
      }

      touched[l] = TRUE;

      for(k = 0; k < net.NumInput; k++)
        net.SetInput(k, input[l][k]);
      net.ForwardPass();

      for(k = 0; k < net.NumOutput; k++)
      {
        net.ApplyTarget(net.NumUnits - net.NumOutput + k, output[l][k]);
        rms += net.Error[net.NumUnits - net.NumOutput + k]
               * net.Error[net.NumUnits - net.NumOutput + k];
      }
      net.BackwardPass(eta[l], 0);
    }

    memset(touched, 0, data_cnt * sizeof(int));

    if(i % 100 == 99)
    {
      fprintf(stdout, "RMS(%i): %f\n",i,sqrt(rms / (net.NumOutput * data_cnt)));
      net.Save(filename);
    }
  }

  rms /= net.NumOutput * data_cnt;
  rms = sqrt(rms);
  fprintf(stdout, "RMS: %f\n", rms);

  net.EndTrain();

  net.Save(filename);
}

