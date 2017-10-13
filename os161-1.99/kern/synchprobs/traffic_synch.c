#include <types.h>
#include <lib.h>
#include <synchprobs.h>
#include <synch.h>
#include <opt-A1.h>

#include <array.h>

bool right_turn (Direction * origin, Direction * destination);
bool good_pair (Direction * first_origin, 
                Direction * first_destination, 
                Direction * second_origin,
                Direction * second_destination);

 bool right_turn (Direction * origin, Direction * destination) {
    if (*origin == north && *destination == west) {
      return true;
    }
    else if (*origin == east && *destination == north) {
      return true;      
    }
    else if (*origin == south && *destination == east) {
      return true;      
    }
    else if (*origin == west && *destination == south) {
      return true;      
    }
    else {
      return false;
    }
}

bool good_pair (Direction *first_origin, 
                Direction * first_destination, 
                Direction * second_origin,
                Direction * second_destination) {
  if (*first_origin == *second_origin) {
    return true;
  }
  else if ((*first_origin == *second_destination) && (*second_origin == *first_destination)) {
    return true;
  }
  else if ((*first_destination != *second_destination) && 
           (right_turn(first_origin, first_destination) || right_turn(second_origin, second_destination))) {
    return true;
  }
  else {
    return false;
  }
}

struct array* origin_array; 
struct array* destination_array; 
struct lock* masterLock;
struct cv* masterCV;

/* 
 * The simulation driver will call this function once before starting
 * the simulation
 *
 * You can use it to initialize synchronization and other variables.
 * 
 */
void
intersection_sync_init(void)
{
  origin_array = array_create();
  array_init(origin_array);
  destination_array = array_create();
  array_init(destination_array);
  masterLock = lock_create("masterLock");
  masterCV = cv_create("masterCV");
}

/* 
 * The simulation driver will call this function once after
 * the simulation has finished
 *
 * You can use it to clean up any synchronization and other variables.
 *
 */
void
intersection_sync_cleanup(void)
{
  array_destroy(origin_array);
  array_destroy(destination_array);  
  lock_destroy(masterLock);
  cv_destroy(masterCV);
}  
/*
 * The simulation driver will call this function each time a vehicle
 * tries to enter the intersection, before it enters.
 * This function should cause the calling simulation thread 
 * to block until it is OK for the vehicle to enter the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle is arriving
 *    * destination: the Direction in which the vehicle is trying to go
 *
 * return value: none
 */

void
intersection_before_entry(Direction origin, Direction destination) 
{
  lock_acquire(masterLock);

  int array_size = array_num(origin_array);

  Direction * input_origin = kmalloc(sizeof(enum Directions));
  Direction * input_destination = kmalloc(sizeof(enum Directions));
  *input_origin = origin;
  *input_destination = destination;

  bool safe = false;
  LOOP:while ( safe == false ) {
    array_size = array_num(origin_array);

    if (array_size == 0) {
      safe = true;
    }
    
    for (int i = 0; i < array_size; i++) { 
      safe = good_pair(input_origin, input_destination, array_get(origin_array, i), array_get(destination_array, i));

      if ( safe == false ) {
        cv_wait(masterCV, masterLock);
        goto LOOP;
      }
    }

    array_add(origin_array, input_origin, NULL);
    array_add(destination_array, input_destination, NULL);
  }

  lock_release(masterLock);
}


/*
 * The simulation driver will call this function each time a vehicle
 * leaves the intersection.
 *
 * parameters:
 *    * origin: the Direction from which the vehicle arrived
 *    * destination: the Direction in which the vehicle is going
 *
 * return value: none
 */

void
intersection_after_exit(Direction origin, Direction destination) 
{
  lock_acquire(masterLock);
  
  int array_size = array_num(origin_array); 
  
  for (int i = 0; i < array_size; i++) { 
    Direction * target_origin = array_get(origin_array, i);
    Direction * target_destination = array_get(destination_array, i);
    if ((*target_origin == origin) && (*target_destination == destination)) {
      array_remove(origin_array, i);
      array_remove(destination_array, i);
      cv_broadcast(masterCV, masterLock);
      break;
    }
  }

  lock_release(masterLock);
}
