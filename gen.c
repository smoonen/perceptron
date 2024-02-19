// Gen - generate a network file according to a description.
//
// The first line of stdin specifies the filename.
// The second line of stdin specifies the number of rows.
// The remaining lines specify the number of elements in each row.
//
// The network is generated as a fully interconnected network.

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "nwclass.h"

void main(void)
{
  char  buffer[1027];
  char  filename[1027];
  int   num_rows, i, j, k;
  int  *row_cnt;
  long unsigned int **elem_id;
  Network net;

  Randomize32(time(NULL));

  fgets(filename, sizeof(filename), stdin);
  *strchr(filename, '\n') = '\0';

  fgets(buffer, sizeof(buffer), stdin);
  num_rows = atoi(buffer);

  row_cnt = (int *)malloc(num_rows * sizeof(int));
  elem_id = (long unsigned int **)malloc(num_rows * sizeof(int));

  for(i = 0; i < num_rows; i++)
  {
    fgets(buffer, sizeof(buffer), stdin);
    row_cnt[i] = atoi(buffer);

    elem_id[i] = (long unsigned int *)malloc(row_cnt[i] * sizeof(int));
  }

  // Create input units.

  for(i = 0; i < row_cnt[0]; i++)
    net.CreateUnit(0, 0, UNIT_INPUT, 0, 1, 1, "in", 0, 1, &elem_id[0][i]);

  // Create internal units.

  for(i = 1; i < num_rows - 1; i++)
    for(j = 0; j < row_cnt[i]; j++)
      net.CreateUnit(0, 0, UNIT_INTERNAL, 0, 1, 1, "md", 0, 1, &elem_id[i][j]);

  // Create output units.

  for(i = 0; i < row_cnt[num_rows - 1]; i++)
    net.CreateUnit(0, 0, UNIT_OUTPUT, 0, 1, 1, "out", 0, 1,
                   &elem_id[num_rows - 1][i]);

  // Connect all units.

  for(i = 0; i < num_rows - 1; i++)
    for(j = 0; j < row_cnt[i]; j++)
      for(k = 0; k < row_cnt[i + 1]; k++)
        net.CreateConnection(elem_id[i][j], elem_id[i + 1][k]);

  net.Save(filename);
}

