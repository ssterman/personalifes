// Virtual timer implementation

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "nrf.h"

#include "virtual_timer.h"
#include "virtual_timer_linked_list.h"


// This is the interrupt handler that fires on a compare event
void TIMER4_IRQHandler(void) {
  // This should always be the first line of the interrupt handler!
  // It clears the event so that it doesn't happen again
  NRF_TIMER4->EVENTS_COMPARE[0] = 0;
  NRF_TIMER4->EVENTS_COMPARE[2] = 0;
  //printf("Timer Fired!\n");
  node_t* timer_node = list_get_first();
  timer_node->cb();
  //printf("Is it repeated: %d\n", timer_node->repeated);
  if (timer_node->repeated) {
  	//uint32_t current_time = read_timer();
  	uint32_t next_interrupt_time = timer_node->timer_value + timer_node->microseconds;
  	timer_node->timer_value = next_interrupt_time;
  	__disable_irq();
  	list_remove(timer_node);
  	list_insert_sorted(timer_node);
  	__enable_irq();
  	//printf("next interrupt time: %d\n", timer_node->timer_value);
  	node_t* next_node = list_get_first();
  	uint32_t current_time = read_timer();
  	if (next_node != NULL) {
  		NRF_TIMER4->CC[2] = next_node->timer_value;
  	}
  	while ((next_node!=NULL) && (current_time > next_node->timer_value)) {
  		next_node->cb();
  		next_node->timer_value = next_node->timer_value + next_node->microseconds;
  		__disable_irq();
  		list_remove(next_node);
  		list_insert_sorted(next_node);
  		__enable_irq();
  		next_node = list_get_first();
  		NRF_TIMER4->CC[2] = next_node->timer_value;
  	}
  }
  else {
  	__disable_irq();
  	list_remove(timer_node);
  	__enable_irq();
  	free(timer_node);
  	node_t* next_node = list_get_first();
  	if (next_node != NULL) {
  		NRF_TIMER4->CC[2] = next_node->timer_value;
  	}
  	//node_t* next_node = list_get_first();
  	uint32_t current_time = read_timer();
  	while ((next_node!=NULL) && (current_time > next_node->timer_value)) {
  		next_node->cb();
  		__disable_irq();
  		list_remove(next_node);
  		__enable_irq();
  		next_node = list_get_first();
  		NRF_TIMER4->CC[2] = next_node->timer_value;
  	}
  }
  // Place your interrupt handler code here
  //list_print();
}

// Read the current value of the timer counter
uint32_t read_timer(void) {
	NRF_TIMER4->TASKS_CAPTURE[1] = 1;
	uint32_t internal_counter = NRF_TIMER4->CC[1];
  // Should return the value of the internal counter for TIMER4
  return internal_counter;
}

// Initialize TIMER4 as a free running timer
// 1) Set to be a 32 bit timer
// 2) Set to count at 1MHz
// 3) Enable the timer peripheral interrupt (look carefully at the INTENSET register!)
// 4) Clear the timer
// 5) Start the timer
void virtual_timer_init(void) {
  // Place your timer initialization code here
	NRF_TIMER4->MODE = 0;
	NRF_TIMER4->BITMODE = 3;
	NRF_TIMER4->PRESCALER = 4;
	NRF_TIMER4->TASKS_CLEAR = 1;
	NRF_TIMER4->TASKS_START = 1;
	NRF_TIMER4->INTENSET = 1<<18;
	NVIC_EnableIRQ(TIMER4_IRQn);
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
  node_t* timer = malloc(sizeof(node_t));
  uint32_t current_time = read_timer();
  uint32_t next_interrupt_time = current_time + microseconds;
  timer->timer_value = next_interrupt_time;
  timer->cb = cb;
  timer->repeated = repeated;
  timer->microseconds = microseconds;
  __disable_irq();
  list_insert_sorted(timer);
  node_t* timer_node = list_get_first();
  __enable_irq();
  //if (current_time > NRF_TIMER4->CC[2]) {
  NRF_TIMER4->CC[2] = timer_node->timer_value;
  //}
  // Return a unique timer ID. (hint: What is guaranteed unique about the timer you have created?)
  return (uint32_t) timer;
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
	node_t* timer_node = (node_t*) timer_id;
	if (timer_node->next != NULL) {
		list_remove(timer_node);
  		free(timer_node);
		node_t* next_node = list_get_first();
		NRF_TIMER4->CC[2] = next_node->timer_value;
	}
  	else {
  		uint32_t current_time = read_timer();
  		NRF_TIMER4->CC[2] = current_time - 1;
  		list_remove(timer_node);
  		free(timer_node);
	}
}

