#include "nSysimp.h"
#include <nSystem.h>

typedef struct {
  nTask *vec;
  int size;
} *PriQueue;

PriQueue MakePriQueue();
nTask PriGet(PriQueue pq);
void PriPut(PriQueue pq, nTask t, int pri);
int PriBest(PriQueue pq);
int EmptyPriQueue(PriQueue pq);

/* ... agregue aca la implementacion del sistema de subastas ... */
#define TRUE 1
#define FALSE 0
#define SINRES 3

typedef struct subasta{
  PriQueue c; //cola prioridades
  int disponibles; //numero unidades disponibles
  int timeout; //timeout subasta
  int recaudacion;
  int ocupados;

} *nSubasta;

nSubasta nNuevaSubasta(int n, int tiempo){
  nSubasta s = nMalloc(sizeof(*s));
  s->c = MakePriQueue(n);
  s->disponibles = n;
  s->timeout = tiempo;
  s->recaudacion=0;
  s->ocupados = 0;

  //nPrintf("Cantidad unidades: %d\n", n);
  return s;
}


int nRecaudacion(nSubasta s, int *punidades){
  START_CRITICAL();

  nTask this_task = current_task;
  //Si debe esperar
  if(s->timeout>0){
    this_task->status = WAIT_TIMEOUT;
    ProgramTask(s->timeout); //tarea despierta automaticamente luego de timeout
  }
  else{
    this_task->status = WAIT;
  }
  ResumeNextReadyTask();


  for (int i=0; i<s->ocupados; i++){
    nTask task_ci = PriGet(s->c);
    task_ci->status=READY;

    PushTask(ready_queue,task_ci);

  }

  *punidades = s->disponibles - s->ocupados;
  this_task->status = READY;
  PushTask(ready_queue, this_task);


  END_CRITICAL();
  return s->recaudacion;
}


//muchas task, una por cada vez que se llama esto
int nOfrecer(nSubasta s, int precio){
  int result; //verdadero o falso
  START_CRITICAL();

  nTask t = current_task;

  t-> status = SINRES;
  //caso en que no se han pedido todas las unidades
  if(s->ocupados < s->disponibles){
    s->ocupados=s->ocupados+1;
    s->recaudacion = s->recaudacion + precio;
    t->status = TRUE;
    PriPut(s->c,t, precio);

  }
  //caso en que se han pedido mas de n unidades
  else{
    nTask t_salida = PriGet(s->c);
    if(t_salida->oferta < precio){

      s->recaudacion = s->recaudacion - t_salida->oferta; //descuento el precio que se saca
      t_salida->status = FALSE;
      t->status =TRUE;

      PriPut(s->c,t,precio);
      s->recaudacion = s->recaudacion + precio; //agrego precio oferta nueva
      //termino subasta rechazada
      // t_salida->status=READY;
      // PushTask(ready_queue, t_salida);

    }
    else{
      t->status = FALSE;
      t_salida->status=TRUE;
      PriPut(s->c, t_salida, t_salida->oferta); //vuelvo a poner la task sacada en la cola

      ////termino subasta rechazada
      // t->status=READY;
      // PushTask(ready_queue, t);

    }

  }

  // t->status=READY;
  // PushTask(ready_queue, t);

  result= t->status;

  END_CRITICAL();

  return result;
}




PriQueue MakePriQueue(int maxsize) {
  PriQueue pq= nMalloc(sizeof(*pq));
  pq->vec= nMalloc(sizeof(nTask)*(maxsize+1));
  pq->size= 0;
  return pq;
}

nTask PriGet(PriQueue pq) {
  nTask t;
  int k;
  if (pq->size==0)
    return NULL;
  t= pq->vec[0];
  pq->size--;
  for (k= 0; k<pq->size; k++)
    pq->vec[k]= pq->vec[k+1];
  return t;
}

void PriPut(PriQueue pq, nTask t, int oferta) {
  int k;
  t->oferta= oferta;
  for (k= pq->size-1; k>=0; k--) {
    nTask t= pq->vec[k];
    if (oferta > t->oferta)
      break;
    else
      pq->vec[k+1]= t;
  }
  pq->vec[k+1]= t;
  pq->size++;
}

int PriBest(PriQueue pq) {
  return pq->size==0 ? 0 : pq->vec[0]->oferta;
}

int EmptyPriQueue(PriQueue pq) {
  return pq->size==0;
}
