/***************************************************************************//**
 * @file
 * @brief Routines for managing messages for sleepy end devices.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include "app/framework/include/af.h"
#include "sleepy-message-queue.h"

#include "sleepy-message-queue-config.h"

//------------------------------------------------------------------------------
// Forward Declaration
static sl_zigbee_af_event_t timeoutEvent;
static void timeoutEventHandler(sl_zigbee_af_event_t * event);
//------------------------------------------------------------------------------
// Internal Prototypes & Structure Definitions

// Internal supporting functions
static void  sli_zigbee_af_sleepy_messageQueueInitEntry(uint8_t index);
static void  sli_zigbee_af_restart_message_timer(void);
static uint8_t sli_zigbee_af_get_first_unused_queue_index(void);

//static void emRefreshMessageTimeouts( void );

typedef struct {
  sl_zigbee_af_sleepy_message_t sleepyMsg;
  uint8_t  status;
  uint32_t timeoutMSec;
} sli_zigbee_af_sleepy_message;

#define SLEEPY_MSG_QUEUE_NUM_ENTRIES  SL_ZIGBEE_AF_PLUGIN_SLEEPY_MESSAGE_QUEUE_SLEEPY_QUEUE_SIZE
#define MAX_MESSAGE_TIMEOUT_SEC  (0x7fffffff >> 10)

sli_zigbee_af_sleepy_message SleepyMessageQueue[SLEEPY_MSG_QUEUE_NUM_ENTRIES];

enum {
  SLEEPY_MSG_QUEUE_STATUS_UNUSED = 0x00,
  SLEEPY_MSG_QUEUE_STATUS_USED   = 0x01,
};

//------------------------------------------------------------------------------

void sl_zigbee_af_sleepy_message_queue_init_cb(uint8_t init_level)
{
  switch (init_level) {
    case SL_ZIGBEE_INIT_LEVEL_EVENT:
    {
      sl_zigbee_af_event_init(&timeoutEvent,
                              timeoutEventHandler);
      break;
    }
    case SL_ZIGBEE_INIT_LEVEL_LOCAL_DATA:
    {
      // Initialize sleepy buffer plugin.
      uint8_t x;
      for ( x = 0; x < SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ ) {
        sli_zigbee_af_sleepy_messageQueueInitEntry(x);
      }
      sl_zigbee_af_app_println("Initialized Sleepy Message Queue");
      break;
    }
  }
}

static void sli_zigbee_af_sleepy_messageQueueInitEntry(uint8_t x)
{
  if ( x < SLEEPY_MSG_QUEUE_NUM_ENTRIES ) {
    SleepyMessageQueue[x].sleepyMsg.payload = NULL;
    SleepyMessageQueue[x].sleepyMsg.length = 0;
    SleepyMessageQueue[x].sleepyMsg.payloadId = 0;
    memset(SleepyMessageQueue[x].sleepyMsg.dstEui64, 0, EUI64_SIZE);
    SleepyMessageQueue[x].status = SLEEPY_MSG_QUEUE_STATUS_UNUSED;
    SleepyMessageQueue[x].timeoutMSec = 0;
  }
}

uint8_t sl_zigbee_af_sleepy_message_queue_get_num_unused_entries()
{
  uint8_t x;
  uint8_t cnt = 0;
  for ( x = 0; x < SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ ) {
    if ( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_UNUSED ) {
      cnt++;
    }
  }
  return cnt;
}

static uint8_t sli_zigbee_af_get_first_unused_queue_index()
{
  uint8_t x;
  for ( x = 0; x < SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ ) {
    if ( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_UNUSED ) {
      break;
    }
  }
  if ( x >= SLEEPY_MSG_QUEUE_NUM_ENTRIES ) {
    x = SL_ZIGBEE_AF_PLUGIN_SLEEPY_MESSAGE_INVALID_ID;
  }
  return x;
}

#define MAX_DELAY_QS  0x7FFF
#define MAX_DELAY_MS  0x7FFF
static void sli_zigbee_af_restart_message_timer()
{
  uint32_t smallestTimeoutIndex = SLEEPY_MSG_QUEUE_NUM_ENTRIES;
  uint32_t smallestTimeoutMSec = 0xFFFFFFFF;
  uint32_t timeNowMs;
  uint32_t remainingMs;
  uint32_t delayQs;
  uint8_t x;

  timeNowMs = halCommonGetInt32uMillisecondTick();

  for ( x = 0; x < SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ ) {
    if ( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED ) {
      if ( timeGTorEqualInt32u(timeNowMs, SleepyMessageQueue[x].timeoutMSec) ) {
        // Timeout already expired - break out of loop - process immediately.
        smallestTimeoutIndex = x;
        smallestTimeoutMSec = 0;
        break;
      } else {
        remainingMs = elapsedTimeInt32u(timeNowMs, SleepyMessageQueue[x].timeoutMSec);
        if ( remainingMs < smallestTimeoutMSec ) {
          smallestTimeoutMSec = remainingMs;
          smallestTimeoutIndex = x;
        }
      }
    }
  }
  // Now know the smallest timeout index, and the smallest timeout value.
  if ( smallestTimeoutIndex < SLEEPY_MSG_QUEUE_NUM_ENTRIES ) {
    // Run the actual timer as a QS timer since that allows us to delay longer
    // than a u16 MS timer would.
    delayQs = smallestTimeoutMSec >> 8;
    delayQs++;    // Round up to the next quarter second tick.
    if ( delayQs > MAX_DELAY_QS ) {
      delayQs = MAX_DELAY_QS;
    }

    sl_zigbee_af_event_set_delay_qs(&timeoutEvent, delayQs);
    sl_zigbee_af_app_println("Restarting sleepy message timer for %d Qsec.", delayQs);
  }
}

sl_zigbee_af_sleepy_message_id_t sl_zigbee_af_sleepy_message_queue_store_message(sl_zigbee_af_sleepy_message_t *pmsg, uint32_t timeoutSec)
{
  uint8_t x;

  if ( timeoutSec <= MAX_MESSAGE_TIMEOUT_SEC ) {
    x = sli_zigbee_af_get_first_unused_queue_index();
    if ( x < SLEEPY_MSG_QUEUE_NUM_ENTRIES ) {
      memcpy( (uint8_t *)&SleepyMessageQueue[x].sleepyMsg, (uint8_t *)pmsg, sizeof(sl_zigbee_af_sleepy_message_t) );
      SleepyMessageQueue[x].status = SLEEPY_MSG_QUEUE_STATUS_USED;
      SleepyMessageQueue[x].timeoutMSec = (timeoutSec << 10) + halCommonGetInt32uMillisecondTick();

      // Reschedule timer after adding the new message since new message could have the shortest timeout.
      sli_zigbee_af_restart_message_timer();
    }
    return x;
  }
  return SL_ZIGBEE_AF_PLUGIN_SLEEPY_MESSAGE_INVALID_ID;
}

sl_zigbee_af_sleepy_message_id_t sl_zigbee_af_sleepy_message_queue_get_pending_message_id(sl_802154_long_addr_t dstEui64)
{
  uint8_t  smallestTimeoutIndex = SLEEPY_MSG_QUEUE_NUM_ENTRIES;
  uint32_t smallestTimeoutMSec = 0xFFFFFFFF;
  uint32_t timeNowMs;
  uint32_t remainingMs;
  uint8_t  x;
  uint8_t  stat;

  timeNowMs = halCommonGetInt32uMillisecondTick();
  for ( x = 0; x < SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ ) {
    if ( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED ) {
      stat = memcmp(SleepyMessageQueue[x].sleepyMsg.dstEui64, dstEui64, EUI64_SIZE);
      if ( !stat ) {
        // Matching entry found - look for match with smallest timeout.
        if ( timeGTorEqualInt32u(timeNowMs, SleepyMessageQueue[x].timeoutMSec) ) {
          // Timeout already expired - break out of loop - process immediately.
          smallestTimeoutIndex = x;
          smallestTimeoutMSec = 0;
          break;
        } else {
          remainingMs = elapsedTimeInt32u(timeNowMs, SleepyMessageQueue[x].timeoutMSec);
          if ( remainingMs < smallestTimeoutMSec ) {
            smallestTimeoutMSec = remainingMs;
            smallestTimeoutIndex = x;
          }
        }
      }
    }
  }
  if ( smallestTimeoutIndex >= SLEEPY_MSG_QUEUE_NUM_ENTRIES ) {
    smallestTimeoutIndex = SL_ZIGBEE_AF_PLUGIN_SLEEPY_MESSAGE_INVALID_ID;
  }
  return smallestTimeoutIndex;
}

uint32_t sli_zigbee_af_message_m_sec_remaining(sl_zigbee_af_sleepy_message_id_t sleepyMsgId)
{
  uint32_t remainingMs = 0xFFFFFFFF;
  uint32_t timeNowMs = halCommonGetInt32uMillisecondTick();

  if ( (sleepyMsgId < SLEEPY_MSG_QUEUE_NUM_ENTRIES) && (SleepyMessageQueue[sleepyMsgId].status == SLEEPY_MSG_QUEUE_STATUS_USED) ) {
    if ( timeGTorEqualInt32u(timeNowMs, SleepyMessageQueue[sleepyMsgId].timeoutMSec) ) {
      remainingMs = 0;
    } else {
      remainingMs = elapsedTimeInt32u(timeNowMs, SleepyMessageQueue[sleepyMsgId].timeoutMSec);
    }
  }
  return remainingMs;
}

#define PRINT_TIME(x)  PrintMessageTimeInfo(x)
void PrintMessageTimeInfo(uint8_t x)
{
  if ( (x < SLEEPY_MSG_QUEUE_NUM_ENTRIES) && (SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED) ) {
    sl_zigbee_af_app_println("==== USED MESSAGE, x=%d", x);
    sl_zigbee_af_app_println("  currTime=%d, timeout=%d", halCommonGetInt32uMillisecondTick(), SleepyMessageQueue[x].timeoutMSec);
  }
}

bool sl_zigbee_af_sleepy_message_queue_get_pending_message(sl_zigbee_af_sleepy_message_id_t sleepyMsgId, sl_zigbee_af_sleepy_message_t *pmsg)
{
  if ( (sleepyMsgId < SLEEPY_MSG_QUEUE_NUM_ENTRIES) && (SleepyMessageQueue[sleepyMsgId].status == SLEEPY_MSG_QUEUE_STATUS_USED) ) {
    memcpy( (uint8_t *)pmsg, (uint8_t *)&SleepyMessageQueue[sleepyMsgId].sleepyMsg, sizeof(sl_zigbee_af_sleepy_message_t) );
    return true;
  }
  return false;
}

uint8_t sl_zigbee_af_sleepy_message_queue_get_num_messages(sl_802154_long_addr_t dstEui64)
{
  uint8_t x;
  uint8_t stat;
  uint8_t matchCnt = 0;

  for ( x = 0; x < SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ ) {
    if ( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED ) {
      stat = memcmp(SleepyMessageQueue[x].sleepyMsg.dstEui64, dstEui64, EUI64_SIZE);
      if ( !stat ) {
        matchCnt++;
      }
    }
  }
  return matchCnt;
}

bool sl_zigbee_af_sleepy_message_queue_remove_message(sl_zigbee_af_sleepy_message_id_t  sleepyMsgId)
{
  if ( sleepyMsgId < SLEEPY_MSG_QUEUE_NUM_ENTRIES ) {
    sli_zigbee_af_sleepy_messageQueueInitEntry(sleepyMsgId);
    sli_zigbee_af_restart_message_timer();    // Restart the message timer
    return true;
  }
  return false;
}

void sl_zigbee_af_sleepy_message_queue_remove_all_messages(sl_802154_long_addr_t dstEui64)
{
  uint8_t x;
  uint8_t stat;
  uint8_t matchCnt = 0;

  for ( x = 0; x < SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ ) {
    if ( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED ) {
      stat = memcmp(SleepyMessageQueue[x].sleepyMsg.dstEui64, dstEui64, EUI64_SIZE);
      if ( !stat ) {
        sli_zigbee_af_sleepy_messageQueueInitEntry(x);
        matchCnt++;
      }
    }
  }
  if ( matchCnt ) {
    // At least one entry was removed.  Restart the message timer.
    sli_zigbee_af_restart_message_timer();
  }
}

static void timeoutEventHandler(sl_zigbee_af_event_t * event)
{
  uint32_t timeNowMs;
  uint8_t x;

  sl_zigbee_af_event_set_inactive(&timeoutEvent);
  timeNowMs = halCommonGetInt32uMillisecondTick();

  for ( x = 0; x < SLEEPY_MSG_QUEUE_NUM_ENTRIES; x++ ) {
    if ( SleepyMessageQueue[x].status == SLEEPY_MSG_QUEUE_STATUS_USED ) {
      if ( timeGTorEqualInt32u(timeNowMs, SleepyMessageQueue[x].timeoutMSec) ) {
        // Found entry that expired.
        // Invoke callback to notify about message timeout.
        // Allow the callback the opportunity to access elements in this entry.
        sl_zigbee_af_app_println("Expiring Message %d", x);
        sl_zigbee_af_sleepy_message_queue_message_timed_out_cb(x);

        // After timeout callback completes, mark event as unused.
        sli_zigbee_af_sleepy_messageQueueInitEntry(x);
      }
    }
  }

  // Restart timer if another message is buffered.
  sli_zigbee_af_restart_message_timer();
}
