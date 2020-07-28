
__kernel void matrix_multiply(
    __global int *A, __global int *B, __global int *C, int AX, int BX) {
  int row = get_global_id(0);
  int col = get_global_id(1);
  int sum = 0;

  for (int k = 0; k < AX; k++)
    sum += A[row * AX + k] * B[k * BX + col];

  C[row * BX + col] = sum;
}

__kernel void matrix_multiply_float(
    __global float *A, __global float *B, __global float *C, int AX, int BX) {
  int row = get_global_id(0);
  int col = get_global_id(1);
  float sum = 0.0;

  for (int k = 0; k < AX; k++)
    sum += A[row * AX + k] * B[k * BX + col];

  C[row * BX + col] = sum;
}

__kernel void matrix_multiply_double(
    __global double *A, __global double *B, __global double *C, int AX, int BX) {
  int row = get_global_id(0);
  int col = get_global_id(1);
  double sum = 0.0;

  for (int k = 0; k < AX; k++)
    sum += A[row * AX + k] * B[k * BX + col];

  C[row * BX + col] = sum;
}



__kernel void matrix_transpose(
	__global int* A, __global int* B, int X, int Y){
	int row = get_global_id(0);
	int col = get_global_id(1);

	B[col * Y + row] = A[row * X + col];
}


//Following kernels is used to perform gaussian method of calculation of inverse matrix

/*
	
	NOTE: this kernel works properly if operated by single work-group 
	due to messed up synchronisation
	
	Please, ensure matrix has size less than 256 * 256

*/

__kernel void matrix_simplify_column( // Expects Both A and B matrices be squared and same size
	__global double* A, __global double* B, int SIZE, int COLUMN, __global int* err){
	
	int id = get_global_id(0);


	if(id == 0 && A[COLUMN * SIZE + COLUMN] == 0){ // one of the cores will search for non-null unit of column
		int i;
		for(i = COLUMN + 1; i < SIZE; i++)
			if(A[i * SIZE + COLUMN] != 0)
				break;
		
		if(i < SIZE){
			err[0] = i;
		}
		else{
			err[0] = -1; //if it fails to find one
		}
	}

	barrier(CLK_GLOBAL_MEM_FENCE); // others waiting till that core to finish

	if(err[0] == -1) //if failed -> exit
		return;

	
	//if ok -> performing a parallel swap
	if(err[0] != -2){
		int swap_row = err[0];
		

		double temp = A[COLUMN * SIZE + id];
		A[COLUMN * SIZE + id] = A[swap_row * SIZE + id];
		A[swap_row * SIZE + id] = temp;


	}

	mem_fence(CLK_GLOBAL_MEM_FENCE);
	
	if(id == 0)
		err[0] = -2;

	barrier(CLK_GLOBAL_MEM_FENCE); //waiting for all to finish swap

	//finally normalizing a row

	double mult = A[COLUMN * SIZE + COLUMN];

	mem_fence(CLK_GLOBAL_MEM_FENCE);

	A[COLUMN * SIZE + id] /= mult;
	B[COLUMN * SIZE + id] /= mult;

	
	barrier(CLK_GLOBAL_MEM_FENCE); //waiting for row to normalize

	//and performing a per-row summ


	// TODO: this part could be probably divided within local groups performing one row each, eliminating the cycle


	if(id == COLUMN)
		return;


	mult = A[id * SIZE + COLUMN];

	for(int i = 0; i < SIZE; i++){
		A[id * SIZE + i] -= mult * A[COLUMN * SIZE + i];
		B[id * SIZE + i] -= mult * B[COLUMN * SIZE + i];
	}


}