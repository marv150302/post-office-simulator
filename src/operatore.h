//
// Created by Marvel  Asuenimhen  on 26/03/25.
//

#ifndef OPERATORE_H
#define OPERATORE_H

#include "config.h"

#define OPERATORS_SHM_KEY 1503
typedef struct  {

  int *breaks_taken;
  int *assigned_sportello;
  int current_day;
}Operatore;


void take_break(Operatore *operatore, int index);
#endif //OPERATORE_H
