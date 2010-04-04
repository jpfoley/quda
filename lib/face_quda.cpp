#include <quda_internal.h>
#include <face_quda.h>
#include <cstdio>
#include <cstdlib>
#include <quda.h>
#include <string.h>

#define QMP_COMMS
#ifdef QMP_COMMS
#include <qmp.h>

// This is now just for the gauge face.
QMP_msgmem_t mm_gauge_send_fwd;
QMP_msgmem_t mm_gauge_from_back;
QMP_msghandle_t mh_gauge_send_fwd;
QMP_msghandle_t mh_gauge_from_back;
#endif

using namespace std;


cudaStream_t *stream;

// set these both = 0 for no overlap of qmp and cudamemcpyasync
// sendBackIdx = 0, and sendFwdIdx = 1 for overlap
int sendBackIdx = 0;
int sendFwdIdx = 1;
int recFwdIdx = sendBackIdx;
int recBackIdx = sendFwdIdx;

FaceBuffer allocateFaceBuffer(int Vs, int V, int stride, Precision precision)
{
   FaceBuffer ret;
   ret.Vs = Vs;
   ret.V = V;
   ret.stride=stride;
   ret.precision=precision;
  
   // Buffers hold half spinors
   ret.nbytes = ret.Vs*12*precision;

   // add extra space for the norms for half precision
   if (precision == QUDA_HALF_PRECISION) {
     ret.nbytes += ret.Vs*sizeof(float);
   }

#ifndef __DEVICE_EMULATION__
   cudaMallocHost(&(ret.my_fwd_face), ret.nbytes);
#else
   ret.my_fwd_face = malloc(ret.nbytes);
#endif

   if( !ret.my_fwd_face ) { 
     errorQuda("Unable to allocate my_fwd_face");
   }
   
#ifndef __DEVICE_EMULATION__
   cudaMallocHost(&(ret.my_back_face), ret.nbytes);
#else
   ret.my_back_face = malloc(ret.nbytes);
#endif

   if( !ret.my_back_face ) { 
     errorQuda("Unable to allocate my_back_face");
   }
   
#ifndef __DEVICE_EMULATION__
   cudaMallocHost(&(ret.from_fwd_face), ret.nbytes);
#else
   ret.from_fwd_face = malloc(ret.nbytes);
#endif
   
   if( !ret.from_fwd_face ) { 
     errorQuda("Unable to allocate from_fwd_face");
   }
   
   
#ifndef __DEVICE_EMULATION__
   cudaMallocHost(&(ret.from_back_face), ret.nbytes);
#else
   ret.from_back_face = malloc(ret.nbytes);
#endif
   

   if( !ret.from_back_face ) { 
     errorQuda("Unable to allocate from_back_face");
   }
   
   
#ifdef QMP_COMMS
   ret.mm_send_fwd = QMP_declare_msgmem(ret.my_fwd_face, ret.nbytes);
   if( ret.mm_send_fwd == NULL ) { 
     errorQuda("Unable to allocate send fwd message mem");
   }
   ret.mm_send_back = QMP_declare_msgmem(ret.my_back_face, ret.nbytes);
   if( ret.mm_send_back == NULL ) { 
     errorQuda("Unable to allocate send back message mem");
   }
   

   ret.mm_from_fwd = QMP_declare_msgmem(ret.from_fwd_face, ret.nbytes);
   if( ret.mm_from_fwd == NULL ) { 
     errorQuda("Unable to allocate recv from fwd message mem");
   }
   
   ret.mm_from_back = QMP_declare_msgmem(ret.from_back_face, ret.nbytes);
   if( ret.mm_from_back == NULL ) { 
     errorQuda("Unable to allocate recv from back message mem");
   }
   
   ret.mh_send_fwd = QMP_declare_send_relative(ret.mm_send_fwd,
					       3,
					       +1, 
					       0);
   if( ret.mh_send_fwd == NULL ) {
     errorQuda("Unable to allocate forward send");
   }
   
    ret.mh_send_back = QMP_declare_send_relative(ret.mm_send_back, 
						 3,
						 -1,
						 0);
    if( ret.mh_send_back == NULL ) {
      errorQuda("Unable to allocate backward send");
    }
    
    
    ret.mh_from_fwd = QMP_declare_receive_relative(ret.mm_from_fwd,
						   3,
						   +1,
						   0);
    if( ret.mh_from_fwd == NULL ) {
      errorQuda("Unable to allocate forward recv");
    }
    
    ret.mh_from_back = QMP_declare_receive_relative(ret.mm_from_back, 
					     3,
					     -1,
					     0);
    if( ret.mh_from_back == NULL ) {
      errorQuda("Unable to allocate backward recv");
    }

#endif

    return ret;
}


void freeFaceBuffer(FaceBuffer f)
{

#ifdef QMP_COMMS
  QMP_free_msghandle(f.mh_send_fwd);
  QMP_free_msghandle(f.mh_send_back);
  QMP_free_msghandle(f.mh_from_fwd);
  QMP_free_msghandle(f.mh_from_back);
  QMP_free_msgmem(f.mm_send_fwd);
  QMP_free_msgmem(f.mm_send_back);
  QMP_free_msgmem(f.mm_from_fwd);
  QMP_free_msgmem(f.mm_from_back);
#else

#ifndef __DEVICE_EMULATION__
  cudaFreeHost(f.my_fwd_face);
  cudaFreeHost(f.my_back_face);
  cudaFreeHost(f.from_fwd_face);
  cudaFreeHost(f.from_back_face);
#else 
  free(f.my_fwd_face);
  free(f.my_back_face);
  free(f.from_fwd_face);
  free(f.from_back_face);
#endif
#endif
  f.my_fwd_face=NULL;
  f.my_back_face=NULL;
  f.from_fwd_face=NULL;
  f.from_back_face=NULL;

}

// This would need to change for going truly parallel..
// Right now, its just a question of copying faces. My front face
// is what I send forward so it will be my 'from-back' face
// and my back face is what I send backward -- will be my from front face
void exchangeFaces(FaceBuffer bufs)
{

#ifdef QMP_COMMS


  QMP_start(bufs.mh_from_fwd);
  QMP_start(bufs.mh_from_back);

  QMP_start(bufs.mh_send_back);
  QMP_start(bufs.mh_send_fwd);

  QMP_wait(bufs.mh_send_back);
  QMP_wait(bufs.mh_send_fwd);

  QMP_wait(bufs.mh_from_fwd);
  QMP_wait(bufs.mh_from_back);

  
#else 

#ifndef __DEVICE_EMULATION__

#ifdef OVERLAP_COMMS
  cudaMemcpyAsync(bufs.from_fwd_face, bufs.my_back_face, bufs.nbytes, 
		  cudaMemcpyHostToHost, stream[sendBackIdx]);

  cudaMemcpyAsync(bufs.from_back_face, bufs.my_fwd_face, bufs.nbytes, 
		  cudaMemcpyHostToHost, stream[sendFwdIdx);
#else
  cudaMemcpy(bufs.from_fwd_face, bufs.my_back_face, bufs.nbytes, 
	     cudaMemcpyHostToHost);

  cudaMemcpy(bufs.from_back_face, bufs.my_fwd_face, bufs.nbytes, 
	     cudaMemcpyHostToHost);
#endif

#else
  memcpy(bufs.from_fwd_face, bufs.my_back_face, bufs.nbytes);
  memcpy(bufs.from_back_face, bufs.my_fwd_face, bufs.nbytes);
#endif
#endif
   
}


void exchangeFacesStart(FaceBuffer face, ParitySpinor in, int dagger, 
			cudaStream_t *stream_p)
{
  stream = stream_p;

#ifdef QMP_COMMS
  // Prepost all receives
  QMP_start(face.mh_from_fwd);
  QMP_start(face.mh_from_back);
#endif

  // Gather into face...
  gatherFromSpinor(face, in, dagger);

#ifdef OVERLAP_COMMS
  // Need to wait for copy to finish before sending to neighbour
  cudaStreamSynchronize(stream[sendBackIdx]);
#endif

#ifdef QMP_COMMS
  // Begin backward send
  QMP_start(face.mh_send_back);
#endif

#ifdef OVERLAP_COMMS
  // Need to wait for copy to finish before sending to neighbour
  cudaStreamSynchronize(stream[sendFwdIdx]);
#endif

#ifdef QMP_COMMS
  // Begin forward send
  QMP_start(face.mh_send_fwd);
#endif

}


void exchangeFacesWait(FaceBuffer face, ParitySpinor out, int dagger)
{

  /*
#ifdef QMP_COMMS
  // Make sure all outstanding sends are done
  QMP_wait(face.mh_send_back);
  QMP_wait(face.mh_send_fwd);

  // Finish receives
  QMP_wait(face.mh_from_fwd);
  QMP_wait(face.mh_from_back);
#else
*/
#ifndef QMP_COMMS
// NO QMP -- do copies
#ifndef __DEVICE_EMULATION__

#ifdef OVERLAP_COMMS
  cudaMemcpyAsync(face.from_fwd_face, face.my_back_face, face.nbytes, 
		  cudaMemcpyHostToHost, stream[sendBackIdx]);

  cudaMemcpyAsync(face.from_back_face, face.my_fwd_face, face.nbytes, 
		  cudaMemcpyHostToHost, stream[sendFwdIdx]);
#else
  cudaMemcpy(face.from_fwd_face, face.my_back_face, face.nbytes, 
	     cudaMemcpyHostToHost);

  cudaMemcpy(face.from_back_face, face.my_fwd_face, face.nbytes, 
	     cudaMemcpyHostToHost);
#endif

#else
  memcpy(face.from_fwd_face, face.my_back_face, face.nbytes);
  memcpy(face.from_back_face, face.my_fwd_face, face.nbytes);
#endif
#endif

  // Scatter faces.
  scatterToPads(out, face, dagger);
}

template <int vecLen, typename Float>
void gather12Float(Float* dest, Float* spinor, int Vs, int V, int stride, bool upper, bool tIsZero,
		   int strmIdx)
{
  int Npad = 12/vecLen;  // Number of Pad's in one half spinor...
  int lower_spin_offset=vecLen*Npad*stride;	
  int t_zero_offset=0; // T=0 is the first VS block
  int Nt_minus_one_offset=vecLen*(V - Vs); // N_t -1 = V-Vs & vecLen is from FloatN.

 
  // QUDA Memcpy NPad's worth. 
  //  -- Dest will point to the right beginning PAD. 
  //  -- Each Pad has size vecLen*Vs Floats. 
  //  --  There is vecLen*Stride Floats from the
  //           start of one PAD to the start of the next
#ifdef OVERLAP_COMMS
  for(int i=0; i < Npad; i++) {
    if( upper ) { 
      if( tIsZero ) { 
        cudaMemcpyAsync((void *)(dest + vecLen*i*Vs), 
			(void *)(spinor + t_zero_offset + i*vecLen*stride),
			vecLen*Vs*sizeof(Float),
			cudaMemcpyDeviceToHost, stream[strmIdx]);
      } else {
        cudaMemcpyAsync((void *)(dest + vecLen*i*Vs), 
			(void *)(spinor + Nt_minus_one_offset + i*vecLen*stride),
			vecLen*Vs*sizeof(Float),
			cudaMemcpyDeviceToHost, stream[strmIdx]);      
      }
    } else {
      if( tIsZero ) { 
        cudaMemcpyAsync((void *)(dest + vecLen*i*Vs), 
			(void *)(spinor + lower_spin_offset + t_zero_offset + i*vecLen*stride),
			vecLen*Vs*sizeof(Float),
			cudaMemcpyDeviceToHost, stream[strmIdx]);      
      } else {
        cudaMemcpyAsync((void *)(dest + vecLen*i*Vs), 
			(void *)(spinor + lower_spin_offset + Nt_minus_one_offset + i*vecLen*stride),
			vecLen*Vs*sizeof(Float),
			cudaMemcpyDeviceToHost, stream[strmIdx]);      
      }
    }
  }
#else
  for(int i=0; i < Npad; i++) {
    if( upper ) { 
      if( tIsZero ) { 
        cudaMemcpy((void *)(dest + vecLen*i*Vs), 
                   (void *)(spinor + t_zero_offset + i*vecLen*stride),
	           vecLen*Vs*sizeof(Float),
		   cudaMemcpyDeviceToHost);
      } else {
        cudaMemcpy((void *)(dest + vecLen*i*Vs), 
                   (void *)(spinor + Nt_minus_one_offset + i*vecLen*stride),
	           vecLen*Vs*sizeof(Float),
		   cudaMemcpyDeviceToHost);
      }
    } else {
      if( tIsZero ) { 
        cudaMemcpy((void *)(dest + vecLen*i*Vs), 
		   (void *)(spinor + lower_spin_offset + t_zero_offset + i*vecLen*stride),
		   vecLen*Vs*sizeof(Float),
		   cudaMemcpyDeviceToHost);
      } else {
        cudaMemcpy((void *)(dest + vecLen*i*Vs), 
                   (void *)(spinor + lower_spin_offset + Nt_minus_one_offset + i*vecLen*stride),
	           vecLen*Vs*sizeof(Float),
		   cudaMemcpyDeviceToHost);
      }
    }
  }
#endif

}

// like the above but for the norms required for QUDA_HALF_PRECISION
template <typename Float>
  void gatherNorm(Float* dest, Float* norm, int Vs, int V, int stride, bool tIsZero, int strmIdx)
{
  int t_zero_offset=0; // T=0 is the first VS block
  int Nt_minus_one_offset=V - Vs; // N_t -1 = V-Vs.
 
#ifdef OVERLAP_COMMS
  if( tIsZero ) { 
    cudaMemcpyAsync((void *)dest, 
		    (void *)(norm + t_zero_offset),
		    Vs*sizeof(Float),
		    cudaMemcpyDeviceToHost, stream[strmIdx]); 
  } else {
    cudaMemcpyAsync((void *)dest, 
		    (void *)(norm + Nt_minus_one_offset),
		    Vs*sizeof(Float),
		    cudaMemcpyDeviceToHost, stream[strmIdx]);      
  }
#else
  if( tIsZero ) { 
    cudaMemcpy((void *)dest, 
	       (void *)(norm + t_zero_offset),
	       Vs*sizeof(Float),
	       cudaMemcpyDeviceToHost);      
  } else {
    cudaMemcpy((void *)dest, 
	       (void *)(norm + Nt_minus_one_offset),
	       Vs*sizeof(Float),
	       cudaMemcpyDeviceToHost);      
  }
#endif

}



  // QUDA Memcpy 3 Pad's worth. 
  //  -- Dest will point to the right beginning PAD. 
  //  -- Each Pad has size vecLen * Vs Floats. 
  //  --  There is 4Stride Floats from the
  //           start of one PAD to the start of the next

template <int vecLen, typename Float>
  void scatter12Float(Float* spinor, Float* buf, int Vs, int V, int stride, 
		      bool upper, int strmIdx=0)
{
  int Npad = 12/vecLen;
  int spinor_end = 2*Npad*vecLen*stride;
  int face_size = Npad*vecLen*Vs;
  
#ifdef OVERLAP_COMMS
  if( upper ) { 
    cudaMemcpyAsync((void *)(spinor + spinor_end), (void *)(buf), face_size*sizeof(Float), 
		    cudaMemcpyHostToDevice, stream[strmIdx]);
  } else {
    cudaMemcpyAsync((void *)(spinor + spinor_end + face_size), (void *)(buf), face_size*sizeof(Float), 
		    cudaMemcpyHostToDevice, stream[strmIdx]);
#else
  if( upper ) { 
    cudaMemcpy((void *)(spinor + spinor_end), (void *)(buf), face_size*sizeof(Float), cudaMemcpyHostToDevice);
  } else {
    cudaMemcpy((void *)(spinor + spinor_end + face_size), (void *)(buf), face_size*sizeof(Float), cudaMemcpyHostToDevice);

#endif
  }
  
}

// half precision norm version of the above
template <typename Float>
  void scatterNorm(Float* norm, Float* buf, int Vs, int V, int stride, bool upper, int strmIdx=0)
{
  int norm_end = stride;
  int face_size = Vs;

#ifdef OVERLAP_COMMS
  if (upper) { // upper goes in the first norm zone
    cudaMemcpyAsync((void *)(norm + norm_end), (void *)(buf), Vs*sizeof(Float), 
		    cudaMemcpyHostToDevice, stream[strmIdx]);  
  } else { // lower goes in the second norm zone
    cudaMemcpyAsync((void *)(norm + norm_end + face_size), (void *)(buf), Vs*sizeof(Float), 
		    cudaMemcpyHostToDevice, stream[strmIdx]);
  }
#else
  if (upper) { // upper goes in the first norm zone
    cudaMemcpy((void *)(norm + norm_end), (void *)(buf), Vs*sizeof(Float), cudaMemcpyHostToDevice);
  } else { // lower goes in the second norm zone
    cudaMemcpy((void *)(norm + norm_end + face_size), (void *)(buf), Vs*sizeof(Float), cudaMemcpyHostToDevice);
  }
#endif
}

void gatherFromSpinor(FaceBuffer face, ParitySpinor in, int dagger)
{
  
  // I need to gather the faces with opposite Checkerboard
  // Depending on whether I do dagger or not I want top 2 components
  // from forward, and bottom 2 components from backward
  if (!dagger) { 

    // Not HC: send lower components back, receive them from forward
    // lower components = buffers 4,5,6

    if (in.precision == QUDA_DOUBLE_PRECISION) {
      // lower_spins => upper = false, t=0, so tIsZero=true 
      gather12Float<2>((double *)(face.my_back_face), (double *)in.spinor, 
		       face.Vs, face.V, face.stride, false, true, sendBackIdx);
    
      // Not Hermitian conjugate: send upper spinors forward/recv from back
      // upper spins => upper = true,t=Nt-1 => tIsZero=false
      gather12Float<2>((double *)(face.my_fwd_face), (double *)in.spinor,
		       face.Vs, face.V, face.stride, true, false, sendFwdIdx);
    
    } else if (in.precision == QUDA_SINGLE_PRECISION) {
      // lower_spins => upper = false, t=0, so tIsZero=true 
      gather12Float<4>((float *)(face.my_back_face), (float *)in.spinor, 
		       face.Vs, face.V, face.stride, false, true, sendBackIdx);
    
      // Not Hermitian conjugate: send upper spinors forward/recv from back
      // upper spins => upper = true,t=Nt-1 => tIsZero=false
      gather12Float<4>((float *)(face.my_fwd_face), (float *)in.spinor,
		       face.Vs, face.V, face.stride, true, false, sendFwdIdx);
    
    } else {       
      // lower_spins => upper = false, t=0, so tIsZero=true 
      gather12Float<4>((short *)(face.my_back_face), (short *)in.spinor, 
		       face.Vs, face.V, face.stride, false, true, sendBackIdx);
    
      gatherNorm((float*)((short*)face.my_back_face+12*face.Vs), 
		 (float*)in.spinorNorm, face.Vs, face.V, face.stride, true, sendBackIdx);

      // Not Hermitian conjugate: send upper spinors forward/recv from back
      // upper spins => upper = true,t=Nt-1 => tIsZero=false
      gather12Float<4>((short *)(face.my_fwd_face), (short *)in.spinor,
		       face.Vs, face.V, face.stride, true, false, sendFwdIdx);

      gatherNorm((float*)((short*)face.my_fwd_face+12*face.Vs), 
		 (float*)in.spinorNorm, face.Vs, face.V, face.stride, false, sendFwdIdx);
    }
 
  } else { 

    if (in.precision == QUDA_DOUBLE_PRECISION) {
      // HC: Send upper components back, receive them from front
      // upper spins => upper = true,t=Nt-1 => tIsZero=false
      gather12Float<2>((double *)(face.my_back_face), (double *)in.spinor,
		       face.Vs, face.V, face.stride, true, true, sendBackIdx);
    
      // HC: send lower components fwd, receive them from back
      // Lower Spins => upper = false, t=Nt-1      => tIsZero = true
      gather12Float<2>((double *)(face.my_fwd_face), (double *)in.spinor, 
		       face.Vs, face.V, face.stride, false, false, sendFwdIdx);    
    } else if (in.precision == QUDA_SINGLE_PRECISION) {
      // HC: Send upper components back, receive them from front
      // upper spins => upper = true,t=Nt-1 => tIsZero=false
      gather12Float<4>((float *)(face.my_back_face), (float *)in.spinor,
		       face.Vs, face.V, face.stride, true, true, sendBackIdx);
    
      // Lower Spins => upper = false, t=Nt-1      => tIsZero = true
      gather12Float<4>((float *)(face.my_fwd_face), (float *)in.spinor, 
		       face.Vs, face.V, face.stride, false, false, sendFwdIdx);
    
    } else {       
      // HC: Send upper components back, receive them from front
      //UpperSpins => upper = true, t=0 => tIsZero = true
      gather12Float<4>((short *)(face.my_back_face), (short *)in.spinor,
		       face.Vs, face.V, face.stride, true, true, sendBackIdx);

      gatherNorm((float*)((short*)face.my_back_face+12*face.Vs), 
		 (float*)in.spinorNorm, face.Vs, face.V, face.stride, true, sendBackIdx);

      // lower_spins => upper = false, t=0, so tIsZero=true 
      gather12Float<4>((short *)(face.my_fwd_face), (short *)in.spinor, 
		       face.Vs, face.V, face.stride, false, false, sendFwdIdx);
    
      gatherNorm((float*)((short*)face.my_fwd_face+12*face.Vs), 
		 (float*)in.spinorNorm, face.Vs, face.V, face.stride, false, sendFwdIdx);

    }

  }
}

  // Finish backwards send and forwards receive
#ifdef QMP_COMMS				
#define QMP_finish_from_fwd			\
  QMP_wait(face.mh_send_back);			\
  QMP_wait(face.mh_from_fwd);			

// Finish forwards send and backwards receive
#define QMP_finish_from_back			\
  QMP_wait(face.mh_send_fwd);			\
  QMP_wait(face.mh_from_back);			
#else
#define QMP_finish_from_fwd
#define QMP_finish_from_back
#endif


void scatterToPads(ParitySpinor out, FaceBuffer face, int dagger)
{
 
  // I need to gather the faces with opposite Checkerboard
  // Depending on whether I do dagger or not I want top 2 components
  // from forward, and bottom 2 components from backward
  if (!dagger) { 

    if (out.precision == QUDA_DOUBLE_PRECISION) {
      // Not HC: send lower components back, receive them from forward
      // lower components = buffers 4,5,6
      QMP_finish_from_fwd;

      scatter12Float<2>((double *)out.spinor, (double *)face.from_fwd_face, 
			face.Vs, face.V, face.stride, false, recFwdIdx); // LOWER
      
      QMP_finish_from_back;

      // Not H: Send upper components forward, receive them from back
      scatter12Float<2>((double *)out.spinor, (double *)face.from_back_face,
			face.Vs, face.V, face.stride, true, recBackIdx);  // Upper
    } else if (out.precision == QUDA_SINGLE_PRECISION) {
      QMP_finish_from_fwd;

      // Not HC: send lower components back, receive them from forward
      // lower components = buffers 4,5,6
      scatter12Float<4>((float *)out.spinor, (float *)face.from_fwd_face, 
			face.Vs, face.V, face.stride, false, recFwdIdx); // LOWER
      
      QMP_finish_from_back;

      // Not H: Send upper components forward, receive them from back
      scatter12Float<4>((float *)out.spinor, (float *)face.from_back_face,
			face.Vs, face.V, face.stride, true, recBackIdx); // Upper
    } else {
      QMP_finish_from_fwd;

      // Not HC: send lower components back, receive them from forward
      // lower components = buffers 4,5,6
      scatter12Float<4>((short *)out.spinor, (short *)face.from_fwd_face, 
			face.Vs, face.V, face.stride, false, recFwdIdx); // LOWER
      
      scatterNorm((float*)out.spinorNorm, (float*)((short*)face.from_fwd_face+12*face.Vs), 
		  face.Vs, face.V, face.stride, false, recFwdIdx);

      QMP_finish_from_back;

      // Not H: Send upper components forward, receive them from back
      scatter12Float<4>((short *)out.spinor, (short *)face.from_back_face,
			face.Vs, face.V, face.stride, true, recBackIdx); // Upper

      scatterNorm((float*)out.spinorNorm, (float*)((short*)face.from_back_face+12*face.Vs),
		  face.Vs, face.V, face.stride, true, recBackIdx); // Lower

    }
    
  } else { 
    if (out.precision == QUDA_DOUBLE_PRECISION) {
      QMP_finish_from_fwd;

      // HC: send lower components fwd, receive them from back
      // upper components = buffers 1, 2,3, go forward (into my_fwd face)
      scatter12Float<2>((double *)out.spinor, (double *)face.from_fwd_face,
			face.Vs, face.V, face.stride, true, recFwdIdx);       

      QMP_finish_from_back;

      // lower components = buffers 4,5,6
      scatter12Float<2>((double *)out.spinor, (double *)face.from_back_face,
			face.Vs, face.V, face.stride, false, recBackIdx);       // Lower
      
    } else if (out.precision == QUDA_SINGLE_PRECISION) {
      QMP_finish_from_fwd;

      // HC: send lower components fwd, receive them from back
      // upper components = buffers 1, 2,3, go forward (into my_fwd face)
      scatter12Float<4>((float *)out.spinor, (float *)face.from_fwd_face,
			face.Vs, face.V, face.stride, true, recFwdIdx );

      QMP_finish_from_back;

       // lower components = buffers 4,5,6
      scatter12Float<4>((float *)out.spinor, (float *)face.from_back_face,
			face.Vs, face.V, face.stride, false, recBackIdx);       // Lower
      
   } else {
      QMP_finish_from_fwd;

      // HC: send lower components fwd, receive them from back
      // upper components = buffers 1, 2,3, go forward (into my_fwd face)
      scatter12Float<4>((short *)out.spinor, (short *)face.from_fwd_face,
			face.Vs, face.V, face.stride, true, recFwdIdx );
 
      scatterNorm((float*)out.spinorNorm, (float*)((short*)face.from_fwd_face+12*face.Vs), 
		  face.Vs, face.V, face.stride, true, recFwdIdx);

      QMP_finish_from_back;

      // lower components = buffers 4,5,6
      scatter12Float<4>((short *)out.spinor, (short *)face.from_back_face,
			face.Vs, face.V, face.stride, false, recBackIdx);       // Lower
      
      scatterNorm((float*)out.spinorNorm, (float*)((short*)face.from_back_face+12*face.Vs),
		  face.Vs, face.V, face.stride, false, recBackIdx);

    }

  }
}


void transferGaugeFaces(void *gauge, void *gauge_face, Precision precision,
			int veclength, ReconstructType reconstruct, int V, int Vs)
{
  int nblocks, ndim=4;
  size_t linksize, blocksize, nbytes;
  ptrdiff_t offset, stride;
  void *g, *gf;

  switch (reconstruct) {
  case QUDA_RECONSTRUCT_NO:
    linksize = 18;
    break;
  case QUDA_RECONSTRUCT_12:
    linksize = 12;
    break;
  case QUDA_RECONSTRUCT_8:
    linksize = 8;
    break;
  default:
    errorQuda("Invalid reconstruct type");
  }
  nblocks = ndim*linksize/veclength;
  blocksize = Vs*veclength*precision;
  offset = (V-Vs)*veclength*precision;
  stride = (V+Vs)*veclength*precision; // assume that pad = Vs
  // stride = V*veclength*precision;
  // nbytes = Vs*ndim*linksize*precision; /* for contiguous face buffer */

#ifdef QMP_COMMS

  g = (void *) ((char *) gauge + offset);
  mm_gauge_send_fwd = QMP_declare_strided_msgmem(g, blocksize, nblocks, stride);
  if (!mm_gauge_send_fwd) {
    errorQuda("Unable to allocate gauge message mem");
  }
  // mm_gauge_from_back = QMP_declare_msgmem(gauge_face, nbytes); /* for contiguous face buffer */
  mm_gauge_from_back = QMP_declare_strided_msgmem(gauge_face, blocksize, nblocks, stride);
  if (!mm_gauge_from_back) { 
    errorQuda("Unable to allocate gauge face message mem");
  }

  mh_gauge_send_fwd = QMP_declare_send_relative(mm_gauge_send_fwd, 3, +1, 0);
  if (!mh_gauge_send_fwd) {
    errorQuda("Unable to allocate gauge message handle");
  }
  mh_gauge_from_back = QMP_declare_receive_relative(mm_gauge_from_back, 3, -1, 0);
  if (!mh_gauge_from_back) {
    errorQuda("Unable to allocate gauge face message handle");
  }

  QMP_start(mh_gauge_send_fwd);
  QMP_start(mh_gauge_from_back);
  
  QMP_wait(mh_gauge_send_fwd);
  QMP_wait(mh_gauge_from_back);

  QMP_free_msghandle(mh_gauge_send_fwd);
  QMP_free_msghandle(mh_gauge_from_back);
  QMP_free_msgmem(mm_gauge_send_fwd);
  QMP_free_msgmem(mm_gauge_from_back);

#else 

  for (int i=0; i<nblocks; i++) {
    g = (void *) ((char *) gauge + offset + i*stride);
    gf = (void *) ((char *) gauge_face + i*stride);
    // gf = (void *) ((char *) gauge_face + i*blocksize); /* for contiguous face buffer */

#ifndef __DEVICE_EMULATION__

    // I don't think stream has been set here so can't use async copy
    /*#ifdef OVERLAP_COMMS
    cudaMemcpyAsync(gf, g, blocksize, cudaMemcpyHostToHost, *stream);
    #else*/
    cudaMemcpy(gf, g, blocksize, cudaMemcpyHostToHost);
    //#endif

#else
    memcpy(gf, g, blocksize);
#endif
  }

#endif // QMP_COMMS
}
