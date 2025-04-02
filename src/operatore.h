//
// Created by Marvel  Asuenimhen  on 26/03/25.
//

#ifndef OPERATORE_H
#define OPERATORE_H

#include "config.h"
#include "sportello.h"

#define OPERATORS_SHM_KEY 1503
typedef struct  {

  int *breaks_taken;
  int *assigned_sportello;
  int *service_type;
  int n_op_on_break;
}Operatore;


void free_counter(SportelloStatus *sportello, Operatore* operatore,  int sportello_index, int operatore_index);
void take_break(Operatore *operatore, int index);
#endif //OPERATORE_H
