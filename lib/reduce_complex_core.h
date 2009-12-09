
#if (REDUCE_TYPE == REDUCE_KAHAN)


#define DSACC(c0, c1, a0, a1) dsadd((c0), (c1), (c0), (c1), (a0), (a1))
#define ZCACC(c0, c1, a0, a1) zcadd((c0), (c1), (c0), (c1), (a0), (a1))

__global__ void REDUCE_FUNC_NAME(Kernel) (REDUCE_TYPES, QudaSumComplex *g_odata, unsigned int n) {
  unsigned int tid = threadIdx.x;
  unsigned int i = blockIdx.x*(reduce_threads) + threadIdx.x;
  unsigned int gridSize = reduce_threads*gridDim.x;
  
  QudaSumFloat acc0 = 0;
  QudaSumFloat acc1 = 0;
  QudaSumFloat acc2 = 0;
  QudaSumFloat acc3 = 0;
  
  while (i < n) {
    REDUCE_REAL_AUXILIARY(i);
    REDUCE_IMAG_AUXILIARY(i);
    DSACC(acc0, acc1, REDUCE_REAL_OPERATION(i), 0);
    DSACC(acc2, acc3, REDUCE_IMAG_OPERATION(i), 0);
    i += gridSize;
  }
  
  extern __shared__ QudaSumComplex cdata[];
  QudaSumComplex *s = cdata + 2*tid;
  s[0].x = acc0;
  s[1].x = acc1;
  s[0].y = acc2;
  s[1].y = acc3;
  
  __syncthreads();
  
  if (reduce_threads >= 1024) { if (tid < 512) { ZCACC(s[0],s[1],s[1024+0],s[1024+1]); } __syncthreads(); }
  if (reduce_threads >= 512) { if (tid < 256) { ZCACC(s[0],s[1],s[512+0],s[512+1]); } __syncthreads(); }    
  if (reduce_threads >= 256) { if (tid < 128) { ZCACC(s[0],s[1],s[256+0],s[256+1]); } __syncthreads(); }
  if (reduce_threads >= 128) { if (tid <  64) { ZCACC(s[0],s[1],s[128+0],s[128+1]); } __syncthreads(); }    
  if (tid < 32) {
    if (reduce_threads >=  64) { ZCACC(s[0],s[1],s[64+0],s[64+1]); }
    if (reduce_threads >=  32) { ZCACC(s[0],s[1],s[32+0],s[32+1]); }
    if (reduce_threads >=  16) { ZCACC(s[0],s[1],s[16+0],s[16+1]); }
    if (reduce_threads >=   8) { ZCACC(s[0],s[1], s[8+0], s[8+1]); }
    if (reduce_threads >=   4) { ZCACC(s[0],s[1], s[4+0], s[4+1]); }
    if (reduce_threads >=   2) { ZCACC(s[0],s[1], s[2+0], s[2+1]); }
  }
  
  // write result for this block to global mem as single QudaSumComplex
  if (tid == 0) {
    g_odata[blockIdx.x].x = cdata[0].x+cdata[1].x;
    g_odata[blockIdx.x].y = cdata[0].y+cdata[1].y;
  }
}

#else

__global__ void REDUCE_FUNC_NAME(Kernel) (REDUCE_TYPES, QudaSumComplex *g_odata, unsigned int n) {
  unsigned int tid = threadIdx.x;
  unsigned int i = blockIdx.x*reduce_threads + threadIdx.x;
  unsigned int gridSize = reduce_threads*gridDim.x;
  
  extern __shared__ QudaSumComplex cdata[];
  QudaSumComplex *s = cdata + tid;
  s[0].x = 0;
  s[0].y = 0;
  
  while (i < n) {
    REDUCE_REAL_AUXILIARY(i);
    REDUCE_IMAG_AUXILIARY(i);
    s[0].x += REDUCE_REAL_OPERATION(i);
    s[0].y += REDUCE_IMAG_OPERATION(i);
    i += gridSize;
  }
  __syncthreads();
  
  // do reduction in shared mem
  if (reduce_threads >= 1024) { if (tid < 512) { s[0].x += s[512].x; s[0].y += s[512].y; } __syncthreads(); }
  if (reduce_threads >= 512) { if (tid < 256) { s[0].x += s[256].x; s[0].y += s[256].y; } __syncthreads(); }
  if (reduce_threads >= 256) { if (tid < 128) { s[0].x += s[128].x; s[0].y += s[128].y; } __syncthreads(); }
  if (reduce_threads >= 128) { if (tid <  64) { s[0].x += s[ 64].x; s[0].y += s[ 64].y; } __syncthreads(); }
  
  if (tid < 32) {
    if (reduce_threads >=  64) { s[0].x += s[32].x; s[0].y += s[32].y; }
    if (reduce_threads >=  32) { s[0].x += s[16].x; s[0].y += s[16].y; }
    if (reduce_threads >=  16) { s[0].x += s[ 8].x; s[0].y += s[ 8].y; }
    if (reduce_threads >=   8) { s[0].x += s[ 4].x; s[0].y += s[ 4].y; }
    if (reduce_threads >=   4) { s[0].x += s[ 2].x; s[0].y += s[ 2].y; }
    if (reduce_threads >=   2) { s[0].x += s[ 1].x; s[0].y += s[ 1].y; }
  }
  
  // write result for this block to global mem 
  if (tid == 0) {
    g_odata[blockIdx.x].x = s[0].x;
    g_odata[blockIdx.x].y = s[0].y;
  }
}

#endif

template <typename Float, typename Float2>
cuDoubleComplex REDUCE_FUNC_NAME(Cuda) (REDUCE_TYPES, int n, int kernel, QudaPrecision precision) {

  setBlock(kernel, n, precision);
  
  if (n % blasBlock.x != 0) {
    printf("ERROR reduce_complex(): length %d must be a multiple of %d\n", n, blasBlock.x);
    exit(-1);
  }
  
  if (blasBlock.x > REDUCE_MAX_BLOCKS) {
    printf("ERROR reduce_complex: block size greater then maximum permitted\n");
    exit(-1);
  }
  
#if (REDUCE_TYPE == REDUCE_KAHAN)
  int smemSize = blasBlock.x * 2 * sizeof(QudaSumComplex);
#else
  int smemSize = blasBlock.x * sizeof(QudaSumComplex);
#endif

  if (blasBlock.x == 64) {
    REDUCE_FUNC_NAME(Kernel)<64><<< blasGrid, blasBlock, smemSize >>>(REDUCE_PARAMS, d_reduceComplex, n);
  } else if (blasBlock.x == 128) {
    REDUCE_FUNC_NAME(Kernel)<128><<< blasGrid, blasBlock, smemSize >>>(REDUCE_PARAMS, d_reduceComplex, n);
  } else if (blasBlock.x == 256) {
    REDUCE_FUNC_NAME(Kernel)<256><<< blasGrid, blasBlock, smemSize >>>(REDUCE_PARAMS, d_reduceComplex, n);
  } else if (blasBlock.x == 512) {
    REDUCE_FUNC_NAME(Kernel)<512><<< blasGrid, blasBlock, smemSize >>>(REDUCE_PARAMS, d_reduceComplex, n);
  } else if (blasBlock.x == 1024) {
    REDUCE_FUNC_NAME(Kernel)<1024><<< blasGrid, blasBlock, smemSize >>>(REDUCE_PARAMS, d_reduceComplex, n);
  } else {
    printf("Reduction not implemented for %d threads\n", blasBlock.x);
    exit(-1);
  }

  // copy result from device to host, and perform final reduction on CPU
  cudaError_t error = cudaMemcpy(h_reduceComplex, d_reduceComplex, blasGrid.x*sizeof(QudaSumComplex), cudaMemcpyDeviceToHost);
  if (error != cudaSuccess) {
    printf("Error: %s\n", cudaGetErrorString(error));
    exit(-1);
  }
  
  cuDoubleComplex gpu_result;
  gpu_result.x = 0;
  gpu_result.y = 0;
  for (int i = 0; i < blasGrid.x; i++) {
    gpu_result.x += h_reduceComplex[i].x;
    gpu_result.y += h_reduceComplex[i].y;
  }
  
  return gpu_result;
}

