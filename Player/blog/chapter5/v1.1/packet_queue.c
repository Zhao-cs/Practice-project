/*=========================================================================\
* Copyright(C)2016 Chudai.
*
* File name    : packet_queue.c
* Version      : v1.0.0
* Author       : 初代
* Date         : 2016/10/03
* Description  :
* Function list: 1.
*                2.
*                3.
* History      :
\*=========================================================================*/

/*-----------------------------------------------------------*
 * 头文件                                                    *
 *-----------------------------------------------------------*/

#define __STDC_CONSTANT_MACROS      //ffmpeg要求

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>

#ifdef __cplusplus
}
#endif

#include "packet_queue.h"
/*======================================================================\
* Author     (作者): i.sshe
* Date       (日期): 2016/10/03
* Others     (其他):
\*=======================================================================*/
void packet_queue_init(PacketQueue *queue)
{
//     memset(queue, 0, sizeof(PacketQueue));
     queue->first_pkt    = NULL;
     queue->last_pkt     = NULL;
     queue->mutex           = SDL_CreateMutex();
     queue->cond            = SDL_CreateCond();
}



/*======================================================================\
* Author     (作者): i.sshe
* Date       (日期): 2016/10/03
* Others     (其他):
\*=======================================================================*/
int packet_queue_put(PacketQueue *queue, AVPacket *packet)
{
     AVPacketList   *pkt_list;

     // ???
     if (av_dup_packet(packet) < 0)
     {
         return -1;
     }

     pkt_list = (AVPacketList *)av_malloc(sizeof(AVPacketList));
     if (pkt_list == NULL)
     {
          return -1;
     }

     pkt_list->pkt   = *packet;
     pkt_list->next  = NULL;

     //上锁
     SDL_LockMutex(queue->mutex);

     if (queue->last_pkt == NULL)    //空队
     {
          queue->first_pkt = pkt_list;
     }
     else
     {
         queue->last_pkt->next = pkt_list;
     }

     queue->last_pkt = pkt_list;  //这里queue->last_pkt = queue->last_pkt->next 的意思，但是，处理了更多的情况。
     queue->nb_packets++;
     queue->size += packet->size;
     SDL_CondSignal(queue->cond);      //???

     SDL_UnlockMutex(queue->mutex);

     return 0;
}

/*======================================================================\
* Author     (作者): i.sshe
* Date       (日期): 2016/10/03
* Others     (其他):
\*=======================================================================*/
extern int quit;
int packet_queue_get(PacketQueue *queue, AVPacket *pkt, int block)
{
     AVPacketList   *pkt_list = NULL;
     int            ret = 0;

     SDL_LockMutex(queue->mutex);

     while(1)
     {
          if (quit)
          {
              ret = -1;
              break;
          }

          pkt_list = queue->first_pkt;
          if (pkt_list != NULL)         //队不空，还有数据
          {
              queue->first_pkt = queue->first_pkt->next;    //pkt_list->next
              if (queue->first_pkt == NULL)
              {
                   queue->last_pkt = NULL;
              }

              queue->nb_packets--;
              queue->size -= pkt_list->pkt.size;
              *pkt = pkt_list->pkt;          // 复制给packet。
              av_free(pkt_list);
              ret = 1;
              break;
          }
          else if (block == 0)
          {
               ret = 0;
               break;
          }
          else
          {
              SDL_CondWait(queue->cond, queue->mutex);
          }
     }

     SDL_UnlockMutex(queue->mutex);

     return ret;
}
