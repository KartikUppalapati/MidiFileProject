/* Kartik Uppalapti, alterations.c, CS 24000, Fall 2020
 * Last updated October 12, 2020
 */

/* Add any includes here */
#include "alterations.h"
#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>


/* This function
 * changes an individual event ocatve
 */

int change_event_octave(event_t *event, int *octave_num) {

  // Initialize variables
  int new_event_octave = 0;

  // Check that type within bounds
  if ((event->type >= 0x08) && (event->type <= 0xAF)) {
    new_event_octave = (*octave_num * 12) + event->midi_event.data[0];
    // Check that new event octave is within bounds
    if ((new_event_octave >= 0) && (new_event_octave <= 127)) {
      // Reassign octave and return success value
      event->midi_event.data[0] = new_event_octave;
      return 1;
    }
  }

  // Else return 0 for not success
  return 0;

} /* change_event_octave() */


/* This function
 * is a helper function to calculate bytes needed for a vlq conversion
 */

int bytes_needed_for_conversion(int before_time, int after_time) {

  // Intialize variables
  int before_bits = 0;
  int after_bits = 0;
  int before_bytes = 0;
  int after_bytes = 0;

  // Use for loops to find number of bits needed for before and after time
  for (int i = 0; i < 28; i++) {
    // Use bit shifting to find num of bits needed to express before time
    if (before_time < (1 << i)) {
      before_bits = i;
      break;
    }
  }
  for (int k = 0; k < 28; k++) {
    // Use bit shifting to find num of bits needed to express after time
    if (after_time < (1 << k)) {
      after_bits = k;
      break;
    }
  }

  // Get bytes needed for vlq representation using bits needed value
  for (after_bytes = 1; after_bytes <= 4; after_bytes++) {
    if (after_bits <= (after_bytes * 7)) {
      break;
    }
  }
  for (before_bytes = 1; before_bytes <= 4; before_bytes++) {
    if (before_bits <= (before_bytes * 7)) {
      break;
    }
  }


  // Return difference in bytes between after and before times
  return after_bytes - before_bytes;

} /* bytes_needed_for_conversion() */


/* This function
 * changes an individual event time using multiplier input
 */

int change_event_time(event_t *event, float *multiplier) {

  // Intialize variables
  int before_time = 0;
  int after_time = 0;
  int bytes_needed = 0;

  // Set before time and after time using original delta time and multiplier
  before_time = event->delta_time;
  after_time = before_time * (*multiplier);
  event->delta_time = event->delta_time * (*multiplier);

  // Use helper function to get bytes needed
  bytes_needed = bytes_needed_for_conversion(before_time, after_time);


  // Return bytes needed
  return bytes_needed;
  
} /* change_event_time() */


/* This function
 * function changes a midi event instrument used
 */

int change_event_instrument(event_t *event, remapping_t remap) {

  // Check bounds
  if ((event->type >= 0xC0) && (event->type <= 0xCF)) {
    // Remap using input and return success
    event->midi_event.data[0] = remap[event->midi_event.data[0]];
    return 1;
  }

  // Else return 0 for not success
  return 0;

} /* change_event_instrumentation() */


/* This function
 * function changes the notes of midi events to input notes
 */

int change_event_note(event_t *event, remapping_t remap) {

  // Check that within bounds
  if (((event->type & 0xF0) == 0x80) ||
      ((event->type & 0xF0) == 0x90) ||
      ((event->type & 0xF0) == 0xA0)) {
     // Remap using input and return success
     event->midi_event.data[0] = remap[event->midi_event.data[0]];
     return 1;
  }

  // Else return 0 for not success
  return 0;

} /* change_event_note() */


/* This function
 * does a certain action to all events of all tracks of input song
 */

int apply_to_events(song_data_t *song, event_func_t function_to_do, void *arbitrary) {

  // Initialize variables
  int sum = 0;
  int function_return_value = 0;
  track_node_t *current_track = song->track_list;
  event_node_t *current_event = NULL;

  // Use nested while loop to get sum of all events
  while (1) {
    // Stop when track is null
    if (current_track == NULL) {
      break;
    }
    // Set current event to first event of current track
    current_event = current_track->track->event_list;
    // Else use another while loop to sum event data
    while (1) {
      // Stop when event is null
      if (current_event == NULL) {
        break;
      }
      // Else sum track data using input function
      function_return_value = function_to_do(current_event->event, arbitrary);
      sum += function_return_value;
      // Go to next event
      current_event = current_event->next_event;
    }
    // Go to next track
    current_track = current_track->next_track;
  }


  // Return sum
  return sum;

} /* apply_to_events() */


/* This function
 * changs an individual event octave
 */

int change_octave(song_data_t *song, int octave_shift) {

  // Initialize variable
  int events_changed = 0;
  int *octave_shift_ptr = &octave_shift;

  // Use apply to events function to change octave for all events
  events_changed = apply_to_events(song, (event_func_t) change_event_octave, octave_shift_ptr);


  return events_changed;

} /* change_octave */


/* This function
 * changes the time of the track by a certain multiplier value
 */

int warp_time(song_data_t *song, float multiplier) {

  // Initialize variables
  int byte_difference = 0;
  int function_return_value = 0;
  track_node_t *current_track = song->track_list;
  event_node_t *current_event = NULL;

  // Use nested while loop to get sum of all events
  while (1) {
    // Stop when track is null
    if (current_track == NULL) {
      break;
    }
    // Set current event to first event of current track
    current_event = current_track->track->event_list;
    // Else use another while loop to sum event data
    while (1) {
      // Stop when event is null
      if (current_event == NULL) {
        break;
      }
      // Else get differnce in bytes using change event function
      function_return_value = change_event_time(current_event->event, &multiplier);
      // Update track len based on byte difference
      current_track->track->length += function_return_value;
      // Add byte difference to total byte difference
      byte_difference += function_return_value;
      // Go to next event
      current_event = current_event->next_event;
    }
    // Go to next track
    current_track = current_track->next_track;
  }


  // Return sum
  return byte_difference;


} /* warp_time() */


/* This function
 * remaps the instruments of all events to input instruemnt map
 */

int remap_instruments(song_data_t *song, remapping_t remap) {

  // Initalize variables
  int events_changed = 0;

  // Use apply to events function
  events_changed = apply_to_events(song, (event_func_t) change_event_instrument, remap);

  // Return num of events changed
  return events_changed;

} /* remap_instruments() */


/* This function
 * remaps the notes of all midi events to input note map
 */

int remap_notes(song_data_t *song, remapping_t remap) {

  // Initalize variables
  int events_changed = 0;

  // Use apply to events function
  events_changed = apply_to_events(song, (event_func_t) change_event_note, remap);

  // Return num of events changed
  return events_changed;

} /* remap_notes() */


/* This function
 * is a helper function to find unused lowest channel of midi events
 */

int channel_asserts(song_data_t *song) {

  // Initialize variables
  int channels[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  int lowest = 20;
  int low_bits = 0;
  track_node_t *current_track = song->track_list;
  event_node_t *current_event = NULL;

  // Use nested while loop to get all midi events
  while (1) {
    // Stop when track is null
    if (current_track == NULL) {
      break;
    }
    // Set current event to first event of current track
    current_event = current_track->track->event_list;
    // Else use another while loop to sum event data
    while (1) {
      // Stop when event is null
      if (current_event == NULL) {
        break;
      }
      // Get channel number using lowest 4 bits
      if ((current_event->event->type >= 0x80) && (current_event->event->type <= 0xEF)) {
        low_bits = current_event->event->type & 0x0F;
        channels[low_bits] = 16;
      }
      // Go to next event
      current_event = current_event->next_event;
    }
    // Go to next track
    current_track = current_track->next_track;
  }

  // Find lowest value in array
  for (int i = 0; i < 16; i++) {
    if (channels[i] == i) {
      lowest = i;
      break;
    }
  }

  // Assert lowest is not 16
  assert(lowest != 20);

  // Return lowest channel value not used
  return lowest;

} /* channel_asserts() */


/* This function
 * is a helper function to malloc and copy an event
 */

event_node_t *copy_event(event_node_t *input_current_event_node) {

  // Malloc for event node and assert then set next event to null
  event_node_t *copied_current_event_node = malloc(sizeof(event_node_t));
  assert(copied_current_event_node != NULL);
  copied_current_event_node->next_event = NULL;

  // Malloc for event of event node and assert
  copied_current_event_node->event = malloc(sizeof(event_t));
  assert(copied_current_event_node->event != NULL);
  copied_current_event_node->event->delta_time = input_current_event_node->event->delta_time;
  copied_current_event_node->event->type = input_current_event_node->event->type;

  // Malloc for event data of event and assert then copy
  event_t *copied_current_event = copied_current_event_node->event;
  event_t *input_current_event = input_current_event_node->event;

  // Based on type of event malloc for sys data
  if ((copied_current_event->type == SYS_EVENT_1) || (copied_current_event->type == SYS_EVENT_2)) {
    // For sys data
    copied_current_event->sys_event.data_len = input_current_event->sys_event.data_len;
    // If data len not 0 then malloc based on data len size
    if (copied_current_event->sys_event.data_len != 0) {
      // Malloc and assert
      copied_current_event->sys_event.data = malloc(input_current_event->sys_event.data_len);
      assert(copied_current_event->sys_event.data != NULL);
      // Reassign
      uint8_t *copy_data = copied_current_event->sys_event.data;
      uint8_t *original_data = input_current_event->sys_event.data;
      for (int i = 0; i < copied_current_event->sys_event.data_len; i++) {
        *copy_data = *original_data;
        copy_data++;
        original_data++;
      }
    }
  }
  else if (copied_current_event->type == META_EVENT) {
    // Copy name and data len
    copied_current_event->meta_event.name = input_current_event->meta_event.name;
    copied_current_event->meta_event.data_len = input_current_event->meta_event.data_len;
    // If data len not 0 then malloc for data based on data len size
    if (copied_current_event->meta_event.data_len != 0) {
      copied_current_event->meta_event.data = malloc(input_current_event->meta_event.data_len);
      assert(copied_current_event->meta_event.data != NULL);
      uint8_t* copy_data = copied_current_event->meta_event.data;
      uint8_t* original_data = input_current_event->meta_event.data;
      for (int i = 0; i < copied_current_event->meta_event.data_len; i++) {
        *copy_data = *original_data;
        copy_data++;
        original_data++;
      }
    }
  }
  else {
    // Copy name and data len
    copied_current_event->midi_event.name = input_current_event->midi_event.name;
    copied_current_event->midi_event.data_len = input_current_event->midi_event.data_len;
    // If data len not 0 then malloc for data based on data len size
    if (copied_current_event->midi_event.data_len != 0) {
      copied_current_event->midi_event.data = malloc(input_current_event->midi_event.data_len);
      assert(copied_current_event->midi_event.data != NULL);
      uint8_t* copy_data = copied_current_event->midi_event.data;
      uint8_t* original_data = input_current_event->midi_event.data;
      for (int i = 0; i < copied_current_event->midi_event.data_len; i++) {
        *copy_data = *original_data;
        copy_data++;
        original_data++;
      }
    }
  }

  // Return copied event node
  return copied_current_event_node;

} /* copy_event() */


/* This function
 * is a helper function to malloc and copy and return input track
 */

track_node_t *copy_track(track_node_t *input_track) {

  // Initialize variables
  track_node_t *copied = NULL;

  // Malloc for track node and assert then copy
  copied = malloc(sizeof(track_node_t));
  assert(copied != NULL);
  copied->next_track = NULL;

  // Malloc for track and assert then copy
  copied->track = malloc(sizeof(track_t));
  assert(copied->track != NULL);
  copied->track->length = input_track->track->length;
  copied->track->event_list = NULL;

  // Use while loop to malloc for all events and their data
  event_node_t *input_current_event_node = input_track->track->event_list;
  event_node_t *copied_head = NULL;
  event_node_t *copied_tail = NULL;

  while (input_current_event_node != NULL) {

    // Special case for head
    if (copied_head == NULL) {
      copied_head = copy_event(input_current_event_node);
      copied_tail = copied_head;
    }
    else {
      // Use helper function to get copied event node
      copied_tail->next_event = copy_event(input_current_event_node);
      copied_tail = copied_tail->next_event;
      copied_tail->next_event = NULL;
    }

    // Go to next event
    input_current_event_node = input_current_event_node->next_event;
  }

  // Set event list to point to head of copied event list
  copied->track->event_list = copied_head;

  // Return copied and malloc()ed track node
  return copied;

} /* copy_track() */


/* This function
 * creates a round of sorts using input song
 */

void add_round(song_data_t *song, int track_index, int octave_differential,
               unsigned int time_delay, uint8_t input_instrument) {

  // Assert statements
  assert(track_index < song->num_tracks);
  assert(song->format != 2);
  if (song->format == 0) {
    song->format = 1;
  }
  printf("total before tracks: %d\n", song->num_tracks);
  // Initalize variable
  int channel_message = channel_asserts(song);
  printf("lowest channel: %d\n", channel_message);
  int before_time = 0;
  int after_time = 0;
  int byte_difference = 0;
  int current_track_number = 0;
  int instruments_remapped = 0;
  int event_octaves_changed = 0;
  track_node_t *current_track = song->track_list;
  track_node_t *track_to_copy = NULL;
  remapping_t NEW_INSTRUMENT = {0, };

  // Use while loop to find right index track to copy
  while (1) {
    // If at right index then malloc and copy
    if (current_track_number == track_index) {
      // Use helper function to malloc and copy
      track_to_copy = copy_track(current_track);
    }
    // If at tail stop
    if (current_track->next_track == NULL) {
      break;
    }
    // Else go to next track
    current_track = current_track->next_track;
    current_track_number++;
  }

  // Increase song's track number and set next track to newly copied track
  song->num_tracks++;
  current_track->next_track = track_to_copy;
  printf("total after tracks: %d\n", song->num_tracks);
  // Add time delay to first event and get byte difference
  before_time = track_to_copy->track->event_list->event->delta_time;
  after_time = track_to_copy->track->event_list->event->delta_time + time_delay;
  byte_difference = bytes_needed_for_conversion(before_time, after_time);
  track_to_copy->track->event_list->event->delta_time += time_delay;
  // Add byte difference to total track len
  track_to_copy->track->length += byte_difference;

  // Build mapping table for new instrument and remap instrument of events
  for (int i = 0; i <= 0xFF; i++) {
    NEW_INSTRUMENT[i] = input_instrument;
  }

  // Change all midi channels to use lowest unused channel
  int event_number = 1;
  event_node_t *current_event_node = track_to_copy->track->event_list;
  while (current_event_node != NULL) {

    // Check that midi event and change status
    if ((current_event_node->event->type >= 0x80) && (current_event_node->event->type <= 0xEF)) {
      printf("Event #%d old status: %d ", event_number, current_event_node->event->midi_event.status);
      int new_status = current_event_node->event->midi_event.status;
      new_status = new_status & 0x0F;
      new_status = new_status | channel_message;
      current_event_node->event->type = new_status;
      current_event_node->event->midi_event.status = new_status;
      printf("new status: %d new type: %d\n", current_event_node->event->midi_event.status, current_event_node->event->type);
    }

    // Change event octaves of track
    event_octaves_changed += change_event_octave(current_event_node->event, &octave_differential);
    instruments_remapped += change_event_instrument(current_event_node->event, NEW_INSTRUMENT);

    // Go to next event node
    event_number++;
    current_event_node = current_event_node->next_event;
  }


} /* add_round() */


/*
 * Function called prior to main that sets up random mapping tables
 */

void build_mapping_tables()
{
  for (int i = 0; i <= 0xFF; i++) {
    I_BRASS_BAND[i] = 61;
  }

  for (int i = 0; i <= 0xFF; i++) {
    I_HELICOPTER[i] = 125;
  }

  for (int i = 0; i <= 0xFF; i++) {
    N_LOWER[i] = i;
  }
  //  Swap C# for C
  for (int i = 1; i <= 0xFF; i += 12) {
    N_LOWER[i] = i-1;
  }
  //  Swap F# for G
  for (int i = 6; i <= 0xFF; i += 12) {
    N_LOWER[i] = i+1;
  }
} /* build_mapping_tables() */
