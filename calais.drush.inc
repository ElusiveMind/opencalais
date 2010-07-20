<?php
/**
 * @file
 *   drush integration for opencalais.
 */

/**
 * Implementation of hook_drush_command().
 *
 * @See drush_parse_command() for a list of recognized keys.
 *
 * @return
 *   An associative array describing your command(s).
 */
function calais_drush_command() {
  $items = array();

  $items['calais-reindex'] = array(
    // the name of the function implementing your command.
    'callback' => 'calais_drush_reindex',
    // a short description of your command
    'description' => dt('Marks content for reindexing. Takes content types as parameters.'),
    'arguments' => array(
      'types' => dt('A space delimited list of content types to be marked for reindexing.'),
    ),
  );
  $items['calais-process'] = array(
    'callback' => 'calais_drush_process',
    'description' => dt('Bulk process nodes with Calais. Takes node count and content types as parameters'),
    'arguments' => array(
      'count' => dt('Optional. Default: 25. The number of nodes to process on this Drush execution.'),
      'types' => dt('Optional. Default: All queued. An individual content type to be processed from the queue.'),
    ),
  );
  return $items;
}

/**
 * Implementation of hook_drush_help().
 *
 * This function is called whenever a drush user calls
 * 'drush help <name-of-your-command>'
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return
 *   A string with the help text for your command.
 */
function calais_drush_help($section) {
  switch ($section) {
    case 'drush:calais-reindex':
      return dt("Used with parameter for content types, it queues the content types that are specified for reprocessing. All scheduled nodes will be processed with Calais on future cron or Drush runs.");
    case 'drush:calais-process':
      return dt("Bulk process nodes with Calais. Can be specified with a count and a content type");
  }
}

/**
 * Prepare all nodes of the specified content types with Calais.
 */
function calais_drush_reindex() {
  $args = func_get_args();
  if (count($args) > 0) {
    foreach ($args as $type) {
      calais_build_queue($type);
    }
  }
  else {
    drush_log(dt('No content types were specific'), 'error');
    return;
  }
}

/**
 * Bulk process nodes with Calais.
 */
function calais_drush_process() {  
  $args = func_get_args();  
  $argc = count($args);
  
  $count = 25;
  $type = NULL;
  
  if ($argc === 2) {
    $count = intval($args[0]);
    $type = $args[1];
  }
  else if ($argc === 1) {
    $arg = $args[0];
    if (is_numeric($arg)) {
      $count = intval($arg);
    }
    else {
      $type = $arg;
    }
  }

  if (isset($type) && node_get_types('name', $type) === FALSE) {
    drush_log(dt('Invalid content type: @type', array('@type' => $type)), 'error');
    return;
  }

  calais_process_queue($count, $type);    
}
