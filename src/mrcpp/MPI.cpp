#include <mpi.h>
#include <iostream>
#include "MPI.h"
#include "TelePrompter.h"
#include "Timer.h"

using namespace std;

int MPI_rank = 0;
int MPI_size = 1;

/** Send or receive a serial tree using MPI
 */
//template<int D>
void MPI_Initializations(){

  MPI_Comm_rank(MPI_COMM_WORLD, &MPI_rank);
  MPI_Comm_size(MPI_COMM_WORLD, &MPI_size);

}

void SendRcv_SerialTree(SerialTree<1>* STree, int source, int dest, int tag, MPI_Comm comm){;}
void SendRcv_SerialTree(SerialTree<2>* STree, int source, int dest, int tag, MPI_Comm comm){;}
void SendRcv_SerialTree(SerialTree<3>* STree, int source, int dest, int tag, MPI_Comm comm){
  MPI_Status status;
  Timer timer;


  timer.start();
  cout<<MPI_rank<<" STree  at "<<STree<<endl;
  if(MPI_rank==source){
    //size to send is adress of first unused NodesCoeff minus start of array
    int count = STree->CoeffStack[STree->nNodesCoeff+1] - STree->firstNode;
    cout<<MPI_rank<<" sending "<<count<<" doubles to "<<dest<<endl;
    MPI_Send(STree->firstNode, count, MPI_DOUBLE, dest, tag, comm);
    timer.stop();
    cout<<" time send     " << timer<<endl;
  }
  if(MPI_rank==dest){
    int count =STree->firstNodeCoeff+STree->maxNodesCoeff*STree->sizeNodeCoeff - STree->firstNode;//max size available
    cout<<MPI_rank<<" max size receivable (number of doubles)"<<count<<endl;
    MPI_Recv(STree->firstNode, count, MPI_DOUBLE, MPI_ANY_SOURCE, tag, comm, &status);
    cout<<MPI_rank<<" received serial tree with "<<STree->nNodes<<" nodes"<<endl;
    timer.stop();
    cout<<" time receive  " << timer<<endl;
  }
    timer.start();
   cout<<MPI_rank<<" allocator after send pointer at "<<STree<<endl;
   STree->RewritePointers();
    timer.stop();
   cout<<MPI_rank<<" allocator after rewrite pointer at "<<STree<<endl;
    cout<<" time rewrite pointers  " << timer<<endl;
}

/** Define all the different MPI groups
 */
void define_groups(){

  int MPI_worldsize;
  MPI_Comm_size(MPI_COMM_WORLD, &MPI_worldsize);
  int MPI_orbital_group_size = MPI_worldsize;//temporary definition
  println(0,"MPI world size: "<< MPI_worldsize ); 
  int MPI_worldrank;
  MPI_Comm_rank(MPI_COMM_WORLD, &MPI_worldrank);
  int i = (MPI_worldrank) % MPI_orbital_group_size;

  //divide the world into groups
  //NB: each group has its own group communicator definition!
  MPI_Comm MPI_Comm_Orbital;
  MPI_Comm_split(MPI_COMM_WORLD, 0, i, &MPI_Comm_Orbital);//0 is color (same color->in same group), i is key (orders rank in the group)
  MPI_Comm_size(MPI_COMM_WORLD, &MPI_worldsize);
  MPI_Comm_size(MPI_COMM_WORLD, &MPI_orbital_group_size);
  println(0,"orbital group size: "<< MPI_orbital_group_size); 
  
}

/** Define MPI groups that share the same compute-node i.e. the same memory and array with shared memory between
 * sh_size is size in Bytes
 * returns a pointer to the shared memory
void Share_memory(MPI_Comm ncomm, MPI_Comm ncomm_sh, int sh_size, double * d_ptr){

  //split ncomm into groups that can share memory
  MPI_Comm_split_type(ncomm, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &ncomm_sh);

  int shrank;
  MPI_Comm_rank(ncomm_sh, &shrank);
  MPI_Aint size = (shrank==0) ? sh_size : 0;//rank 0 defines length of segment 
  MPI_Win win = MPI_WIN_NULL;
  int disp_unit = 16;//in order for the compiler to keep aligned. 
  //sh_size is in bytes
  MPI_Win_allocate_shared(sh_size, disp_unit, MPI_INFO_NULL, ncomm_sh, &d_ptr, &win);

  MPI_Win_fence(0, win);//wait until finished

  MPI_Aint qsize = 0;
  int qdisp = 0;
  int * qbase = NULL;
  MPI_Win_shared_query(win, 0, &qsize, &qdisp, &d_ptr);
  printf("me = %d, allocated: size=%zu, bytes at = %p\n", shrank, qsize, d_ptr);

  qbase[shrank]=shrank;
  MPI_Barrier(ncomm);
  for (int i=0; i<sh_size; i++){
    printf("shared: me=%d, i=%d, val=%d\n", shrank, i, qbase[i]);
  }
  //MPI_Win_free(&win);//to include at the end of the program
  //MPI_Comm_free(&ncomm_sh);//to include at the end of the program?

} */
