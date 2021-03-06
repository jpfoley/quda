#ifndef _FAT_FORCE_QUDA_H
#define _FAT_FORCE_QUDA_H

#include <quda_internal.h>
#include <quda.h>

#ifdef __cplusplus
extern "C" {
#endif

  void loadLinkToGPU(FullGauge cudaGauge, void **cpuGauge, QudaGaugeParam* param);
  void loadLinkToGPU_gf(FullGauge cudaGauge, void *cpuGauge, QudaGaugeParam* param);
  void storeLinkToCPU(void* cpuGauge, FullGauge *cudaGauge, QudaGaugeParam* param);
  void createLinkQuda(FullGauge* cudaGauge, QudaGaugeParam* param);
  void createStapleQuda(FullStaple* cudaStaple, QudaGaugeParam* param);
  void freeStapleQuda(FullStaple* cudaStaple);
  /*  void createMomQuda(FullMom* cudaMom, QudaGaugeParam* param);
  void freeMomQuda(FullMom *cudaMom);
  void storeMomToCPU(void* mom, FullMom cudaMom, QudaGaugeParam* param);
  void loadMomToGPU(FullMom cudaMom, void* mom, QudaGaugeParam* param);*/
  void packGhostStaple(FullStaple* cudaStaple, int dir, int whichway, void** fwd_nbr_buf_gpu, void** back_nbr_buf_gpu, 
		       void** fwd_nbr_buf, void** back_nbr_buf, cudaStream_t* stream);
  void  unpackGhostStaple(FullStaple* cudaStaple, int dir, int whichway, void** fwd_nbr_buf, void** back_nbr_buf, 
			  cudaStream_t* stream);
  void pack_ghost_all_staples_cpu(void *staple, void **cpuGhostStapleBack, void** cpuGhostStapleFwd, int nFace, QudaPrecision precision);
  void pack_ghost_all_links(void **cpuLink, void **cpuGhostBack, void** cpuGhostFwd, int dir, int nFace, QudaPrecision precision);
  void pack_gauge_diag(void* buf, int* X, void** sitelink, int nu, int mu, int dir1, int dir2, QudaPrecision prec);
#define freeLinkQuda freeGaugeField

#define momSiteSize   10 // real numbers per momentum
#define gaugeSiteSize 18 // real numbers per gauge field
  
#ifdef __cplusplus
}
#endif

#endif // _GAUGE_QUDA_H
