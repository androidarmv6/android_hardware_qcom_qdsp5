/*
	An Open max test application ....
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>
#ifdef _ANDROID_
#include <binder/MemoryHeapBase.h>
#endif
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_QCOMExtns.h"
#include "qtv_msg.h"

#include <stdint.h>
#include <signal.h>
#include <linux/msm_mdp.h>
#include <linux/fb.h>
#include "qutility.h"

/************************************************************************/
/*				#DEFINES	                        */
/************************************************************************/

#define false 0
#define true 1
#define H264_START_CODE         0x00000001
#define VOP_START_CODE 0x000001B6
#define SHORT_HEADER_START_CODE 0x00008000
#define VC1_START_CODE  0x00000100
#define NUMBER_OF_ARBITRARYBYTES_READ  (4 * 1024)
#define VC1_SEQ_LAYER_SIZE_WITHOUT_STRUCTC 32
#define VC1_SEQ_LAYER_SIZE_V1_WITHOUT_STRUCTC 16

#define CONFIG_VERSION_SIZE(param) \
	param.nVersion.nVersion = CURRENT_OMX_SPEC_VERSION;\
	param.nSize = sizeof(param);

#define FAILED(result) (result != OMX_ErrorNone)

#define SUCCEEDED(result) (result == OMX_ErrorNone)  
#define SWAPBYTES(ptrA, ptrB) { char t = *ptrA; *ptrA = *ptrB; *ptrB = t;}
#define SIZE_NAL_FIELD_MAX  4
#define MDP_DEINTERLACE 0x80000000

/************************************************************************/
/*				GLOBAL DECLARATIONS                     */
/************************************************************************/
#ifdef _ANDROID_
using namespace android;
#endif

typedef enum {
  CODEC_FORMAT_H264 = 1,
  CODEC_FORMAT_MP4,
  CODEC_FORMAT_H263,
  CODEC_FORMAT_VC1,
  CODEC_FORMAT_MAX = CODEC_FORMAT_VC1
} codec_format;

typedef enum {
  FILE_TYPE_DAT_PER_AU = 1,
  FILE_TYPE_ARBITRARY_BYTES,
  FILE_TYPE_COMMON_CODEC_MAX,

  FILE_TYPE_START_OF_H264_SPECIFIC = 10,
  FILE_TYPE_264_NAL_SIZE_LENGTH = FILE_TYPE_START_OF_H264_SPECIFIC,
  FILE_TYPE_START_CODE,

  FILE_TYPE_START_OF_MP4_SPECIFIC = 20,
  FILE_TYPE_PICTURE_START_CODE = FILE_TYPE_START_OF_MP4_SPECIFIC,

  FILE_TYPE_START_OF_VC1_SPECIFIC = 30,
  FILE_TYPE_RCV = FILE_TYPE_START_OF_VC1_SPECIFIC,
  FILE_TYPE_VC1
} file_type;

typedef enum {
  GOOD_STATE = 0,
  PORT_SETTING_CHANGE_STATE,
  ERROR_STATE,
  INVALID_STATE
} test_status;

typedef enum {
  FREE_HANDLE_AT_LOADED = 1,
  FREE_HANDLE_AT_IDLE,
  FREE_HANDLE_AT_EXECUTING,
  FREE_HANDLE_AT_PAUSE
} freeHandle_test;

static int (*Read_Buffer)(OMX_BUFFERHEADERTYPE  *pBufHdr );

FILE * inputBufferFile;   
FILE * outputBufferFile; 
int takeYuvLog = 0;
int displayYuv = 0;
int realtime_display = 0;

pthread_mutex_t eos_lock;
pthread_cond_t eos_cond;
pthread_mutex_t lock;
pthread_cond_t cond;
pthread_t ebd_thread_id;
pthread_t fbd_thread_id;
pthread_t fbiopan_thread_id;
void* ebd_thread(void*);
void* fbd_thread(void*);
void* fbiopan_thread(void*);

int ebd_pipe [2]= {0};
int fbd_pipe[2] = {0};
int fbiopan_pipe[2] = {0};

typedef struct 
{
    OMX_IN OMX_HANDLETYPE hComponent;
    OMX_IN OMX_PTR pAppData;
    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer;      
}bufferdone_OMX_data;

OMX_PARAM_PORTDEFINITIONTYPE portFmt;
OMX_PORT_PARAM_TYPE portParam;
OMX_ERRORTYPE error;

static int fb_fd = -1;
static struct fb_var_screeninfo vinfo;
void render_fb(struct OMX_BUFFERHEADERTYPE *pBufHdr);

/************************************************************************/
/*				GLOBAL INIT			        */
/************************************************************************/
int input_buf_cnt = 0;
int output_buf_cnt = 0;
int height =0, width =0;
volatile int event_is_done = 0;
int ebd_cnt, fbd_cnt;
int bInputEosReached = 0;
int bOutputEosReached = 0;
char in_filename[512];

unsigned char omx_allocate_buffer_option = true;

struct timeval t_avsync={0};

unsigned char struct_c_only = false;
int timeStampLfile = 0;
int timestampInterval = 33333;  // default 33333 microsec
static struct sigaction sigact = {0};
codec_format  codec_format_option;
file_type     file_type_option;
freeHandle_test freeHandle_option;
int nalSize;
int sent_disabled = 0;
int waitForPortSettingsChanged = 1;
test_status currentStatus = GOOD_STATE;

//* OMX Spec Version supported by the wrappers. Version = 1.1 */
const OMX_U32 CURRENT_OMX_SPEC_VERSION = 0x00000101;
OMX_COMPONENTTYPE* dec_handle = 0;

OMX_BUFFERHEADERTYPE  **pInputBufHdrs = NULL;	
OMX_BUFFERHEADERTYPE  **pOutYUVBufHdrs= NULL;

int rcv_v1=0;

/* Performance related variable*/
QPERF_INIT(render_fb);
QPERF_INIT(client_decode);

/************************************************************************/
/*				GLOBAL FUNC DECL                        */
/************************************************************************/
int Init_Decoder();
int Play_Decoder();
int run_tests();

/**************************************************************************/
/*				STATIC DECLARATIONS                       */
/**************************************************************************/
static int video_playback_count = 1; 
static int open_video_file ();
static int Read_Buffer_From_DAT_File(OMX_BUFFERHEADERTYPE  *pBufHdr );
static int Read_Buffer_ArbitraryBytes(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_H264_Start_Code_File(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_Vop_Start_Code_File(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_Size_Nal(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_RCV_File_Seq_Layer(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_RCV_File(OMX_BUFFERHEADERTYPE  *pBufHdr);
static int Read_Buffer_From_VC1_File(OMX_BUFFERHEADERTYPE  *pBufHdr);

static OMX_ERRORTYPE Allocate_Buffer ( OMX_COMPONENTTYPE *dec_handle, 
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs, 
                                       OMX_U32 nPortIndex, 
                                       long bufCntMin, long bufSize); 
static OMX_ERRORTYPE Use_Buffer( OMX_COMPONENTTYPE     *avc_dec_handle, 
                                 OMX_BUFFERHEADERTYPE  ***pBufHdrs, 
                                 OMX_U32               nPortIndex,
                                 OMX_U32               bufCntMin,
                                 OMX_U32               bufSize,
                                 OMX_U8                *buffer);
static OMX_ERRORTYPE EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                  OMX_IN OMX_PTR pAppData, 
                                  OMX_IN OMX_EVENTTYPE eEvent,
                                  OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2, 
                                  OMX_IN OMX_PTR pEventData);
static OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent, 
                                     OMX_IN OMX_PTR pAppData, 
                                     OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);
static OMX_ERRORTYPE FillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent, 
                                    OMX_OUT OMX_PTR pAppData, 
                                    OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer);

static void do_freeHandle_and_clean_up(bool isDueToError);
static void freeHandle_CloseAllFilesAndPipes();

/*static usecs_t get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return ((usecs_t)tv.tv_usec) +
        (((usecs_t)tv.tv_sec) * ((usecs_t)1000000));
}*/


void wait_for_event(void)
{
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Waiting for event\n");
    pthread_mutex_lock(&lock);
    while (event_is_done == 0) {
        pthread_cond_wait(&cond, &lock);
    }
    event_is_done = 0;
    pthread_mutex_unlock(&lock);
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Running .... get the event\n");
}

void event_complete(void )
{
    pthread_mutex_lock(&lock);
    if (event_is_done == 0) {
        event_is_done = 1;
        pthread_cond_broadcast(&cond);
    }
    pthread_mutex_unlock(&lock);
}

void* ebd_thread(void* pArg)
{
  while(1)
  {
    int readBytes =0;
    OMX_BUFFERHEADERTYPE* pBuffer = NULL;
    int ioresult =0;
    bufferdone_OMX_data ebd_omx_data = {0};

    ioresult = read (ebd_pipe[0],&ebd_omx_data,sizeof(ebd_omx_data)); 
    if(ioresult <= 0)
    {
      printf("\n Error in writing to ebd PIPE %d \n", ioresult);
      break;
    }

    pBuffer = ebd_omx_data.pBuffer;
    if(pBuffer == NULL)
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                   "Error - No etb pBuffer to dequeue\n");
      continue;
    }
    pBuffer->nOffset = 0;
    if((readBytes = Read_Buffer(pBuffer)) > 0) { 
      pBuffer->nFilledLen = readBytes;
      OMX_EmptyThisBuffer(dec_handle,pBuffer);
    }
    else
    {
      pBuffer->nFlags |= OMX_BUFFERFLAG_EOS;
      bInputEosReached = true;
      pBuffer->nFilledLen = readBytes;
      OMX_EmptyThisBuffer(dec_handle,pBuffer);
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,
                   "EBD::Either EOS or Some Error while reading file\n");
      break;
    }
  }
  return NULL;
}

/* Signal Handler function to catch SIGINT signal and perform cleanup actions */
void sig_handler(int signum)
{   
  if(signum == SIGINT) 
  {
    printf("Caught SIGINT signal from user\n");

    if(displayYuv)
    {
       if (fb_fd > 0)
       {
         close (fb_fd);
         fb_fd = -1;
       }
    }

    do_freeHandle_and_clean_up(true);

    printf("\n\n Frame BUF COUNT- TO DSP:%d FROM DSP:%d \n\n",ebd_cnt,fbd_cnt);
    exit(0);
  }
}


/* Function to install signal handler for SIGINT */
static void install_sighandler(void)
{
  sigact.sa_handler = (__sighandler_t)sig_handler;
  memset(&(sigact.sa_mask), 0, sizeof(sigact.sa_mask));
  sigact.sa_flags = SA_SIGINFO;
  sigact.sa_restorer = NULL;
  sigaction(SIGINT, &sigact,NULL);
}

void* fbiopan_thread(void* pArg)
{
  int ioresult = 0;
  char fbiopan_signal = 0;

  while(1)
  {
    ioresult = read(fbiopan_pipe[0], &fbiopan_signal, 1);
    if(ioresult <= 0)
    {
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"\n Error in reading from fbiopan PIPE %d \n", ioresult);
      return NULL;
    }

    vinfo.activate = FB_ACTIVATE_VBL;
    vinfo.xoffset = 0;
    vinfo.yoffset = 0;
    
    if(ioctl(fb_fd, FBIOPAN_DISPLAY, &vinfo) < 0) 
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"FBIOPAN_DISPLAY: Failed\n");
    }
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"render_fb complete!\n");
  }
  return NULL;
}

/* Display Thread */
void* fbd_thread(void* pArg)
{
  long current_avsync_time = 0, delta_time = 0;
  int canDisplay = 1;
  static int contigous_drop_frame = 0;
  static long base_avsync_time = 0;
  static long base_timestamp = 0;
  long lipsync_time = 250000;
  char fbiopan_signal = 1;

  while(1)
  {
    OMX_BUFFERHEADERTYPE *pBuffer = NULL;
    bufferdone_OMX_data fbd_omx_data = {0};
    int ioresult = 0;
    int bytes_written = 0;

    ioresult = read(fbd_pipe[0], &fbd_omx_data, sizeof(bufferdone_OMX_data));
    if(ioresult <= 0)
    {
      printf("\n Error in reading from fbd PIPE %d \n", ioresult);
      break;
    }
    pBuffer = fbd_omx_data.pBuffer;

    if (realtime_display) 
    {
      if(!gettimeofday(&t_avsync,NULL))
      {
         current_avsync_time =(t_avsync.tv_sec*1000000)+t_avsync.tv_usec;
      }
      
      if (base_avsync_time != 0)
      {  
        delta_time = (current_avsync_time - base_avsync_time) - (fbd_omx_data.pBuffer->nTimeStamp - base_timestamp);
        if (delta_time < 0 ) 
        {
          QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Sleep %d us. AV Sync time is left behind\n", 
                 -delta_time);
          usleep(-delta_time);
          canDisplay = 1;
        }
        else if ((delta_time>lipsync_time) && (contigous_drop_frame < 6))
        {
          QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"Error - Drop the frame at the renderer. Video frame with ts %d behind by %d us\n", 
                        delta_time, fbd_omx_data.pBuffer->nTimeStamp);
          canDisplay = 0;
          contigous_drop_frame++;
        }
        else
        {
          canDisplay = 1;
        }
      }
      else
      {
        base_avsync_time = current_avsync_time;
        base_timestamp = fbd_omx_data.pBuffer->nTimeStamp;
      }
    }
    
    if (displayYuv && canDisplay && pBuffer->nFilledLen > 0)
    {
      QPERF_TIME(render_fb, render_fb(pBuffer));
      contigous_drop_frame = 0;
    }

    if (takeYuvLog) 
    {
      /*********************************************
      Write the output of the decoder to the file.
      *********************************************/
      bytes_written = fwrite((const char *)pBuffer->pBuffer,
                              pBuffer->nFilledLen,1,outputBufferFile);
      if (bytes_written < 0) {
          QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                       "\nFillBufferDone: Failed to write to the file\n");
      } 
      else {
          QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                        "\nFillBufferDone: Wrote %d YUV bytes to the file\n",
                        bytes_written);
      }
    }

    /********************************************************************/
    /* De-Initializing the open max and relasing the buffers and */
    /* closing the files.*/
    /********************************************************************/
    if (pBuffer->nFlags & OMX_BUFFERFLAG_EOS ) {
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                   "***************************************************\n");
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                   "FillBufferDone: End Of Stream Reached\n");
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                   "***************************************************\n");

      pthread_mutex_lock(&eos_lock);
      bOutputEosReached = true;
      pthread_cond_broadcast(&eos_cond);
      pthread_mutex_unlock(&eos_lock);

      QPERF_END(client_decode);
      QPERF_SET_ITERATION(client_decode, fbd_cnt);
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                   "***************************************************\n");
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"FBD_THREAD bOutputEosReached %d\n",bOutputEosReached);
      break;
    }
    OMX_FillThisBuffer(dec_handle, pBuffer);

    ioresult =  write(fbiopan_pipe[1], &fbiopan_signal, 1);
    if(ioresult < 0)
    {
       QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"\n Error in writing to fbd PIPE %d \n", ioresult);
       break;
    } 
  }
  return NULL;
}

OMX_ERRORTYPE EventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                           OMX_IN OMX_PTR pAppData, 
                           OMX_IN OMX_EVENTTYPE eEvent,
                           OMX_IN OMX_U32 nData1, OMX_IN OMX_U32 nData2,
                           OMX_IN OMX_PTR pEventData)
{	
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Function %s \n", __FUNCTION__);

    switch(eEvent) {
        case OMX_EventCmdComplete:
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\n OMX_EventCmdComplete \n");
            // check nData1 for DISABLE event
            if(OMX_CommandPortDisable == (OMX_COMMANDTYPE)nData1)
            {
                QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                             "*********************************************\n");
                QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                              "Recieved DISABLE Event Command Complete[%d]\n",nData2);
                QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                             "*********************************************\n");
                sent_disabled = 0;
            }
            else if(OMX_CommandPortEnable == (OMX_COMMANDTYPE)nData1)
            {
                QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                             "*********************************************\n");
                QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                              "Recieved ENABLE Event Command Complete[%d]\n",nData2);
                QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                             "*********************************************\n");
            }
            currentStatus = GOOD_STATE;
            event_complete();
            break;

        case OMX_EventError:
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                         "OMX_EventError \n");
            currentStatus = ERROR_STATE;
            if (OMX_ErrorInvalidState == (OMX_ERRORTYPE)nData1) 
            {
              QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                           "Invalid State \n");
              currentStatus = INVALID_STATE;
            }
            event_complete();
            break;
        case OMX_EventPortSettingsChanged:
            QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                          "OMX_EventPortSettingsChanged port[%d]\n",nData1);
            waitForPortSettingsChanged = 0;
            currentStatus = PORT_SETTING_CHANGE_STATE;
            // reset the event 
            event_complete();
            break;

        default:
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"ERROR - Unknown Event \n");
            break;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE EmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent, 
                              OMX_IN OMX_PTR pAppData, 
                              OMX_IN OMX_BUFFERHEADERTYPE* pBuffer)
{
    int readBytes =0; int bufCnt=0;
    OMX_ERRORTYPE result;
    bufferdone_OMX_data ebd_omx_data = {0};
    int ioresult = 0;

    QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                  "Function %s cnt[%d]\n", __FUNCTION__, ebd_cnt);

    if(bInputEosReached) {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                     "*****EBD:Input EoS Reached************\n");
        return OMX_ErrorNone;
    }

    ebd_cnt++;
    ebd_omx_data.hComponent = hComponent;
    ebd_omx_data.pAppData = pAppData;
    ebd_omx_data.pBuffer = pBuffer;

    ioresult = write (ebd_pipe[1],&ebd_omx_data,sizeof(ebd_omx_data));    
    if(ioresult < 0)
    {
       printf("\n Error in writing to ebd PIPE %d \n", ioresult);
       exit(1);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE FillBufferDone(OMX_OUT OMX_HANDLETYPE hComponent,
                             OMX_OUT OMX_PTR pAppData, 
                             OMX_OUT OMX_BUFFERHEADERTYPE* pBuffer) 
{
  bufferdone_OMX_data fbd_omx_data = {0};
  int ioresult = 0;
  QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                "Inside %s callback_count[%d] \n", __FUNCTION__, fbd_cnt);

  /* Test app will assume there is a dynamic port setting
   * In case that there is no dynamic port setting, OMX will not call event cb, 
   * instead OMX will send empty this buffer directly and we need to clear an event here
   */
  if(waitForPortSettingsChanged)
  {
    currentStatus = GOOD_STATE;
    waitForPortSettingsChanged = 0;
    event_complete();
  }
  
  if(pBuffer->nFilledLen !=0) 
      fbd_cnt++;

  fbd_omx_data.hComponent = hComponent;
  fbd_omx_data.pAppData = pAppData;
  fbd_omx_data.pBuffer = pBuffer;

  if(!sent_disabled)
  {
    ioresult =  write(fbd_pipe[1], &fbd_omx_data, sizeof(bufferdone_OMX_data));
  }

  if(ioresult < 0)
  {
     printf("\n Error in writing to fbd PIPE %d \n", ioresult);
     exit(1);
  }  
  return OMX_ErrorNone;
}

int main(int argc, char **argv) 
{
    int i=0; 
    int bufCnt=0; 
    int num=0;
    int outputOption = 0;
    int test_option = 0;
    OMX_ERRORTYPE result;

    if (argc < 2) 
    {
      printf("To use it: ./mm-vdec-omx-test <clip location> \n");
      printf("Command line argument is also available\n");
      return -1;
    }
    strncpy(in_filename, argv[1], strlen(argv[1])+1);

    if(argc > 5)
    {
      codec_format_option = (codec_format)atoi(argv[2]);
      file_type_option = (file_type)atoi(argv[3]);
    }
    else
    {
      printf("Command line argument is available\n");
      printf("To use it: ./mm-vdec-omx-test <clip location> <codec_type> \n");
      printf("           <input_type: 1. per AU(.dat), 2. arbitrary, 3.per NAL/frame>\n");
      printf("           <output_type> <test_case> \n");
      printf("           <realtime_display: 0 if ASAP, 1 if real time, only for non-arbitrary option>\n");
      printf("           <fps> <size_nal if H264>\n\n\n");

      printf(" *********************************************\n");
      printf(" ENTER THE TEST CASE YOU WOULD LIKE TO EXECUTE\n");
      printf(" *********************************************\n");
      printf(" 1--> H264\n");
      printf(" 2--> MP4\n");
      printf(" 3--> H263\n");
      printf(" 4--> VC1\n");
      fflush(stdin);
      scanf("%d", &codec_format_option);
      fflush(stdin);
  
      if (codec_format_option > CODEC_FORMAT_MAX) 
      {
          printf(" Wrong test case...[%d] \n", codec_format_option);
          return -1;
      }
  
      printf(" *********************************************\n");
      printf(" ENTER THE TEST CASE YOU WOULD LIKE TO EXECUTE\n");
      printf(" *********************************************\n");
      printf(" 1--> PER ACCESS UNIT CLIP (.dat). Clip only available for H264 and Mpeg4\n");
      // printf(" 2--> ARBITRARY BYTES (need .264/.264c/.mv4/.263/.rcv/.vc1) - Currently not supported at 7k\n");
      if (codec_format_option == CODEC_FORMAT_H264) 
      {
        printf(" 3--> NAL LENGTH SIZE CLIP (.264c)\n");
        printf(" 4--> START CODE CLIP (.264)\n");
      }
      else if ( (codec_format_option == CODEC_FORMAT_MP4) || (codec_format_option == CODEC_FORMAT_H263) ) 
      {
        printf(" 3--> MP4 VOP or H263 P0 SHORT HEADER START CODE CLIP (.m4v or .263)\n");
      }
      else if (codec_format_option == CODEC_FORMAT_VC1) 
      {
        printf(" 3--> VC1 clip Simple/Main Profile (.rcv)\n");
        printf(" 4--> VC1 clip Advance Profile (.vc1)\n");
      }
      fflush(stdin);
      scanf("%d", &file_type_option);
      fflush(stdin);
    }

    /* For temporary, arbitrary bytes option is unsupported */
    if (file_type_option == 2) 
    {
      printf("Arbitrary bytes option is not supported. Please choose other option\n");
      return -1;
    }


    if (file_type_option >= FILE_TYPE_COMMON_CODEC_MAX)
    {
      switch (codec_format_option) 
      {
        case CODEC_FORMAT_H264:
          file_type_option = (file_type)(FILE_TYPE_START_OF_H264_SPECIFIC + file_type_option - FILE_TYPE_COMMON_CODEC_MAX);
          break;
        case CODEC_FORMAT_MP4:
        case CODEC_FORMAT_H263:
          file_type_option = (file_type)(FILE_TYPE_START_OF_MP4_SPECIFIC + file_type_option - FILE_TYPE_COMMON_CODEC_MAX);
          break;
        case CODEC_FORMAT_VC1:
          file_type_option = (file_type)(FILE_TYPE_START_OF_VC1_SPECIFIC + file_type_option - FILE_TYPE_COMMON_CODEC_MAX);
          break;
        default:
          printf("Error: Unknown code %d\n", codec_format_option);
      }
    }

    if(argc > 5)
    {
      outputOption = atoi(argv[4]);
      test_option = atoi(argv[5]);

      if ((file_type_option != FILE_TYPE_ARBITRARY_BYTES) && (argc > 6)) 
      {
        realtime_display = atoi(argv[6]);
      }

      if (realtime_display && (argc > 7)) 
      {
        int fps = atoi(argv[7]);
        timestampInterval = 1000000/fps;
      }

      if (argc > 8) 
      {
        nalSize = atoi(argv[8]);
      }
      else
      {
        nalSize = 0;
      }
      height=144;width=176; // Assume Default as QCIF
      printf("Executing DynPortReconfig QCIF 144 x 176 \n");
    }
    else
    {
      int fps = 30;
      switch(file_type_option)
      {
          case FILE_TYPE_DAT_PER_AU:
          case FILE_TYPE_ARBITRARY_BYTES:
          case FILE_TYPE_264_NAL_SIZE_LENGTH:
          case FILE_TYPE_START_CODE:
          case FILE_TYPE_PICTURE_START_CODE:
          case FILE_TYPE_RCV:
          case FILE_TYPE_VC1:
          {
              nalSize = 0;
              if ((file_type_option == FILE_TYPE_264_NAL_SIZE_LENGTH) ||
                  ((codec_format_option == CODEC_FORMAT_H264) && (file_type_option == FILE_TYPE_ARBITRARY_BYTES)))
              {
                printf(" Enter Nal length size [2 or 4] \n");
                if (file_type_option == FILE_TYPE_ARBITRARY_BYTES)
                {
                  printf(" Enter 0 if it is a start code based clip\n");
                }          
                scanf("%d", &nalSize);
                if ((file_type_option == FILE_TYPE_264_NAL_SIZE_LENGTH) &&
                    (nalSize == 0))
                {
                  printf("Error - Can't pass NAL length size = 0\n");
                  return -1;
                } 
              }

              height=144;width=176; // Assume Default as QCIF
              printf("Executing DynPortReconfig QCIF 144 x 176 \n");
              break;
          }

          default:
          {
              printf(" Wrong test case...[%d] \n",file_type_option);
              return -1;
          }
      }

      printf(" *********************************************\n");
      printf(" Output buffer option:\n");
      printf(" *********************************************\n");
      printf(" 0 --> No display and no YUV log\n");
      printf(" 1 --> Diplay YUV\n");
      printf(" 2 --> Take YUV log\n");
      printf(" 3 --> Display YUV and take YUV log\n");
      fflush(stdin);
      scanf("%d", &outputOption);
      fflush(stdin);

      printf(" *********************************************\n");
      printf(" ENTER THE TEST CASE YOU WOULD LIKE TO EXECUTE\n");
      printf(" *********************************************\n");
      printf(" 1 --> Play the clip till the end\n");
      printf(" 2 --> Test OMX_UseBuffer for input buffer\n");
      printf(" 3 --> Run free handle compliance test. Do NOT expect any display for most option. \n");
      printf("       Please only see \"TEST SUCCESSFULL\" to indidcate test pass\n");
      fflush(stdin);
      scanf("%d", &test_option);
      fflush(stdin);

      if (file_type_option != FILE_TYPE_ARBITRARY_BYTES) {
        printf(" *********************************************\n");
        printf(" DO YOU WANT TEST APP TO DISPLAY IT BASED ON FPS (Real time) \n");
        printf(" 0 --> NO\n 1 --> YES\n");
        printf(" Warning: For H264, it require one NAL per frame clip.\n");
        printf("          For Arbitrary bytes option, Real time display is not recommend\n");
        printf(" *********************************************\n");
        fflush(stdin);
        scanf("%d", &realtime_display);
        fflush(stdin);
      }

      if (realtime_display) 
      {
        printf(" *********************************************\n");
        printf(" ENTER THE CLIP FPS\n");
        printf(" Exception: VC1 Simple and Main Profile will be based on the timestamp at RCV file.\n");
        printf(" *********************************************\n");
        fflush(stdin);
        scanf("%d", &fps);
        fflush(stdin);
        timestampInterval = 1000000/fps;
      }
    }

    if (test_option == 2) 
    {
      omx_allocate_buffer_option = false;
    }

    if (outputOption == 0) 
    {
      displayYuv = 0;
      takeYuvLog = 0;
    }
    else if (outputOption == 1) 
    {
      displayYuv = 1;
    }
    else if (outputOption == 2) 
    {
      takeYuvLog = 1;
    }
    else if (outputOption == 3) 
    {
      displayYuv = 1;
      takeYuvLog = 1;
    }
    else
    {
      printf("Wrong option. Assume you want to see the YUV display\n");
      displayYuv = 1;
    } 

    if (test_option == 3) 
    {
      printf(" *********************************************\n");
      printf(" ENTER THE COMPLIANCE TEST YOU WOULD LIKE TO EXECUTE\n");
      printf(" *********************************************\n");
      printf(" 1 --> Call Free Handle at the OMX_StateLoaded\n");
      printf(" 2 --> Call Free Handle at the OMX_StateIdle\n");
      printf(" 3 --> Call Free Handle at the OMX_StateExecuting\n");
      printf(" 4 --> Call Free Handle at the OMX_StatePause\n");
      fflush(stdin);
      scanf("%d", &freeHandle_option);
      fflush(stdin);  
    }
    else 
    {
      freeHandle_option = (freeHandle_test)0;
    }

    printf("Input values: inputfilename[%s]\n", in_filename);
    printf("*******************************************************\n");	
    pthread_cond_init(&eos_cond, 0);
    pthread_mutex_init(&eos_lock, 0);
    pthread_cond_init(&cond, 0);
    pthread_mutex_init(&lock, 0);

    if (0 != pipe (ebd_pipe))
    {
      printf("\n Error in Creating ebd_pipe \n");
      return -1;
    }
    if (0 != pipe(fbd_pipe))
    {
      printf("\n Error in Creating fbd_pipe \n");
      do_freeHandle_and_clean_up(true);
      return -1;
    }
    if (displayYuv && (0 != pipe(fbiopan_pipe)))
    {
       printf("\n Error in Creating fbiopan_pipe \n");
       do_freeHandle_and_clean_up(true);
       return -1;
    }

    if(0 != pthread_create(&fbd_thread_id, NULL, fbd_thread, NULL))
    {
      printf("\n Error in Creating fbd_thread \n");
      do_freeHandle_and_clean_up(true);
      return -1;
    }

    if(0 != pthread_create(&fbiopan_thread_id, NULL, fbiopan_thread, NULL))
    {
      printf("\n Error in Creating fbiopan_thread \n");
      do_freeHandle_and_clean_up(true);
      return -1;
    }

    if (displayYuv) 
    {
      QPERF_RESET(render_fb);
      /* Open frame buffer device */
#ifdef _ANDROID_
      fb_fd = open("/dev/graphics/fb0", O_RDWR);
#else
      fb_fd = open("/dev/fb0", O_RDWR);
#endif
      if (fb_fd < 0) {
        printf("[omx_vdec_test] - ERROR - can't open framebuffer!\n");
        do_freeHandle_and_clean_up(true);
        return -1;
      }
      
      if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        printf("[omx_vdec_test] - ERROR - can't retrieve vscreenInfo!\n");
        do_freeHandle_and_clean_up(true);
        close(fb_fd);
        return -1;
      }
    }

    run_tests();
    if (displayYuv)
    {
      close(fb_fd);
      fb_fd = -1;
      QPERF_SHOW_STATISTIC(render_fb);
    }
    QPERF_SHOW_STATISTIC(client_decode);
    return 0;
}

int run_tests()
{
  int eRet = 0;
  OMX_ERRORTYPE result;
  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Inside %s\n", __FUNCTION__);
  waitForPortSettingsChanged = 1;
  currentStatus = GOOD_STATE;
  fbd_cnt = 0; ebd_cnt=0;
  
  if(file_type_option == FILE_TYPE_DAT_PER_AU) {
    Read_Buffer = Read_Buffer_From_DAT_File;
  }
  else if(file_type_option == FILE_TYPE_ARBITRARY_BYTES) {
    Read_Buffer = Read_Buffer_ArbitraryBytes;
  }
  else if(file_type_option == FILE_TYPE_264_NAL_SIZE_LENGTH) {
    Read_Buffer = Read_Buffer_From_Size_Nal;
  }
  else if(file_type_option == FILE_TYPE_START_CODE) {
    Read_Buffer = Read_Buffer_From_H264_Start_Code_File;
  }
  else if((codec_format_option == CODEC_FORMAT_H263) ||
          (codec_format_option == CODEC_FORMAT_MP4)) {
    Read_Buffer = Read_Buffer_From_Vop_Start_Code_File;
  }
  else if(file_type_option == FILE_TYPE_RCV) {
    Read_Buffer = Read_Buffer_From_RCV_File;
  }
  else if(file_type_option == FILE_TYPE_VC1) {
    Read_Buffer = Read_Buffer_From_VC1_File;
  }
  
  QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"file_type_option %d!\n", file_type_option);
  
  switch(file_type_option)
  {
    case FILE_TYPE_DAT_PER_AU:
    case FILE_TYPE_ARBITRARY_BYTES:
    case FILE_TYPE_264_NAL_SIZE_LENGTH:
    case FILE_TYPE_START_CODE:
    case FILE_TYPE_PICTURE_START_CODE:
    case FILE_TYPE_RCV:
    case FILE_TYPE_VC1:
      if(Init_Decoder()!= 0x00)
      {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"Error - Decoder Init failed\n");
        do_freeHandle_and_clean_up(true);
        return -1;
      }
      eRet = Play_Decoder();
      if(eRet == -1)
      {
        do_freeHandle_and_clean_up(true);
        return -1;
      }
      else if (eRet == 1) 
      {
        do_freeHandle_and_clean_up(false);
        return -1;        
      }
      break; 
    default:
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                    "Error - Invalid Entry...%d\n",file_type_option);
      break;
  }
  
  // Wait till EOS is reached...
  pthread_mutex_lock(&eos_lock);
  while (bOutputEosReached == false) 
  {
    pthread_cond_wait(&eos_cond, &eos_lock);
  }
  pthread_mutex_unlock(&eos_lock);
  if(bOutputEosReached)
  {
    int bufCnt = 0;
    
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Moving the decoder to idle state \n");
    OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StateIdle,0);
    wait_for_event();
    if (currentStatus == INVALID_STATE) 
    {
      do_freeHandle_and_clean_up(true);
      return 0;
    }
   
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Moving the decoder to loaded state \n");
    OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StateLoaded,0);
    
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                 "[OMX Vdec Test] - Deallocating i/p and o/p buffers \n");
    for(bufCnt=0; bufCnt < input_buf_cnt; ++bufCnt) 
    {
      OMX_FreeBuffer(dec_handle, 0, pInputBufHdrs[bufCnt]);
    }
    
    for(bufCnt=0; bufCnt < output_buf_cnt; ++bufCnt) 
    {
      OMX_FreeBuffer(dec_handle, 1, pOutYUVBufHdrs[bufCnt]);
    }
    
    bInputEosReached = false;
    bOutputEosReached = false;
    
    wait_for_event();
    
    freeHandle_CloseAllFilesAndPipes(); 

    printf("*****************************************\n");
    printf("******...TEST SUCCESSFULL...*******\n");
    printf("*****************************************\n");
  }
  return 0;
}

int Init_Decoder()
{
    OMX_ERRORTYPE omxresult;
    OMX_U32 total = 0;
    char vdecCompNames[50];
    typedef OMX_U8* OMX_U8_PTR;
    char *role ="video_decoder";
    static OMX_CALLBACKTYPE call_back = {&EventHandler, &EmptyBufferDone, &FillBufferDone};
    int i = 0;
    long bufCnt = 0;
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Inside %s \n", __FUNCTION__);

    /* Init. the OpenMAX Core */
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nInitializing OpenMAX Core....\n");
    omxresult = OMX_Init();

    if(OMX_ErrorNone != omxresult) {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                     "\n Failed to Init OpenMAX core");
        do_freeHandle_and_clean_up(true);
      	return -1;
    }
    else {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                     "\nOpenMAX Core Init Done\n");
    }
	
    /* Query for video decoders*/
    OMX_GetComponentsOfRole(role, &total, 0);
    QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nTotal components of role=%s :%d", role, total);

    if(total) 
    {
        /* Allocate memory for pointers to component name */
        OMX_U8** vidCompNames = (OMX_U8**)malloc((sizeof(OMX_U8*))*total);

        for (i = 0; i < total; ++i) {
            vidCompNames[i] = (OMX_U8*)malloc(sizeof(OMX_U8)*OMX_MAX_STRINGNAME_SIZE);
        }
        OMX_GetComponentsOfRole(role, &total, vidCompNames);
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nComponents of Role:%s\n", role);
        for (i = 0; i < total; ++i) {
            QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nComponent Name [%s]\n",vidCompNames[i]);
            free(vidCompNames[i]);
        }
        free(vidCompNames);
    }
    else {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                      "No components found with Role:%s", role);
    }

    if (codec_format_option == CODEC_FORMAT_H264) 
    {
      strncpy(vdecCompNames, "OMX.qcom.video.decoder.avc", 27);
    }
    else if (codec_format_option == CODEC_FORMAT_MP4) 
    {
      strncpy(vdecCompNames, "OMX.qcom.video.decoder.mpeg4", 29);
    }
    else if (codec_format_option == CODEC_FORMAT_H263) 
    {
      strncpy(vdecCompNames, "OMX.qcom.video.decoder.h263", 28); 
    }
    else if (codec_format_option == CODEC_FORMAT_VC1) 
    {
      strncpy(vdecCompNames, "OMX.qcom.video.decoder.vc1", 27); 
    }
    else
    {
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                    "Error: Unsupported codec %d\n", codec_format_option);
      return -1;
    }

    omxresult = OMX_GetHandle((OMX_HANDLETYPE*)(&dec_handle), 
                              (OMX_STRING)vdecCompNames, NULL, &call_back);
    if (FAILED(omxresult)) {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                      "\nFailed to Load the component:%s\n", vdecCompNames);
        return -1;
    } 
    else 
    {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nComponent %s is in LOADED state\n", vdecCompNames);
    }

    /* Get the port information */
    CONFIG_VERSION_SIZE(portParam);
    omxresult = OMX_GetParameter(dec_handle, OMX_IndexParamVideoInit, 
                                (OMX_PTR)&portParam);

    if(FAILED(omxresult)) {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                     "ERROR - Failed to get Port Param\n");
        return -1;
    } 
    else 
    {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                      "portParam.nPorts:%d\n", portParam.nPorts);
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                      "portParam.nStartPortNumber:%d\n", portParam.nStartPortNumber);
    }

    /* Set the compression format on i/p port */
    if (codec_format_option == CODEC_FORMAT_H264) 
    {
      portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    }
    else if (codec_format_option == CODEC_FORMAT_MP4) 
    {
      portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
    }
    else if (codec_format_option == CODEC_FORMAT_H263) 
    {
      portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
    }
    else if (codec_format_option == CODEC_FORMAT_VC1) 
    {
      portFmt.format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
    }
    else
    {
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                    "Error: Unsupported codec %d\n", codec_format_option);
    }
    
	
    return 0;
}

int Play_Decoder()
{
    int i, bufCnt;
    int frameSize=0;
    OMX_QCOM_PARAM_PORTDEFINITIONTYPE inputPortFmt;
    OMX_ERRORTYPE ret;
	
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Inside %s \n", __FUNCTION__);
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"sizeof[%d]\n", sizeof(OMX_BUFFERHEADERTYPE));
	
    /* open the i/p and o/p files based on the video file format passed */
    if(open_video_file()) {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                     "Error in opening video file\n");
        return -1;
    } 	

    memset(&inputPortFmt, 0, sizeof(OMX_QCOM_PARAM_PORTDEFINITIONTYPE));
    CONFIG_VERSION_SIZE(inputPortFmt);
    inputPortFmt.nPortIndex = 0;  // input port
    switch (file_type_option)
    {
      case FILE_TYPE_DAT_PER_AU:
      case FILE_TYPE_PICTURE_START_CODE:
      case FILE_TYPE_RCV:
      case FILE_TYPE_VC1:
      {
        inputPortFmt.nFramePackingFormat = OMX_QCOM_FramePacking_OnlyOneCompleteFrame;
        break;
      }

      case FILE_TYPE_ARBITRARY_BYTES:
      {
        inputPortFmt.nFramePackingFormat = OMX_QCOM_FramePacking_Arbitrary;
        break;
      }

      case FILE_TYPE_264_NAL_SIZE_LENGTH:
      {
        inputPortFmt.nFramePackingFormat = OMX_QCOM_FramePacking_OnlyOneCompleteSubFrame;
        break;
      }

      default:
        inputPortFmt.nFramePackingFormat = OMX_QCOM_FramePacking_Unspecified;
    } 
    /*OMX_SetParameter(dec_handle,(OMX_INDEXTYPE)OMX_QcomIndexPortDefn,
                     (OMX_PTR)&inputPortFmt);
    */
    /* Query the decoder outport's min buf requirements */
    CONFIG_VERSION_SIZE(portFmt);

    /* Port for which the Client needs to obtain info */
    portFmt.nPortIndex = portParam.nStartPortNumber; 

    OMX_GetParameter(dec_handle,OMX_IndexParamPortDefinition,&portFmt);
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nDec: Min Buffer Count %d\n", portFmt.nBufferCountMin);
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nDec: Buffer Size %d\n", portFmt.nBufferSize);

    if(OMX_DirInput != portFmt.eDir) {
        printf ("\nDec: Expect Input Port\n");
        return -1;
    }

    bufCnt = 0;
    portFmt.format.video.nFrameHeight = height;
    portFmt.format.video.nFrameWidth  = width;
    OMX_SetParameter(dec_handle,OMX_IndexParamPortDefinition,
                                                       (OMX_PTR)&portFmt); 
    OMX_GetParameter(dec_handle,OMX_IndexParamPortDefinition,
                                                               &portFmt);
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nDec: New Min Buffer Count %d", portFmt.nBufferCountMin);


    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nVideo format, height = %d", portFmt.format.video.nFrameHeight);
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nVideo format, height = %d\n", portFmt.format.video.nFrameWidth);
    if ((codec_format_option == CODEC_FORMAT_H264) && (file_type_option == FILE_TYPE_264_NAL_SIZE_LENGTH))
    {
        OMX_VIDEO_CONFIG_NALSIZE naluSize;
        naluSize.nNaluBytes = nalSize;
        OMX_SetConfig(dec_handle,OMX_IndexConfigVideoNalSize,(OMX_PTR)&naluSize);
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"SETTING THE NAL SIZE to %d\n",naluSize.nNaluBytes);
    }
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nOMX_SendCommand Decoder -> IDLE\n");
    OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StateIdle,0);

    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Transition to Idle State succesful...\n");

    /* Allocate buffer on decoder's i/p port */
    if (omx_allocate_buffer_option)
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Using OMX_AllocateBuffer on decoder input port\n");
      error = Allocate_Buffer(dec_handle, &pInputBufHdrs, portFmt.nPortIndex, 
                              portFmt.nBufferCountMin, portFmt.nBufferSize);
    }
    else
    {
      OMX_U8* input_use_buffer = NULL;
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Using OMX_UseBuffer on decoder input port\n");
      input_use_buffer = (OMX_U8*)malloc(portFmt.nBufferCountMin * portFmt.nBufferSize);
      // input_use_buffer will be freed at OMX_Deinit

      if (input_use_buffer == NULL) 
      {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                     "Error - Allocating input buffer is failed\n");
        return -1;
      }
      error = Use_Buffer(dec_handle, &pInputBufHdrs, portFmt.nPortIndex, 
                         portFmt.nBufferCountMin, portFmt.nBufferSize,
                         input_use_buffer); 
    }

    if (error != OMX_ErrorNone) 
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                   "Error - OMX_AllocateBuffer or OMX_UseBuffer for the Input buffer is failed\n");
      return -1;
    } 
    else
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\nOMX_AllocateBuffer or OMX_UseBuffer for the Input buffer success\n");	
      input_buf_cnt = portFmt.nBufferCountMin;
    }

    portFmt.nPortIndex = portParam.nStartPortNumber+1; 
    /* Port for which the Client needs to obtain info */

    OMX_GetParameter(dec_handle,OMX_IndexParamPortDefinition,&portFmt);
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"nMin Buffer Count=%d", portFmt.nBufferCountMin);
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"nBuffer Size=%d", portFmt.nBufferSize);
    if(OMX_DirOutput != portFmt.eDir) {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                     "Error - Expect Output Port\n");
        return -1;
    }

    /* Allocate buffer on decoder's o/p port */
    error = Allocate_Buffer(dec_handle, &pOutYUVBufHdrs, portFmt.nPortIndex,
                            portFmt.nBufferCountMin, portFmt.nBufferSize); 
    if (error != OMX_ErrorNone) {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                     "Error - OMX_AllocateBuffer Output buffer error\n");
        return -1;
    } 
    else 
    {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"OMX_AllocateBuffer Output buffer success\n");
        output_buf_cnt = portFmt.nBufferCountMin;
    }

    wait_for_event();
    if (currentStatus == INVALID_STATE) 
    {
      return -1;
    }

    if (freeHandle_option == FREE_HANDLE_AT_IDLE) 
    {
      OMX_STATETYPE state = OMX_StateInvalid;
      OMX_GetState(dec_handle, &state);
      if (state == OMX_StateIdle) 
      {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                   "Decoder is in OMX_StateIdle and trying to call OMX_FreeHandle \n");
        return 1;
      }
      else
      {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                      "Error - Decoder is in state %d and trying to call OMX_FreeHandle \n", state);
        return -1;
      }
    }


    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                 "OMX_SendCommand Decoder -> Executing\n");
    OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StateExecuting,0);
    wait_for_event();
    if (currentStatus == INVALID_STATE) 
    {
      return -1;
    }

    for(bufCnt=0; bufCnt < portFmt.nBufferCountMin; ++bufCnt) {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                      "OMX_FillThisBuffer on output buf no.%d\n",bufCnt);
        pOutYUVBufHdrs[bufCnt]->nOutputPortIndex = 1;
        pOutYUVBufHdrs[bufCnt]->nFlags &= ~OMX_BUFFERFLAG_EOS;
        ret = OMX_FillThisBuffer(dec_handle, pOutYUVBufHdrs[bufCnt]);
        if (OMX_ErrorNone != ret) {
            QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                          "Error - OMX_FillThisBuffer failed with result %d\n", ret);
        } 
        else {
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                         "OMX_FillThisBuffer success!\n");
        }
    }

    rcv_v1 = 0;

    QPERF_START(client_decode);
    if ((codec_format_option == CODEC_FORMAT_VC1) && (file_type_option == FILE_TYPE_RCV)) 
    {
      pInputBufHdrs[0]->nOffset = 0;
      frameSize = Read_Buffer_From_RCV_File_Seq_Layer(pInputBufHdrs[0]);
      pInputBufHdrs[0]->nFilledLen = frameSize;
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                    "After Read_Buffer_From_RCV_File_Seq_Layer frameSize %d\n", frameSize);
      
      pInputBufHdrs[0]->nOffset = pInputBufHdrs[0]->nFilledLen;
      frameSize = Read_Buffer(pInputBufHdrs[0]);
      pInputBufHdrs[0]->nFilledLen += frameSize;
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                   "After Read_Buffer frameSize %d\n", frameSize);

      pInputBufHdrs[0]->nInputPortIndex = 0;
      pInputBufHdrs[0]->nOffset = 0;
      pInputBufHdrs[0]->nFlags = 0;
      //pBufHdr[bufCnt]->pAppPrivate = this;
      ret = OMX_EmptyThisBuffer(dec_handle, pInputBufHdrs[0]);
      if (OMX_ErrorNone != ret) {
          QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                        "ERROR - OMX_EmptyThisBuffer failed with result %d\n", ret);
          return -1;
      } 
      else {
          QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                       "OMX_EmptyThisBuffer success!\n");
      }
      i = 1;
    }
    else 
    {
      i = 0;
    }

    for (i; i < input_buf_cnt; i++) {
      pInputBufHdrs[i]->nInputPortIndex = 0;
      pInputBufHdrs[i]->nOffset = 0;
      if((frameSize = Read_Buffer(pInputBufHdrs[i])) <= 0 ){
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,"NO FRAME READ\n");
        pInputBufHdrs[i]->nFilledLen = frameSize;
        pInputBufHdrs[i]->nInputPortIndex = 0;
        pInputBufHdrs[i]->nFlags |= OMX_BUFFERFLAG_EOS;;
        bInputEosReached = true;
        
        OMX_EmptyThisBuffer(dec_handle, pInputBufHdrs[i]);
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_HIGH,
                 "File is small::Either EOS or Some Error while reading file\n");
        break;
      }
      pInputBufHdrs[i]->nFilledLen = frameSize;
      pInputBufHdrs[i]->nInputPortIndex = 0;
      pInputBufHdrs[i]->nFlags = 0;
//pBufHdr[bufCnt]->pAppPrivate = this;
      ret = OMX_EmptyThisBuffer(dec_handle, pInputBufHdrs[i]);
      if (OMX_ErrorNone != ret) {
          QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                        "ERROR - OMX_EmptyThisBuffer failed with result %d\n", ret);
          return -1;
      } 
      else {
          QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                       "OMX_EmptyThisBuffer success!\n");
      }
    }

    if(0 != pthread_create(&ebd_thread_id, NULL, ebd_thread, NULL))
    {
      printf("\n Error in Creating fbd_thread \n");
      return -1;
    }

    // wait for event port settings changed event
    wait_for_event();
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                  "RECIEVED EVENT PORT TO DETERMINE IF DYN PORT RECONFIGURATION NEEDED, currentStatus %d\n", 
                  currentStatus);
    if (currentStatus == INVALID_STATE) 
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                   "Error - INVALID_STATE\n");
      return -1;
    }
    else if (currentStatus == PORT_SETTING_CHANGE_STATE) 
    {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                     "PORT_SETTING_CHANGE_STATE\n");
        // Send DISABLE command
        sent_disabled = 1;
        OMX_SendCommand(dec_handle, OMX_CommandPortDisable, 1, 0);
       
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"FREEING BUFFERS\n");
        // Free output Buffer 
        for(bufCnt=0; bufCnt < portFmt.nBufferCountMin; ++bufCnt) {
            OMX_FreeBuffer(dec_handle, 1, pOutYUVBufHdrs[bufCnt]);
        }
        
        // wait for Disable event to come back
        wait_for_event();
        if (currentStatus == INVALID_STATE) 
        {
          return -1;
        }
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"DISABLE EVENT RECD\n");
        // GetParam and SetParam
    
        // Send Enable command
        OMX_SendCommand(dec_handle, OMX_CommandPortEnable, 1, 0);
        // AllocateBuffers
        /* Allocate buffer on decoder's o/p port */
    
        portFmt.nPortIndex = 1;  
        /* Port for which the Client needs to obtain info */
    
        OMX_GetParameter(dec_handle,OMX_IndexParamPortDefinition,&portFmt);
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Min Buffer Count=%d", portFmt.nBufferCountMin);
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Buffer Size=%d", portFmt.nBufferSize);
        if(OMX_DirOutput != portFmt.eDir) {
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                         "Error - Expect Output Port\n");
            return -1;
        }
    
        error = Allocate_Buffer(dec_handle, &pOutYUVBufHdrs, portFmt.nPortIndex,
                                portFmt.nBufferCountActual, portFmt.nBufferSize); 
        if (error != OMX_ErrorNone) {
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                         "Error - OMX_AllocateBuffer Output buffer error\n");
            return -1;
        } 
        else 
        {
            QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                         "OMX_AllocateBuffer Output buffer success\n");
        }

        // wait for enable event to come back
        wait_for_event();
        if (currentStatus == INVALID_STATE) 
        {
          return -1;
        }
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"ENABLE EVENT HANDLER RECD\n");

        for(bufCnt=0; bufCnt < portFmt.nBufferCountMin; ++bufCnt) {
            QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"OMX_FillThisBuffer on output buf no.%d\n",bufCnt);
            pOutYUVBufHdrs[bufCnt]->nOutputPortIndex = 1;
            pOutYUVBufHdrs[bufCnt]->nFlags &= ~OMX_BUFFERFLAG_EOS;
            ret = OMX_FillThisBuffer(dec_handle, pOutYUVBufHdrs[bufCnt]);
            if (OMX_ErrorNone != ret) {
                QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                              "ERROR - OMX_FillThisBuffer failed with result %d\n", ret);
            } 
            else {
                QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"OMX_FillThisBuffer success!\n");
            }
        }
    }

    if (freeHandle_option == FREE_HANDLE_AT_EXECUTING) 
    {
      OMX_STATETYPE state = OMX_StateInvalid;
      OMX_GetState(dec_handle, &state);
      if (state == OMX_StateExecuting) 
      {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                   "Decoder is in OMX_StateExecuting and trying to call OMX_FreeHandle \n");
        return 1;
      }
      else
      {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                      "Error - Decoder is in state %d and trying to call OMX_FreeHandle \n", state);
        return -1;
      }
    }
    else if (freeHandle_option == FREE_HANDLE_AT_PAUSE) 
    {
      OMX_STATETYPE state = OMX_StateInvalid;
      OMX_SendCommand(dec_handle, OMX_CommandStateSet, OMX_StatePause,0);
      wait_for_event();

      OMX_GetState(dec_handle, &state);
      if (state == OMX_StatePause) 
      {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                   "Decoder is in OMX_StatePause and trying to call OMX_FreeHandle \n");
        return 1;
      }
      else
      {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                      "Error - Decoder is in state %d and trying to call OMX_FreeHandle \n", state);
        return -1;
      }
    }

    return 0;
}

static OMX_ERRORTYPE Allocate_Buffer ( OMX_COMPONENTTYPE *dec_handle, 
                                       OMX_BUFFERHEADERTYPE  ***pBufHdrs, 
                                       OMX_U32 nPortIndex, 
                                       long bufCntMin, long bufSize) 
{
     OMX_ERRORTYPE error=OMX_ErrorNone;
    long bufCnt=0;

    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Inside %s \n", __FUNCTION__);
    QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"pBufHdrs = %x,bufCntMin = %d\n", pBufHdrs, bufCntMin);
    *pBufHdrs= (OMX_BUFFERHEADERTYPE **) 
                   malloc(sizeof(OMX_BUFFERHEADERTYPE)*bufCntMin);

    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"OMX_AllocateBuffer No %d \n", bufCnt);
        error = OMX_AllocateBuffer(dec_handle, &((*pBufHdrs)[bufCnt]), 
                                   nPortIndex, NULL, bufSize);
    }

    return error;
}

static OMX_ERRORTYPE Use_Buffer( OMX_COMPONENTTYPE     *avc_dec_handle, 
                                 OMX_BUFFERHEADERTYPE  ***pBufHdrs, 
                                 OMX_U32               nPortIndex,
                                 OMX_U32               bufCntMin,
                                 OMX_U32               bufSize,
                                 OMX_U8                *buffer)
{
    
    OMX_ERRORTYPE error=OMX_ErrorNone;
    long bufCnt=0;
    *pBufHdrs= (OMX_BUFFERHEADERTYPE **)
                   malloc(sizeof(OMX_BUFFERHEADERTYPE)*bufCntMin);
    DEBUG_PRINT("Inside %s \n", __FUNCTION__);
    DEBUG_PRINT("**************************[0x%x][0x%x]\n",pBufHdrs,*pBufHdrs);
    for(bufCnt=0; bufCnt < bufCntMin; ++bufCnt) 
    {
        DEBUG_PRINT("\n OMX_UseBuffer No %d bufSize=0x%x buffer[0x%x]\n",bufCnt,bufSize,buffer);
        error = OMX_UseBuffer(avc_dec_handle, &((*pBufHdrs)[bufCnt]), 
                              nPortIndex, NULL, 
                              bufSize,buffer);
        buffer = buffer + bufSize;
    }
    DEBUG_PRINT("*********************************\n");
    return error;
}

static void do_freeHandle_and_clean_up(bool isDueToError)
{
    int bufCnt = 0;

    for(bufCnt=0; bufCnt < input_buf_cnt; ++bufCnt) 
    {
        OMX_FreeBuffer(dec_handle, 0, pInputBufHdrs[bufCnt]);
    }

    for(bufCnt=0; bufCnt < output_buf_cnt; ++bufCnt) 
    {
        OMX_FreeBuffer(dec_handle, 1, pOutYUVBufHdrs[bufCnt]);
    }

    freeHandle_CloseAllFilesAndPipes();

    printf("*****************************************\n");
    if (isDueToError) 
    {
      printf("************...TEST FAILED...************\n");
    }
    else
    {
      printf("**********...TEST SUCCESSFULL...*********\n");
    }
    printf("*****************************************\n");  
}

static void freeHandle_CloseAllFilesAndPipes()
{
    OMX_ERRORTYPE result = OMX_ErrorNone;
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"[OMX Vdec Test] - Free handle decoder\n");
    if (dec_handle) {
      result = OMX_FreeHandle(dec_handle);
    }
    
    if (result != OMX_ErrorNone) 
    {
       QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                     "[OMX Vdec Test] - OMX_FreeHandle error. Error code: %d\n", result);
    }
    dec_handle = NULL;

    /* Deinit OpenMAX */
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"[OMX Vdec Test] - De-initializing OMX \n");
    OMX_Deinit();

    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"[OMX Vdec Test] - closing all files and pipes\n");
    if (inputBufferFile)
      fclose(inputBufferFile);

    if (takeYuvLog)
        fclose(outputBufferFile);

    if(ebd_pipe[0])
      close(ebd_pipe[0]);
    if(ebd_pipe[1])
      close(ebd_pipe[1]);
    
    if(fbd_pipe[0]) {
      close(fbd_pipe[0]);
    }
    if(fbd_pipe[1]) {
      close(fbd_pipe[1]);
    }
    if(fbiopan_pipe[0])
      close(fbiopan_pipe[0]);
    if(fbiopan_pipe[1])
      close(fbiopan_pipe[1]);

    pthread_join(fbd_thread_id, NULL);
    pthread_join(ebd_thread_id, NULL);
    pthread_join(fbiopan_thread_id, NULL);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&eos_cond);
    pthread_mutex_destroy(&eos_lock);
}

static int Read_Buffer_From_DAT_File(OMX_BUFFERHEADERTYPE  *pBufHdr) 
{
    long frameSize=0;
    char temp_buffer[10];
    char temp_byte;
    int bytes_read=0;
    int i=0;
    unsigned char *read_buffer=NULL;
    char c = '1'; //initialize to anything except '\0'(0)
    char inputFrameSize[10];
    int count =0; char cnt =0; 
    memset(temp_buffer, 0, sizeof(temp_buffer));
	  
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                  "Inside %s \n", __FUNCTION__);

    while (cnt < 10) 
    /* Check the input file format, may result in infinite loop */
    {
        QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"loop[%d] count[%d]\n",cnt,count);
        count  = fread(&inputFrameSize[cnt], 1, 1, inputBufferFile); 
        if(inputFrameSize[cnt] == '\0' )
          break;
        cnt++;
    }
    inputFrameSize[cnt]='\0';
    frameSize = atoi(inputFrameSize);
    pBufHdr->nFilledLen = 0;	

    /* get the frame length */
    fseek(inputBufferFile, -1, SEEK_CUR);
    bytes_read = fread(pBufHdr->pBuffer, 1, frameSize,  inputBufferFile);

    QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                  "Actual frame Size [%d] bytes_read using fread[%d]\n", 
                  frameSize, bytes_read);

    if(bytes_read == 0 || bytes_read < frameSize ) {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                     "Bytes read Zero After Read frame Size \n");
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                      "Checking VideoPlayback Count:video_playback_count is:%d\n",
                       video_playback_count);
        return 0;
    }
    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;
    return bytes_read;
}

static int Read_Buffer_ArbitraryBytes(OMX_BUFFERHEADERTYPE  *pBufHdr)
{
    char temp_buffer[10];
    char temp_byte;
    int bytes_read=0;
    int i=0;
    unsigned char *read_buffer=NULL;
    char c = '1'; //initialize to anything except '\0'(0)
    char inputFrameSize[10];
    int count =0; char cnt =0; 
    memset(temp_buffer, 0, sizeof(temp_buffer));
	  
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Inside %s \n", __FUNCTION__);

    bytes_read = fread(pBufHdr->pBuffer, 1, NUMBER_OF_ARBITRARYBYTES_READ,  inputBufferFile);

    if(bytes_read == 0) {
        QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                     "Bytes read Zero After Read frame Size \n");
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                      "Checking VideoPlayback Count:video_playback_count is:%d\n",
                      video_playback_count);
        return 0;
    }
    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;
    return bytes_read;
}

static int Read_Buffer_From_H264_Start_Code_File(OMX_BUFFERHEADERTYPE  *pBufHdr) 
{
    unsigned int readOffset = 0;
    int bytes_read = 0;
    unsigned int code = 0;

    pBufHdr->nFilledLen = 0;
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Inside %s \n", __FUNCTION__);

    do
    {
      //Start codes are always byte aligned.
      bytes_read = fread(&pBufHdr->pBuffer[readOffset],1, 1,inputBufferFile);
      if(!bytes_read)
      {
          QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Bytes read Zero \n");
          break;
      }
      code <<= 8;
      code |= (0x000000FF & pBufHdr->pBuffer[readOffset]);
      //VOP start code comparision
      if ((readOffset>3) && ( H264_START_CODE == code))
      {
        //Seek backwards by 4
        fseek(inputBufferFile, -4, SEEK_CUR);
        readOffset-=3;
        break;
      }
      readOffset++;
    }while (1);
    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;
    return readOffset;
}

static int Read_Buffer_From_Vop_Start_Code_File(OMX_BUFFERHEADERTYPE  *pBufHdr) 
{
    unsigned int readOffset = 0;
    int bytes_read = 0;
    unsigned int code = 0;
    static unsigned int header_code = 0;

    pBufHdr->nFilledLen = 0;
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Inside %s \n", __FUNCTION__);

    do
    {
      //Start codes are always byte aligned.
      bytes_read = fread(&pBufHdr->pBuffer[readOffset],1, 1,inputBufferFile);
      if(!bytes_read)
      {
          QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Bytes read Zero \n");
          break;
      }
      code <<= 8;
      code |= (0x000000FF & pBufHdr->pBuffer[readOffset]);
      //VOP start code comparision
      if (readOffset>3)
      {
        if(!header_code ){
          if( VOP_START_CODE == code)
          {
            header_code = VOP_START_CODE;
          }
          else if ( (0xFFFFFC00 & code) == SHORT_HEADER_START_CODE )
          {
            header_code = SHORT_HEADER_START_CODE;
          }
        }
        if ((header_code == VOP_START_CODE) && (code == VOP_START_CODE))
        {
          //Seek backwards by 4
          fseek(inputBufferFile, -4, SEEK_CUR);
          readOffset-=3;
          break;
      
        }
        else if (( header_code == SHORT_HEADER_START_CODE ) && ( SHORT_HEADER_START_CODE == (code & 0xFFFFFC00)))
        {
          //Seek backwards by 4
          fseek(inputBufferFile, -4, SEEK_CUR);
          readOffset-=3;
          break;
        }
      }
      readOffset++;
    }while (1);
    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;
    return readOffset;
}

static int Read_Buffer_From_Size_Nal(OMX_BUFFERHEADERTYPE  *pBufHdr) 
{
    // NAL unit stream processing
    char temp_size[SIZE_NAL_FIELD_MAX];
    int i = 0;
    int j = 0;
    unsigned int size = 0;   // Need to make sure that unsigned long int has SIZE_NAL_FIELD_MAX (4) bytes
    int bytes_read = 0;

    // read the "size_nal_field"-byte size field
    bytes_read = fread(pBufHdr->pBuffer + pBufHdr->nOffset, 1, nalSize, inputBufferFile);
    if (bytes_read == 0) 
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_MED, "Failed to read frame or it might be EOF\n");
      return 0;
    } 

    for (i=0; i<SIZE_NAL_FIELD_MAX-nalSize; i++)       
    {
      temp_size[SIZE_NAL_FIELD_MAX - 1 - i] = 0;
    }

    /* Due to little endiannes, Reorder the size based on size_nal_field */
    for (j=0; i<SIZE_NAL_FIELD_MAX; i++, j++) 
    {
      temp_size[SIZE_NAL_FIELD_MAX - 1 - i] = pBufHdr->pBuffer[pBufHdr->nOffset + j];
    }
    size = (unsigned int)(*((unsigned int *)(temp_size)));

    // now read the data
    bytes_read = fread(pBufHdr->pBuffer + pBufHdr->nOffset + nalSize, 1, size, inputBufferFile);
    if (bytes_read != size) 
    {
      QTV_MSG_PRIO(QTVDIAG_GENERAL, QTVDIAG_PRIO_ERROR, "Failed to read frame\n");
    }

    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;

    return bytes_read + nalSize;
}

static int Read_Buffer_From_RCV_File_Seq_Layer(OMX_BUFFERHEADERTYPE  *pBufHdr) 
{
    unsigned int readOffset = 0, size_struct_C = 0;
    unsigned int startcode = 0;
    pBufHdr->nFilledLen = 0;
    pBufHdr->nFlags = 0;

    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Inside %s \n", __FUNCTION__);

    fread(&startcode, 4, 1, inputBufferFile);

    /* read size of struct C as it need not be 4 always*/
    fread(&size_struct_C, 1, 4, inputBufferFile);

    /* reseek to beginning of sequence header */
    fseek(inputBufferFile, -8, SEEK_CUR);

    if ((startcode & 0xFF000000) == 0xC5000000) 
    {
      
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                    "Read_Buffer_From_RCV_File_Seq_Layer size_struct_C: %d\n", size_struct_C);

      if (struct_c_only)
      {
        OMX_PARAM_PORTDEFINITIONTYPE inputPortFmt;
        // STRUCT_C only
        fseek(inputBufferFile, 8, SEEK_CUR);
        readOffset = fread(pBufHdr->pBuffer, 1, 4, inputBufferFile);
        fread(&height, 1, 4, inputBufferFile);
        fread(&width, 1, 4, inputBufferFile);

        DEBUG_PRINT("Sequence layer height %d and width %d\n", height, width);
        inputPortFmt.nPortIndex = 0;
        OMX_GetParameter(dec_handle,OMX_IndexParamPortDefinition,&inputPortFmt);

        inputPortFmt.format.video.nFrameHeight = height;
        inputPortFmt.format.video.nFrameWidth  = width;
        OMX_SetParameter(dec_handle,OMX_IndexParamPortDefinition,(OMX_PTR)&inputPortFmt); 
        fseek(inputBufferFile, 16, SEEK_CUR);
      }
      else
      {
        // .RVC file
        readOffset = fread(pBufHdr->pBuffer, 1, VC1_SEQ_LAYER_SIZE_WITHOUT_STRUCTC + size_struct_C, inputBufferFile);
      }      
    }
    else if((startcode & 0xFF000000) == 0x85000000)
    {
      // .RCV V1 file

      rcv_v1 = 1;
      
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                    "Read_Buffer_From_RCV_File_Seq_Layer size_struct_C: %d\n", size_struct_C);
      
      readOffset = fread(pBufHdr->pBuffer, 1, VC1_SEQ_LAYER_SIZE_V1_WITHOUT_STRUCTC + size_struct_C, inputBufferFile);
    }
    else
    {
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                    "Error: Unknown VC1 clip format %x\n", startcode);
    }

#if 0
    {
      int i=0;
      printf("Read_Buffer_From_RCV_File, length %d readOffset %d\n", readOffset, readOffset);
      for (i=0; i<36; i++)
      {  
        printf("0x%.2x ", pBufHdr->pBuffer[i]);
        if (i%16 == 15) {
          printf("\n");
        }
      }
      printf("\n");
    }
#endif
    return readOffset;
}

static int Read_Buffer_From_RCV_File(OMX_BUFFERHEADERTYPE  *pBufHdr) 
{
    unsigned int readOffset = 0;
    unsigned int len = 0;
    unsigned int key = 0;
    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Inside %s \n", __FUNCTION__);

    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                  "Read_Buffer_From_RCV_File - nOffset %d\n", pBufHdr->nOffset);
    if(rcv_v1)
    {
      /* for the case of RCV V1 format, the frame header is only of 4 bytes and has
         only the frame size information */
      readOffset = fread(&len, 1, 4, inputBufferFile);
      QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                    "Read_Buffer_From_RCV_File - framesize %d %x\n", len, len);

    }
    else
    {
      /* for a regular RCV file, 3 bytes comprise the frame size and 1 byte for key*/
      readOffset = fread(&len, 1, 3, inputBufferFile);
      QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                  "Read_Buffer_From_RCV_File - framesize %d %x\n", len, len);

      readOffset = fread(&key, 1, 1, inputBufferFile);
      if ( (key & 0x80) == false) 
      {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                    "Read_Buffer_From_RCV_File - Non IDR frame key %x\n", key);
       }

    }

    if(!rcv_v1)
    {  
      /* There is timestamp field only for regular RCV format and not for RCV V1 format*/
      readOffset = fread(&pBufHdr->nTimeStamp, 1, 4, inputBufferFile);
      QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                  "Read_Buffer_From_RCV_File - timeStamp %d\n", pBufHdr->nTimeStamp);
    }
    
    readOffset = fread(pBufHdr->pBuffer+pBufHdr->nOffset, 1, len, inputBufferFile);
    if (readOffset != len) 
    {
      QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                    "EOS reach or Reading error %d, %s \n", readOffset, strerror( errno )); 
      return 0;
    }

#if 0
    {
      int i=0;
      printf("Read_Buffer_From_RCV_File, length %d readOffset %d\n", len, readOffset);
      for (i=0; i<64; i++)
      {  
        printf("0x%.2x ", pBufHdr->pBuffer[i]);
        if (i%16 == 15) {
          printf("\n");
        }
      }
      printf("\n");
    }
#endif

    if (realtime_display) 
    {
      pBufHdr->nTimeStamp = timeStampLfile;
      timeStampLfile += timestampInterval;
    }

    return readOffset;
}

static int Read_Buffer_From_VC1_File(OMX_BUFFERHEADERTYPE  *pBufHdr) 
{
    static int timeStampLfile = 0;
    unsigned int readOffset = 0;
    int bytes_read = 0;
    unsigned int code = 0;
    pBufHdr->nFilledLen = 0;	

    QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"Inside %s \n", __FUNCTION__);

    do
    {
      //Start codes are always byte aligned.
      bytes_read = fread(&pBufHdr->pBuffer[readOffset],1, 1,inputBufferFile);
      if(!bytes_read)
      {
          QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"\n Bytes read Zero \n");
          break;
      }
      code <<= 8;
      code |= (0x000000FF & pBufHdr->pBuffer[readOffset]);
      //VOP start code comparision
      if (readOffset>3)
      {
        if (VC1_START_CODE == (code & 0xFFFFFF00))
        {
          //Seek backwards by 4
          fseek(inputBufferFile, -4, SEEK_CUR);
          readOffset-=3;

          while(pBufHdr->pBuffer[readOffset-1] == 0)
            readOffset--;

          break;
        }
      }
      readOffset++;
    }while (1);

    pBufHdr->nTimeStamp = timeStampLfile;
    timeStampLfile += timestampInterval;

#if 0
    {
      int i=0;
      printf("Read_Buffer_From_VC1_File, readOffset %d\n", readOffset);
      for (i=0; i<64; i++)
      {  
        printf("0x%.2x ", pBufHdr->pBuffer[i]);
        if (i%16 == 15) {
          printf("\n");
        }
      }
      printf("\n");
    }
#endif

    return readOffset;
}

static int open_video_file ()
{
    int error_code = 0;
    char outputfilename[512];
    QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                  "Inside %s filename=%s\n", __FUNCTION__, in_filename);

    inputBufferFile = fopen (in_filename, "rb");
    if (inputBufferFile == NULL) {
        QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                      "Error - i/p file %s could NOT be opened\n",
		                  in_filename);
        error_code = -1;
    } 
    else {
      	QTV_MSG_PRIO1(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,"I/p file %s is opened \n", in_filename);
    }

    if (takeYuvLog && (error_code == 0)) {
        char *p_outputfilename = outputfilename;

        strncpy(p_outputfilename, in_filename, strlen(in_filename)+1);
        p_outputfilename = strrchr(outputfilename, '.');
        if (p_outputfilename == NULL) 
        {
          p_outputfilename = outputfilename + strlen(outputfilename);
        }
        strncpy(p_outputfilename, ".yuv", 5);

        outputBufferFile = fopen (outputfilename, "ab");
        if (outputBufferFile == NULL) 
        {
          printf("ERROR - o/p file %s could NOT be opened\n", outputfilename);
          error_code = -1;
        } 
        else 
        {
          printf("O/p file %s is opened \n", outputfilename);
        }
    }
    return error_code;
}

void swap_byte(char *pByte, int nbyte)
{
  int i=0;

  for (i=0; i<nbyte/2; i++)
  {
    pByte[i] ^= pByte[nbyte-i-1];
    pByte[nbyte-i-1] ^= pByte[i];
    pByte[i] ^= pByte[nbyte-i-1];
  }
}

void render_fb(struct OMX_BUFFERHEADERTYPE *pBufHdr)
{
  OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *pPMEMInfo = NULL;
#ifdef _ANDROID_
  MemoryHeapBase *vheap = NULL;
#endif
    
  struct mdp_blit_req *e;
  union {
    char dummy[sizeof(struct mdp_blit_req_list) +
               sizeof(struct mdp_blit_req) * 1];
    struct mdp_blit_req_list list;
  } img;

  if (fb_fd < 0)
  {
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,
                 "Warning: /dev/fb0 is not opened!\n");
    return;
  }

  img.list.count = 1;
  e = &img.list.req[0];

  pPMEMInfo  = (OMX_QCOM_PLATFORM_PRIVATE_PMEM_INFO *)
                 ((OMX_QCOM_PLATFORM_PRIVATE_LIST *)
                    pBufHdr->pPlatformPrivate)->entryList->entry;
#ifdef _ANDROID_
  vheap = (MemoryHeapBase *)pPMEMInfo->pmem_fd;
#endif

  QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                "DecWidth %d DecHeight %d\n",portFmt.format.video.nStride,portFmt.format.video.nSliceHeight);
  QTV_MSG_PRIO2(QTVDIAG_GENERAL,QTVDIAG_PRIO_MED,
                "DispWidth %d DispHeight %d\n",portFmt.format.video.nFrameWidth,portFmt.format.video.nFrameHeight);
      
  e->src.width = portFmt.format.video.nStride;
  e->src.height = portFmt.format.video.nSliceHeight;
  e->src.format = MDP_Y_CBCR_H2V2;
  e->src.offset = pPMEMInfo->offset;
#ifdef _ANDROID_
  e->src.memory_id = vheap->getHeapID();
#else
  e->src.memory_id = pPMEMInfo->pmem_fd;
#endif

  DEBUG_PRINT("pmemOffset %d pmemID %d\n",e->src.offset,e->src.memory_id);

  e->dst.width = vinfo.xres;
  e->dst.height = vinfo.yres;
  e->dst.format = MDP_RGB_565;
  e->dst.offset = 0;
  e->dst.memory_id = fb_fd;
  
  e->transp_mask = 0xffffffff;
  e->flags = 0;
  e->alpha = 0xff;
  
  e->dst_rect.x = 0;
  e->dst_rect.y = 0;
  e->dst_rect.w = portFmt.format.video.nFrameWidth;
  e->dst_rect.h = portFmt.format.video.nFrameHeight; 

  //e->dst_rect.w = 800;
  //e->dst_rect.h = 480;
  
  e->src_rect.x = 0;
  e->src_rect.y = 0;
  e->src_rect.w = portFmt.format.video.nFrameWidth;
  e->src_rect.h = portFmt.format.video.nFrameHeight;

  DEBUG_PRINT("\n\n display_frame(): Stride %d SliceHeight %d\n\n",e->src.width, e->src.height );
  DEBUG_PRINT("\n\n display_frame(): Width %d  Height %d\n\n",e->src_rect.w, e->src_rect.h );

  if (ioctl(fb_fd, MSMFB_BLIT, &img)) {
    QTV_MSG_PRIO(QTVDIAG_GENERAL,QTVDIAG_PRIO_ERROR,"MSMFB_BLIT ioctl failed!\n");
    return;
  }
}

