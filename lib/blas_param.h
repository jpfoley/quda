//
// Auto-tuned blas CUDA parameters, generated by blas_test
//

static int blas_threads[30][3] = {
  { 256,  288,  512},  // Kernel  0: copyCuda (high source precision)
  { 576,   32,  640},  // Kernel  1: copyCuda (low source precision)
  { 544,   96,  128},  // Kernel  2: axpbyCuda
  { 576,   96,  128},  // Kernel  3: xpyCuda
  { 576,   96,  128},  // Kernel  4: axpyCuda
  { 544,   96,  128},  // Kernel  5: xpayCuda
  { 576,   96,  128},  // Kernel  6: mxpyCuda
  {  96,  192,  192},  // Kernel  7: axCuda
  { 512,  128,   96},  // Kernel  8: caxpyCuda
  { 576,  128,   96},  // Kernel  9: caxpbyCuda
  { 512,   96,   96},  // Kernel 10: cxpaypbzCuda
  { 512,   96,   64},  // Kernel 11: axpyBzpcxCuda
  { 512,   96,   64},  // Kernel 12: axpyZpbxCuda
  { 512,  128,   96},  // Kernel 13: caxpbypzYmbwCuda
  { 256,  256,  128},  // Kernel 14: normCuda
  { 128,  256,  256},  // Kernel 15: reDotProductCuda
  { 512, 1024,  512},  // Kernel 16: axpyNormCuda
  { 512, 1024,  512},  // Kernel 17: xmyNormCuda
  { 128,  256,  256},  // Kernel 18: cDotProductCuda
  { 512,  128,  512},  // Kernel 19: xpaycDotzyCuda
  { 128,  128,  128},  // Kernel 20: cDotProductNormACuda
  { 128,  128,  128},  // Kernel 21: cDotProductNormBCuda
  { 512,  512,  512},  // Kernel 22: caxpbypzYmbwcDotProductWYNormYCuda
  {  64,  128,  128},  // Kernel 23: cabxpyAxCuda
  { 512,  256,  256},  // Kernel 24: caxpyNormCuda
  { 512,  256,  256},  // Kernel 25: caxpyXmazNormXCuda
  { 512,  512,  256},  // Kernel 26: cabxpyAxNormCuda
  { 512,  128,  128},  // Kernel 27: caxpbypzCuda
  { 512,  128,   64},  // Kernel 28: caxpbypczpwCuda
  { 512,  512,  256}   // Kernel 29: caxpyDotzyCuda
};

static int blas_blocks[30][3] = {
  {32768,  8192,   256},  // Kernel  0: copyCuda (high source precision)
  {16384, 32768,   256},  // Kernel  1: copyCuda (low source precision)
  {  256, 16384, 32768},  // Kernel  2: axpbyCuda
  {  256, 16384, 32768},  // Kernel  3: xpyCuda
  {  256, 16384, 32768},  // Kernel  4: axpyCuda
  {  256, 16384, 32768},  // Kernel  5: xpayCuda
  {  256, 16384, 65536},  // Kernel  6: mxpyCuda
  { 2048,  8192, 32768},  // Kernel  7: axCuda
  {  256, 16384, 65536},  // Kernel  8: caxpyCuda
  {  256, 65536, 32768},  // Kernel  9: caxpbyCuda
  {  256, 32768, 65536},  // Kernel 10: cxpaypbzCuda
  {  256, 16384, 32768},  // Kernel 11: axpyBzpcxCuda
  {  256, 16384, 32768},  // Kernel 12: axpyZpbxCuda
  {  512, 32768, 16384},  // Kernel 13: caxpbypzYmbwCuda
  {   32,    64,   512},  // Kernel 14: normCuda
  {   64,   512,  2048},  // Kernel 15: reDotProductCuda
  { 4096,    64, 16384},  // Kernel 16: axpyNormCuda
  {32768,    64, 16384},  // Kernel 17: xmyNormCuda
  {   64,   256,    32},  // Kernel 18: cDotProductCuda
  {  128,    64, 16384},  // Kernel 19: xpaycDotzyCuda
  {   64,   128,    64},  // Kernel 20: cDotProductNormACuda
  {   64,   512,    64},  // Kernel 21: cDotProductNormBCuda
  {  128,   512,   256},  // Kernel 22: caxpbypzYmbwcDotProductWYNormYCuda
  { 2048, 32768, 32768},  // Kernel 23: cabxpyAxCuda
  { 1024,  2048, 16384},  // Kernel 24: caxpyNormCuda
  {16384, 16384, 16384},  // Kernel 25: caxpyXmazNormXCuda
  { 8192,  1024, 65536},  // Kernel 26: cabxpyAxNormCuda
  {  256, 16384, 65536},  // Kernel 27: caxpbypzCuda
  {  256, 65536, 32768},  // Kernel 28: caxpbypczpwCuda
  {  128,   256,  2048}   // Kernel 29: caxpyDotzyCuda
};
