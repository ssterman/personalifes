// Virtual timer implementation

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "nrf.h"

#include "virtual_timer.h"
#include "virtual_timer_linked_list.h"

void handle_first_timer(void) {
  node_t* node = list_remove_first();

  if (node != NULL) {
    (node->callback)();

    if (node->repeated) {
      printf("repeated\n");
      node->timer_value = node->timer_value + node->microseconds_offset;
      list_insert_sorted(node);
    } else {
      free(node);
    }
  }
}

node_t* handle_expired_timers(void) {
  node_t* firstnode = list_get_first();
  
  while (firstnode != NULL && firstnode->timer_value <= read_timer()) {
    handle_first_timer();
    firstnode = list_get_first();
  }
  
  NRF_TIMER4->EVENTS_COMPARE[0] = 0;
  if (firstnode != NULL) {
    NRF_TIMER4->CC[0] = firstnode->timer_value;
  }

  return firstnode;
}

// This is the interrupt handler that fires on a compare event
void TIMER4_IRQHandler(void) {
  // This should always be the first line of the interrupt handler!
  // It clears the event so that it doesn't happen again
  __disable_irq();
  NRF_TIMER4->EVENTS_COMPARE[0] = 0;
  // if (list_get_first()->timer_value > read_timer()) {
  //   return;
  // }
  //printf("Timer Fired!");
  // Place your interrupt handler code here
  node_t* firstnode = handle_expired_timers();
  __enable_irq();
}


// Read the current value of the timer counter
uint32_t read_timer(void) {
  //force a write
  NRF_TIMER4->TASKS_CAPTURE[1] = 1;
  //return the value of cc
  // Should return the value of the internal counter for TIMER4
  return NRF_TIMER4->CC[1];
}

// Initialize TIMER4 as a free running timer
// 1) Set to be a 32 bit timer
// 2) Set to count at 1MHz
// 3) Enable the timer peripheral interrupt (look carefully at the INTENSET register!)
// 4) Clear the timer
// 5) Start the timer
void virtual_timer_init(void) {
  // Place your timer initialization code here
  // set BITMODE to 32 bit
  NRF_TIMER4->BITMODE = 3;
  // set PRESCALER to divide 16MHz by 16, so 2^4, so set to 4
  NRF_TIMER4->PRESCALER = 4;  

  NRF_TIMER4->TASKS_CLEAR = 1;
  NRF_TIMER4->TASKS_START = 1;

  NRF_TIMER4->INTENSET |= 1 << 16;
  NVIC_EnableIRQ(TIMER4_IRQn);
  NVIC_SetPriority(TIMER4_IRQn, 0);
}

// Start a timer. This function is called for both one-shot and repeated timers
// To start a timer:
// 1) Create a linked list node (This requires `malloc()`. Don't forget to free later)
// 2) Setup the linked list node with the correct information
//      - You will need to modify the `node_t` struct in "virtual_timer_linked_list.h"!
// 3) Place the node in the linked list
// 4) Setup the compare register so that the timer fires at the right time
// 5) Return a timer ID
//
// Your implementation will also have to take special precautions to make sure that
//  - You do not miss any timers
//  - You do not cause consistency issues in the linked list (hint: you may need the `__disable_irq()` and `__enable_irq()` functions).
//
// Follow the lab manual and start with simple cases first, building complexity and
// testing it over time.
static uint32_t timer_start(uint32_t microseconds, virtual_timer_callback_t cb, bool repeated) {
  //NRF_TIMER4->CC[0] = read_timer() + microseconds;
  
  node_t* new_node_ptr = malloc(sizeof(node_t));

  new_node_ptr->timer_value = read_timer() + microseconds;
  new_node_ptr->callback = cb;
  new_node_ptr->repeated = repeated;
  new_node_ptr->microseconds_offset = microseconds;

  __disable_irq();
  list_insert_sorted(new_node_ptr);
  node_t* firstnode = handle_expired_timers();
  __enable_irq();

  // Return a unique timer ID. (hint: What is guaranteed unique about the timer you have created?)
  return (uint32_t) new_node_ptr;
}

// You do not need to modify this function
// Instead, implement timer_start
uint32_t virtual_timer_start(uint32_t microseconds, virtual_timer_callback_t cb) {
  return timer_start(microseconds, cb, false);
}

// You do not need to modify this function
// Instead, implement timer_start
uint32_t virtual_timer_start_repeated(uint32_t microseconds, virtual_timer_callback_t cb) {
  return timer_start(microseconds, cb, true);
}

// Remove a timer by ID.
// Make sure you don't cause linked list consistency issues!
// Do not forget to free removed timers.
void virtual_timer_cancel(uint32_t timer_id) {
  node_t * node = (node_t *) timer_id;
  printf("cancel\n");
  __disable_irq();
  if (list_get_first() == NULL) {
    //empty 
  } else if (list_get_first() == node) {
    printf("first\n");
    list_remove_first();
    free(node);
    node_t* firstnode = handle_expired_timers();
  } else {
    list_remove((node_t *) node);
    free(node);
  }
  __enable_irq();
}

