/* Kartik Uppalapati, library.c, CS 24000, Fall 2020
 * Last updated October 12, 2020
 */

/* Add any includes here */
#include "library.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <malloc_debug.h>


// Global variable
tree_node_t *g_song_library = NULL;


/* This function
 * finds the parent pointer of a tree node that has song name
 */

tree_node_t **find_parent_pointer(tree_node_t **input_tree,
                                 const char *song_name) {

  // If root is null
  if ((*input_tree) == NULL) {
    return NULL;
  }

  // If song found
  if (strcmp((*input_tree)->song_name, song_name) == 0) {
    return input_tree;
  }

  // If song not found go left
  if (strcmp((*input_tree)->song_name, song_name) > 0) {
    return find_parent_pointer(&((*input_tree)->left_child), song_name);
  }
  // Else go right
  else {
    return find_parent_pointer(&((*input_tree)->right_child), song_name);
  }


} /* find_parent_pointer() */


/* This function
 * inserts a tree node into tree at approporiate place
 */

int tree_insert(tree_node_t **input_tree, tree_node_t *tree_to_add) {

  // Check if input tree null
  if ((*input_tree) == NULL) {
    (*input_tree) = tree_to_add;
    return INSERT_SUCCESS;
  }

  // Initialize variables
  tree_node_t **current = input_tree;

  // Check for duplicates
  if (find_parent_pointer(input_tree, tree_to_add->song_name) != NULL) {
    return DUPLICATE_SONG;
  }


  // Use if statement to insert at appropriate place
  if (strcmp((*current)->song_name, tree_to_add->song_name) > 0) {
    if ((*current)->left_child == NULL) {
      (*current)->left_child = tree_to_add;
      return INSERT_SUCCESS;
    }
    else {
      tree_insert(&((*current)->left_child), tree_to_add);
    }
  }
  else {
    if ((*current)->right_child == NULL) {
      (*current)->right_child = tree_to_add;
      return INSERT_SUCCESS;
    }
    else {
      tree_insert(&((*current)->right_child), tree_to_add);
    }
  }


} /* tree_insert() */


/* This function
 * removes a tree node from tree
 */

int remove_song_from_tree(tree_node_t **input_tree, const char *song_name) {

  // Initialize variables
  tree_node_t **location_found = NULL;
  tree_node_t *right_child_holder = NULL;
  tree_node_t *left_child_holder = NULL;

  // Check that song is in tree
  location_found = find_parent_pointer(input_tree, song_name);
  if (location_found == NULL) {
    return SONG_NOT_FOUND;
  }

  // If right and left childs not null assign them to temp variable
  if ((*location_found)->right_child != NULL) {
    right_child_holder = (*location_found)->right_child;
  }
  if ((*location_found)->left_child != NULL) {
    left_child_holder = (*location_found)->left_child;
  }

  // Free node and remove
  free_node(*location_found);
  *location_found = NULL;

  // Reinsert left and right child back into tree
  if (right_child_holder != NULL) {
    tree_insert(input_tree, right_child_holder);
  }
  if (left_child_holder != NULL) {
    tree_insert(input_tree, left_child_holder);
  }


  // Return delete success
  return DELETE_SUCCESS;

} /* remove_song_from_tree() */


/* This function
 * frees an input_node
 */

void free_node(tree_node_t *input_node) {

  // Free malloc()ed space for song of input_node
  free_song(input_node->song);

  // Free malloc()ed space for node
  free(input_node);


} /* free_node() */


/* This function
 * prints the song name of a input node to the file
 */

void print_node(tree_node_t *input_node, FILE *to_write) {

  // Initialize variables
  int fprintf_val = 0;

  // Use fprintf to write to file and assert done properly
  fprintf_val = fprintf(to_write, "%s\n", input_node->song_name);
  assert(fprintf_val >= 0);

} /* print_node() */


/* This function
 * takes an input node and traverses it in pre orders
 */

void traverse_pre_order(tree_node_t *input_node, void *arbitrary,
                        traversal_func_t to_do) {

  // If tree node null do nothing
  if (input_node == NULL) {
    return;
  }

  // Perform operation using parameter
  to_do(input_node, arbitrary);

  // Go to left child recursively
  traverse_pre_order(input_node->left_child, arbitrary, to_do);

  // Go to right child recursively
  traverse_pre_order(input_node->right_child, arbitrary, to_do);


} /* traverse_pre_order() */


/* This function
 * takes an input node and traverses it in order
 */

void traverse_in_order(tree_node_t *input_node, void *arbitrary,
                       traversal_func_t to_do) {

  // If tree node null do nothing
  if (input_node == NULL) {
    return;
  }

  // Go to left child recursively
  traverse_in_order(input_node->left_child, arbitrary, to_do);

  // Perform operation using parameter
  to_do(input_node, arbitrary);

  // Go to right child recursively
  traverse_in_order(input_node->right_child, arbitrary, to_do);

} /* traverse_in_order() */


/* This function
 * takes an input node and traverses it in post order
 */

void traverse_post_order(tree_node_t *input_node, void *arbitrary,
                         traversal_func_t to_do) {

  // If tree node null do nothing
  if (input_node == NULL) {
    return;
  }

  // Go to right child recursively
  traverse_post_order(input_node->right_child, arbitrary, to_do);

  // Perform operation using parameter
  to_do(input_node, arbitrary);

  // Go to left child recursively
  traverse_post_order(input_node->left_child, arbitrary, to_do);


} /* traverse_post_order() */


/* This function
 * frees the library of all allocated memory
 */

void free_library(tree_node_t *library) {

  // If library null do nothing
  if (library == NULL) {
    return;
  }

  // Else free left and right child
  free_library(library->right_child);
  free_library(library->left_child);

  // Free tree_node
  free_node(library);

} /* free_library() */


/* This function
 * writes the names of all songs in proper order to file
 */

void write_song_list(FILE *fp, tree_node_t *input_tree) {

  // Use other functions to write to file
  traverse_in_order(input_tree, fp, (traversal_func_t) &print_node);


} /* write_song_list() */


/* This function
 * takes in a file path and converts the directory inot a song library
 */

void make_library(const char *file_path) {



} /* make_library() */


