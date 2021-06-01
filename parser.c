/* Kartik Uppalapati, parser.c, CS 24000, Fall 2020
 * Last updated October 12, 2020
 */

/* Add any includes here */
#include "parser.h"
#include <stdio.h>
#include <assert.h>
#include <malloc_debug.h>
#include <string.h>

// Initialize global variable
uint8_t g_prev_event_status = 0;

/* This function
 * takes in a MIDI file and parses it
 */

song_data_t *parse_file(const char *file_path) {

  // Assert input not null
  assert(file_path != NULL);

  // Open file for reading
  FILE *fptr = NULL;
  fptr = fopen(file_path, "rb");
  assert(fptr != NULL);

  // Use fseek and ftell to get end of file value then reset fseek
  fseek(fptr, 0, SEEK_END);
  int file_len = ftell(fptr);
  fseek(fptr, 0, SEEK_SET);

  // Create empty song_data_t struct
  song_data_t *song = malloc(sizeof(song_data_t));
  assert(song != NULL);

  // Malloc space for path in song and strcpy
  song->path = malloc(strlen(file_path) + 1);
  strcpy(song->path, file_path);

  // Use parse functions to parse file
  parse_header(fptr, song);

  // Use while loop and for loop to parse all events and tracks
  for (int i = 0; i < song->num_tracks; i++) {
    // Parse track
    parse_track(fptr, song);
  }

  // Assert that entire file has been read
  int current_position = ftell(fptr);
  assert(current_position == file_len);

  // Close file ptr
  fclose(fptr);
  fptr = NULL;

  // Return song struct
  return song;

} /* parse_file() */


/* This function
 * parses the header of a midi file
 */

void parse_header(FILE *in_file, song_data_t *input_song) {

  // Set file ptr to start of file
  fseek(in_file, 0, SEEK_SET);
  int fread_val = 0;
  char chunk_type_read[5] = "";
  uint8_t length_read[4] = {0, 0, 0, 0};
  uint32_t length_endian_swapped = 0;
  uint8_t format_read[2] = {0, 0};
  uint16_t format_to_input = 0;
  uint8_t num_tracks_read[2] = {0, 0};
  uint16_t num_tracks_to_input = 0;
  uint8_t division_read[2] = {0, 0};
  uint16_t division_to_input = 0;

  // Read chunk type
  fread_val = fread(chunk_type_read, 1, 4, in_file);
  assert(fread_val == 4);

  // Read length and endian swap
  fread_val = fread(&length_read, sizeof(uint8_t), 4, in_file);
  assert(fread_val == 4);
  length_endian_swapped = end_swap_32(length_read);

  // Read format and endian swap
  fread_val = fread(&format_read, sizeof(uint8_t), 2, in_file);
  assert(fread_val == 2);
  format_to_input = end_swap_16(format_read);

  // Read number of tracks and endian swap
  fread_val = fread(&num_tracks_read, sizeof(uint8_t), 2, in_file);
  assert(fread_val == 2);
  num_tracks_to_input = end_swap_16(num_tracks_read);
  
  // Read division
  fread_val = fread(&division_read, sizeof(uint8_t), 2, in_file);
  assert(fread_val == 2);
  division_to_input = end_swap_16(division_read);

  // Assert values are valid
  assert(strcmp(chunk_type_read, "MThd") == 0);
  assert(length_endian_swapped == 6);
  assert((format_to_input == 0) ||
         (format_to_input == 1) ||
         (format_to_input == 2));

  // If format == 0 then assert num tracks 1
  if (format_to_input == 0) {
    assert(num_tracks_to_input == 1);
  }

  // If division uses tpq byte shift and assign
  if ((division_to_input >> 7) & 1) {
    input_song->division.uses_tpq = true;
    input_song->division.ticks_per_qtr = division_to_input & 0x7FFF;
  }
  // Else division doesn't use tpq
  else {
    input_song->division.uses_tpq = false;
    // Assign values to struct
    input_song->division.ticks_per_frame = division_to_input & 0xFF;
    input_song->division.frames_per_sec = division_to_input & 0xFE;
  }

  // Assign values to input_song struct
  input_song->format = format_to_input;
  input_song->num_tracks = num_tracks_to_input;

  // Set track list to null
  input_song->track_list = NULL;

} /* parse_header() */


/* Helper function
 * to add a event node with event to track node event list
 */

void add_all_events(track_node_t *input_track_node, FILE *fptr) {

  // Create temp variable to keep track of current event node
  event_node_t *current_event_node = input_track_node->track->event_list;
  while (1) {
    // Special case for head
    if (current_event_node == NULL) {
      // Malloc for event node
      event_node_t *event_node_to_add = malloc(sizeof(event_node_t));
      assert(event_node_to_add != NULL);
      event_node_to_add->next_event = NULL;
      event_node_to_add->event = parse_event(fptr);
      // Set event list to newly malloc()ed head event node
      input_track_node->track->event_list = event_node_to_add;
      current_event_node = input_track_node->track->event_list;
    }
    // Else if at tail then add to tail
    else {
      // Malloc for event node
      event_node_t *event_node_to_add = malloc(sizeof(event_node_t));
      assert(event_node_to_add != NULL);
      event_node_to_add->next_event = NULL;
      event_node_to_add->event = parse_event(fptr);
      // Set event list to newly malloc()ed head event node
      current_event_node->next_event = event_node_to_add;
      current_event_node = current_event_node->next_event;
    }
    // If event last event than stop while loop
    if (strcmp(current_event_node->event->meta_event.name, "End of Track") == 0) {
      break;
    }
    // Else go to next event node
    //current_event_node = current_event_node->next_event;
  }

} /* add_all_events() */


/* Helper function
 * to add track node to track list
 */

void add_track_node_to_list(track_node_t *track_node_to_add, song_data_t *input_song) {

  // Add to track list
  track_node_t *current_track_node = input_song->track_list;
  while (1) {
    // Special case for head
    if (current_track_node == NULL) {
      input_song->track_list = track_node_to_add;
      break;
    }
    // if at tail then add to tail
    if (current_track_node->next_track == NULL) {
      current_track_node->next_track = track_node_to_add;
      break;
    }
    // Else go to next track
    current_track_node = current_track_node->next_track;
  }

} /* add_track_to_list() */


/* This function
 * reads a file and updates the track
 */

void parse_track(FILE *in_file, song_data_t *input_song) {

  // Initialize variables
  int fread_val = 0;
  char track_chunk_type_read[5] = "";
  uint8_t track_length_read[4] = {0, 0, 0, 0};
  uint32_t track_length_to_input = 0;

  // Read track data from file and assert that type is MTrk
  fread_val = fread(track_chunk_type_read, 1, 4, in_file);
  assert(fread_val == 4);
  assert(strcmp(track_chunk_type_read, "MTrk") == 0);

  // Read track length
  fread_val = fread(&track_length_read, sizeof(uint8_t), 4, in_file);
  assert(fread_val == 4);
  track_length_to_input = end_swap_32(track_length_read);

  // Malloc space for track and track list
  track_node_t *track_node_to_add = malloc(sizeof(track_node_t));
  assert(track_node_to_add != NULL);
  track_node_to_add->track = malloc(sizeof(track_t));
  assert(track_node_to_add->track != NULL);

  // Set track length and next event of track node
  track_node_to_add->track->length = track_length_to_input;
  track_node_to_add->track->event_list = NULL;
  track_node_to_add->next_track = NULL;


  add_all_events(track_node_to_add, in_file);
  add_track_node_to_list(track_node_to_add, input_song);

} /* parse_track() */


/* This function
 * reads a file and returns a event_t
 */

event_t *parse_event(FILE *in_file) {

  // Initialize variables
  event_t *event = malloc(sizeof(event_t));
  assert(event != NULL);
  uint32_t delta_time_read = parse_var_len(in_file);
  uint8_t type_read = 0;
  int fread_val = 0;

  // Read in type and assert fread_val
  fread_val = fread(&type_read, 1, 1, in_file);
  assert(fread_val == 1);

  // Assign read in variables to event struct
  event->delta_time = delta_time_read;
  // Use if statments to determine type of event
  if (type_read == 0xFF) {
    event->type = META_EVENT;
    event->meta_event = parse_meta_event(in_file);
  }
  else if (type_read == 0xF0) {
    event->type = SYS_EVENT_1;
    event->sys_event = parse_sys_event(in_file);
  }
  else if (type_read == 0xF7) {
    event->type = SYS_EVENT_2;
    event->sys_event = parse_sys_event(in_file);
  }
  else {
    // For midi event use set event type to status
    event->type = type_read;
    event->midi_event = parse_midi_event(in_file, event->type);
  }


  // Return event
  return event;

} /* parse_event() */


/* This function
 * parses the file and returns a sys event
 */

sys_event_t parse_sys_event(FILE *in_file) {

  // Initalize variables
  sys_event_t sys = {0, 0};
  int fread_val = 0;

  // Read vlq for data_len and malloc() for sys.data based on data_len
  sys.data_len = parse_var_len(in_file);
  sys.data = malloc((int32_t) sys.data_len);
  assert(sys.data != NULL);

  // Use vlq for length to read in event data
  fread_val = fread(sys.data, (int32_t) sys.data_len, 1, in_file);
  assert(fread_val == 1);


  // Return sys event
  return sys;

} /* parse_sys_event() */


/* This function
 * parses a midi file and turns data into meta event struct
 */

meta_event_t parse_meta_event(FILE *in_file) {

  // Initialize variables
  meta_event_t meta = {"", 0, 0};
  int fread_val1 = 0;
  int fread_val2 = 0;
  uint8_t meta_type_read = 0;

  // Read type
  fread_val1 = fread(&meta_type_read, 1, 1, in_file);
  assert(fread_val1 == 1);

  // Use META_TABLES to find default meta table and assign
  meta = META_TABLE[meta_type_read];
  assert(meta.name != NULL);

  // If meta.data_len == 0 then read vlq as data_len
  if (meta.data_len == 0) {
    // Read vlq for data_len and malloc() for sys.data based on data_len
    meta.data_len = parse_var_len(in_file);
  }
  else {
    // Else assert that default data_len == read in vlq from file
    assert(meta.data_len == parse_var_len(in_file));
  }

  // If data len 0 then read nothing and return meta event
  if (meta.data_len == 0) {
    return meta;
  }

  // Else malloc() data_len bytes into meta data
  meta.data = malloc(meta.data_len);
  assert(meta.data != NULL);

  // Read data_len bytes into data
  fread_val2 = fread(meta.data, meta.data_len, 1, in_file);
  assert(fread_val2 == 1);


  // Return meta event
  return meta;

} /* parse_meta_event() */


/* This function
 * parses file and returns a midi struct
 */

midi_event_t parse_midi_event(FILE *in_file, uint8_t current_event_status) {

  // Initialize variables
  midi_event_t midi = {"", 0, 0, 0};
  int fread_val = 0;

  // If status byte is explicit
  if ((current_event_status & 0x80) != 0) {
    // Use current event status to find defualt in midi table and assert
    midi = MIDI_TABLE[current_event_status];
    // If midi.data_len not = 0 then malloc() and read in data from file
    if (midi.data_len != 0) {
      // Malloc() and assert
      midi.data = malloc(midi.data_len);
      assert(midi.data != NULL);
      // Read and assert
      fread_val = fread(midi.data, 1, midi.data_len, in_file);
      //assert(fread_val == 1);
    }
  }
  // If status byte implicit
  else {
    // Use prev event status byte to find default in midi table and assert
    midi = MIDI_TABLE[g_prev_event_status];
    // If midi.data_len not = 0 then malloc() and read in data from file
    if (midi.data_len != 0) {
      // Malloc() and assert
      midi.data = malloc(midi.data_len);
      assert(midi.data != NULL);
      midi.data[0] = current_event_status;
      // Read and assert
      if (midi.data_len != 1) {
        fread_val = fread(&midi.data[1], 1, midi.data_len - 1, in_file);
        //assert(fread_val == 1);
      }
    }
  }

  // Reset global variable
  g_prev_event_status = midi.status;

  // Return midi sruct
  return midi;

} /* parse_midi_event() */


/* This function
 * reads var length from file input
 */

uint32_t parse_var_len(FILE *in_file) {

  // Initialize variables
  int fread_val = 0;
  uint32_t total_val = 0;
  uint8_t cur_val = 0;
  uint8_t copy_val = 0;

  // Use do while loop to read byte by byte
  do {
    // Read first byte and assert fread val
    fread_val = fread(&cur_val, sizeof(uint8_t), 1, in_file);
    assert(fread_val == 1);
    copy_val = cur_val;
    // Byte shift
    copy_val &= ~(1 << 7);
    total_val = (total_val << 7) + copy_val;
  } while (cur_val & (1 << 7));

  // Return value
  return total_val;

} /* parse_var_len() */


/* This function
 * returns event type of input event
 */

uint8_t event_type(event_t *event) {

  // Get event type
  uint8_t event_type = event->type;

  // Compare to find type of event and return
  if ((event_type == 0xF0) || (event_type == 0xF7)) {
    return SYS_EVENT_T;
  }
  else if (event_type == 0xFa) {
    return MIDI_EVENT_T;
  }
  else {
    return META_EVENT_T;
  }

} /* event_type() */


/* This function
 * frees a song_data_t struct
 */

void free_song(song_data_t *song) {

  // Free all tracks and events associated with track
  track_node_t *current_track = song->track_list;
  track_node_t *track_to_free = NULL;
  while (1) {
    // If current track only track left
    if (current_track->next_track == NULL) {
      free_track_node(current_track);
      break;
    }
    // Else set current track to next track and free previous
    track_to_free = current_track;
    current_track = current_track->next_track;
    free_track_node(track_to_free);
  }

  // Free malloc()ed space in song struct
  free(song->path);
  free(song);


} /* free_song() */


/* This function
 * frees a track from track list
 */

void free_track_node(track_node_t *to_free) {

  // Free all events associatd with track
  event_node_t *current_event = to_free->track->event_list;
  event_node_t *event_to_delete = NULL;
  while (1) {
    // If current event last event to free then free and break
    if (current_event->next_event == NULL) {
      free_event_node(current_event);
      break;
    }
    // Else set current to next event and free previous
    event_to_delete = current_event;
    current_event = current_event->next_event;
    free_event_node(event_to_delete);
  }

  // Free track data
  free(to_free->track);
  free(to_free);


} /* free_track_node() */


/* This function
 * frees an event node from event_list
 */

void free_event_node(event_node_t *to_free) {

  // Free sys/midi/meta events using if statement
  uint8_t type = event_type(to_free->event);
  if (type == SYS_EVENT_T) {
    // If sys event free data
    free(to_free->event->sys_event.data);
  }
  else if (type == META_EVENT_T) {
    // If meta event free name and data
    free(to_free->event->meta_event.data);
  }
  else if (type == MIDI_EVENT_T) {
    // If midi event free name and data
    free(to_free->event->midi_event.data);
  }
  else {
    // Do nothing
  }

  // Free event
  free(to_free->event);
  free(to_free);

} /* free_event_node() */


/* This function
 * takes two uint8_ts and converts them into uint16_t
 */

uint16_t end_swap_16(uint8_t to_swap[2]) {

  // Initialize variable
  uint16_t converted_var = ((uint16_t)to_swap[0] << 8) | to_swap[1];

  return converted_var;

} /* end_swap_16() */


/* This function
 * takes 4 uint8_ts and converts them into a uint32_t
 */

uint32_t end_swap_32(uint8_t to_swap[4]) {

  // Initialize variable
  uint32_t converted_var = to_swap[3] | (to_swap[2] << 8) |
                           (to_swap[1] << 16) | (to_swap[0] << 24);

  return converted_var;

} /* end_swap_32() */


