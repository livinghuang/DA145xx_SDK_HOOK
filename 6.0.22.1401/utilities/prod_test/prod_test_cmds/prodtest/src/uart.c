/**
 ****************************************************************************************
 *
 * @file uart.c
 *
 * @brief UART interface for HCI messages.
 *
 * Copyright (C) 2012-2023 Renesas Electronics Corporation and/or its affiliates.
 * All rights reserved. Confidential Information.
 *
 * This software ("Software") is supplied by Renesas Electronics Corporation and/or its
 * affiliates ("Renesas"). Renesas grants you a personal, non-exclusive, non-transferable,
 * revocable, non-sub-licensable right and license to use the Software, solely if used in
 * or together with Renesas products. You may make copies of this Software, provided this
 * copyright notice and disclaimer ("Notice") is included in all such copies. Renesas
 * reserves the right to change or discontinue the Software at any time without notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS". RENESAS DISCLAIMS ALL WARRANTIES OF ANY KIND,
 * WHETHER EXPRESS, IMPLIED, OR STATUTORY, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. TO THE
 * MAXIMUM EXTENT PERMITTED UNDER LAW, IN NO EVENT SHALL RENESAS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE, EVEN IF RENESAS HAS BEEN ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGES. USE OF THIS SOFTWARE MAY BE SUBJECT TO TERMS AND CONDITIONS CONTAINED IN
 * AN ADDITIONAL AGREEMENT BETWEEN YOU AND RENESAS. IN CASE OF CONFLICT BETWEEN THE TERMS
 * OF THIS NOTICE AND ANY SUCH ADDITIONAL LICENSE AGREEMENT, THE TERMS OF THE AGREEMENT
 * SHALL TAKE PRECEDENCE. BY CONTINUING TO USE THIS SOFTWARE, YOU AGREE TO THE TERMS OF
 * THIS NOTICE.IF YOU DO NOT AGREE TO THESE TERMS, YOU ARE NOT PERMITTED TO USE THIS
 * SOFTWARE.
 *
 ****************************************************************************************
 */

#include <conio.h>
#include <process.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <stddef.h>     // standard definition
#include <assert.h>

#include "queue.h"
#include "uart.h"

//#define COMM_DEBUG

HANDLE hComPortHandle = NULL;
OVERLAPPED ovlRd,ovlWr;

/*
 ****************************************************************************************
 * @brief Write message to UART.
 * @param[in] payload_type 0x01 = HCI_CMD, 0x05 = FE_MSG
 * @param[in] payload_size Message size.
 * @param[in] payload      Pointer to message data.
 ****************************************************************************************
*/
void UARTSend(unsigned char payload_type, unsigned short payload_size, unsigned char *payload)
{
	unsigned char bTransmit232ElementArr[500];
	unsigned short bSenderSize;
	unsigned long dwWritten;

	bTransmit232ElementArr[0] = payload_type; // message header
	memcpy(&bTransmit232ElementArr[1], payload, payload_size);

	bSenderSize = payload_size + 1;

	ovlWr.Offset     = 0;
    ovlWr.OffsetHigh = 0;
    ResetEvent(ovlWr.hEvent);

	WriteFile(hComPortHandle, bTransmit232ElementArr, bSenderSize, &dwWritten, &ovlWr);
}

/*
 ****************************************************************************************
 * @brief Send message received from UART to application's main thread.
 * @param[in] length        Message size.
 * @param[in] bInputDataPtr Pointer to message data.
 ****************************************************************************************
*/
void SendToMain(unsigned char payload_type, unsigned short length, uint8_t *bInputDataPtr)
{
	QueueElement * qe; 
	unsigned char *bDataPtr; 

	// filter out FE API messages
	if (payload_type == 0x05)
	{
		return;
	}

	qe = (QueueElement *) malloc(sizeof(QueueElement));
	assert(qe);
	bDataPtr = (unsigned char *) malloc(length);
	assert(bDataPtr);
	
	memcpy(bDataPtr, bInputDataPtr, length);
	
	qe->payload_type = payload_type;
	qe->payload_size = length;
	qe->payload = bDataPtr;
	
	WaitForSingleObject(UARTRxQueueSem, INFINITE);
	EnQueue(&UARTRxQueue, qe);
	ReleaseMutex(UARTRxQueueSem);
}

/*
 ****************************************************************************************
 * @brief UART Reception thread loop.
 ****************************************************************************************
*/
void UARTProc(PVOID unused)
{
   unsigned long dwBytesRead;
   unsigned char tmp;
   unsigned short wReceive232Pos= 0 ;
   unsigned short wDataLength = 0;
   unsigned char bReceive232ElementArr[1000];
   unsigned char bReceiveState = 0;
   unsigned char bHdrBytesRead = 0;

   while(StopRxTask == FALSE)
   {

      ovlRd.Offset     = 0;
      ovlRd.OffsetHigh = 0;
      ResetEvent(ovlRd.hEvent);

      // use overlapped read, not because of async read, but, due to
      // multi thread read/write
      ReadFile( hComPortHandle, &tmp, 1, &dwBytesRead, &ovlRd );

      GetOverlappedResult( hComPortHandle,
                           &ovlRd,
                           &dwBytesRead,
                           TRUE );

      switch(bReceiveState)
      {
         case 0:   // Receive FE_MSG
            if(tmp == 0x05)
            {
               bReceiveState = 1;
			   wDataLength = 0;
               wReceive232Pos = 0;
			   bHdrBytesRead = 0;

			   bReceive232ElementArr[wReceive232Pos]=tmp;
			   wReceive232Pos++;

				#ifdef COMM_DEBUG
					printf("\nI: ");
					printf("%02X ", tmp);
				#endif   
            }
			else if (tmp == 0x04) // HCI event	
			{
					bReceiveState = 11; 
					wDataLength = 0;
					wReceive232Pos = 0;
					bHdrBytesRead = 0;

					bReceive232ElementArr[wReceive232Pos]=tmp;
					wReceive232Pos++; 	
			}
			else if (tmp == 0x01) // 1-wire echo
			{
					bReceiveState = 21;
					wDataLength = 0;
					wReceive232Pos = 0;
					bHdrBytesRead = 0;

					bReceive232ElementArr[wReceive232Pos]=tmp;
					wReceive232Pos++;
			}
            else
            {
                  #ifdef COMM_DEBUG
                     printf("%02X ", tmp);
                  #endif
            }
            break;

		 case 1:   // Receive Header size = 6
               #ifdef COMM_DEBUG
                  printf("%02X ", tmp);
               #endif
			 bHdrBytesRead++;
			 bReceive232ElementArr[wReceive232Pos] = tmp;
			 wReceive232Pos++;

			 if (bHdrBytesRead == 6)
				 bReceiveState = 2;
				
			 break;
		 case 2:   // Receive LSB of the length
			#ifdef COMM_DEBUG
				printf("%02X ", tmp);
			#endif
			wDataLength += tmp;
            if(wDataLength > MAX_PACKET_LENGTH)
            {
                 bReceiveState = 0;
            }
            else
			{
				bReceive232ElementArr[wReceive232Pos] = tmp;
				wReceive232Pos++;
                bReceiveState = 3;
			}
          break;
         case 3:   // Receive MSB of the length
               #ifdef COMM_DEBUG
                  printf("%02X ", tmp);
               #endif
            wDataLength += (unsigned short) (tmp*256);
            if(wDataLength > MAX_PACKET_LENGTH)
            {

				#ifdef COMM_DEBUG
					printf("\nSIZE: %d ", wDataLength);
				#endif
                bReceiveState = 0;
            }
			else if(wDataLength == 0)
			{
				#ifdef COMM_DEBUG
					printf("\nSIZE: %d ", wDataLength);
				#endif
				SendToMain(0x05, (unsigned short) (wReceive232Pos-1), &bReceive232ElementArr[1]); // an FE msg
                bReceiveState = 0;
			}
            else
			{
			   bReceive232ElementArr[wReceive232Pos] = tmp;
			   wReceive232Pos++;
               bReceiveState = 4;
			}
            break;
         case 4:   // Receive Data
			#ifdef COMM_DEBUG
				printf("%02X ", tmp);
            #endif
            bReceive232ElementArr[wReceive232Pos] = tmp;
            wReceive232Pos++;
			
            if(wReceive232Pos == wDataLength + 9 ) // 1 ( first byte - 0x05) + 2 (Type) + 2 (dstid) + 2 (srcid) + 2 (lengths size)
            {
               // Sendmail program
               SendToMain(0x05, (unsigned short) (wReceive232Pos-1), &bReceive232ElementArr[1]); ///FE msg
			   bReceiveState = 0;
				#ifdef COMM_DEBUG
					printf("\nSIZE: %d ", wDataLength);
				#endif
            }
           break;

			
		 case 11:   // Receive HCI event type byte
				bReceive232ElementArr[wReceive232Pos] = tmp;
				wReceive232Pos++;

				bReceiveState = 12;
				break;
	
		 case 12:   // Receive HCI event length byte
				wDataLength = tmp;
				
				if(wDataLength == 0)
				{
					bReceive232ElementArr[wReceive232Pos] = tmp;
					wReceive232Pos++;

					SendToMain(0x04, (unsigned short) (wReceive232Pos-1), &bReceive232ElementArr[1]);
					bReceiveState = 0;
				}
				else
				{
					bReceive232ElementArr[wReceive232Pos] = tmp;
					wReceive232Pos++;
					bReceiveState = 13;
				}
				break;
	
		 case 13:   // Receive HCI event data
				bReceive232ElementArr[wReceive232Pos] = tmp;
				wReceive232Pos++;

				if(wReceive232Pos == wDataLength + 3 ) // 1 ( first byte - 0x01) + 1 (event) + 1 (length)
				{
					SendToMain(0x04, (unsigned short) (wReceive232Pos-1), &bReceive232ElementArr[1]);
					bReceiveState = 0;
				}
				break;

		 case 21: // HCI Echo Send command Opcode
				 bReceive232ElementArr[wReceive232Pos] = tmp;
				 wReceive232Pos++;
				 bHdrBytesRead++;
				 if (bHdrBytesRead == 2)
					 bReceiveState = 22;
				 break;

		 case 22: // HCI Echo Send command length
				 bReceive232ElementArr[wReceive232Pos] = tmp;
				 wReceive232Pos++;
				 wDataLength = tmp;

				 if (wDataLength == 0)
					 bReceiveState = 0;
				 else
					 bReceiveState = 23;
				 break;

		 case 23: // HCI Echo Send command length - payload
				 bReceive232ElementArr[wReceive232Pos] = tmp;
				 wReceive232Pos++;

				 if (wReceive232Pos == wDataLength + 4) // 1 ( first byte - 0x01) + 2 (opcode) + 1 (length) + x (data)
					 bReceiveState = 0;
				 break;
      	 }
      
   }

   StopRxTask = TRUE;   // To indicate that the task has stopped

   PurgeComm(hComPortHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

   Sleep(100);

   CloseHandle(hComPortHandle);

   ExitThread(0);
}



/*
 ****************************************************************************************
 * @brief Init UART iface.
 * @param[in] Port     COM prot number.
 * @param[in] BaudRate Baud rate.
 * @return -1 on failure / 0 on success.
 ****************************************************************************************
*/
uint8_t InitUART(int Port, int BaudRate)
{
   DCB dcb;
   BOOL fSuccess;
   COMSTAT stat;
   DWORD error;
   COMMTIMEOUTS commtimeouts;
   char CPName[500];
#ifdef RSX
   DWORD dwErrorCode;
#endif

   sprintf(CPName, "\\\\.\\COM%d", Port);

#ifdef DEVELOPMENT_MESSAGES
   fprintf(stderr, "[info] Connecting to %s\n", &CPName[4]);
#endif //DEVELOPMENT_MESSAGES

   ovlRd.hEvent = CreateEvent( NULL,FALSE,FALSE,NULL );
   ovlWr.hEvent = CreateEvent( NULL,FALSE,FALSE,NULL );

   hComPortHandle = CreateFile(CPName,
                               GENERIC_WRITE | GENERIC_READ,
                               0, //FILE_SHARE_WRITE | FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_FLAG_OVERLAPPED,
                               NULL );

   if(hComPortHandle == INVALID_HANDLE_VALUE)
   {
      #ifdef RSX
         dwErrorCode = GetLastError();
         PrintfInt("Failed to open %s! %lu\n", PortName[Port], dwErrorCode);
      #endif
      return -1;
   }

   ClearCommError( hComPortHandle, &error, &stat );
   
   memset(&dcb, 0x0, sizeof(DCB) );
   fSuccess = GetCommState(hComPortHandle, &dcb);
   if(!fSuccess)
   {
      #ifdef RSX
         PrintfInt("Failed to get DCB!\n");
      #endif
      return -1;
   }

   // Fill in the DCB
   dcb.BaudRate = BaudRate;
   dcb.ByteSize = 8;
   dcb.Parity = NOPARITY;
   dcb.StopBits = ONESTOPBIT;
   dcb.fBinary = 1;
   // disable all kind of flow control and error handling
   dcb.fOutxCtsFlow = 0;
   dcb.fOutxDsrFlow = 0;
   dcb.fRtsControl  = RTS_CONTROL_DISABLE;
   dcb.fDtrControl  = DTR_CONTROL_DISABLE;
   dcb.fInX         = 0;
   dcb.fOutX        = 0;
   dcb.fErrorChar   = 0;
   dcb.fNull        = 0;
   dcb.fAbortOnError = 0;

   fSuccess = SetCommState(hComPortHandle, &dcb);
   if(!fSuccess)
   {
#ifdef DEVELOPMENT_MESSAGES
	   fprintf(stderr, "Failed to set DCB!\n");
#endif //DEVELOPMENT_MESSAGES
	   return -1;
   }
  commtimeouts.ReadIntervalTimeout = 1000; 
  commtimeouts.ReadTotalTimeoutMultiplier = 0; 
  commtimeouts.ReadTotalTimeoutConstant = 0; 
  commtimeouts.WriteTotalTimeoutMultiplier = 0; 
  commtimeouts.WriteTotalTimeoutConstant = 0;

  fSuccess = SetCommTimeouts( hComPortHandle,
                              &commtimeouts );
 
#ifdef DEVELOPMENT_MESSAGES
  fprintf(stderr, "[info] %s successfully opened, baud rate %d\n", &CPName[4], BaudRate);
#endif //DEVELOPMENT_MESSAGES

   return 0;
}
