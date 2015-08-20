<?php

/**
 *  Handles bulk processing of nodes for opencalais.
 *
 *  Makes use of the bulk operations api.
 */
 
/**
 *  Create a queue of all the nodes that need to be tagged, 
 *  This will allow for cron to process the nodes
 */
function opencalais_create_queue(){
  $queue = DrupalQueue::get('opencalais_tagging_queue');
  $queue->createQueue();
}

/**
 * Remove all content type items from queue.
 *
 * @param string $content_type
 *   The node (content) type for to remove from the queue.
 */
function opencalais_queue_remove_items($content_type) {
  $items_removed = 0;
  $result = db_select('queue', 'q')
    ->fields('q') 
    ->condition('q.name', 'opencalais_tagging_queue')
    ->execute();
  while ($qdata = $result->fetchObject()) {
    $data = unserialize($qdata->data);
    $iid = $qdata->item_id;
    
    $node = db_select('node', 'n')
      ->fields('n')
      ->condition('n.nid', $data['nid'])
      ->execute()
      ->fetchObject();
    if ($node->type == $content_type) {
      $items_removed++;
      db_delete('queue')
        ->condition('item_id', $iid)
        ->execute();
    }
  }
  
  // Clear out the tracking system variable
  $last_added = variable_get('opencalais_processed', array());
  if (!empty($last_added[$content_type])) {
    unset($last_added[$content_type]);
  }
  variable_set('opencalais_processed', $last_added);
  
  drupal_set_message($items_removed . ' Items removed', 'status');
  drupal_goto('admin/config/content/opencalais/bulk');
}

/**
 * Purge the opencalais bulk processing queue. All of it. Everything. The whole
 * enchilada.
 */
function opencalais_queue_remove_all() {
  db_delete('queue')
    ->condition('name', 'opencalais_tagging_queue')
    ->execute();
  variable_set('opencalais_processed', array());
}

/**
 * Add nodes of a specific content type to the queue.
 *
 * @param string $content_type
 *   The node (content) type for to added to the queue.
 *
 * @return int $count
 *   The number of nodes added to the queue.
 */
function opencalais_queue_add_items($content_type) {
  // Get the last loaded nid for this content type or 0 if not in queue.
  $last_added = variable_get('opencalais_processed', array());
  $last_added = isset($last_added[$content_type]) ? $last_added[$content_type] : 0;
  
  $queue = DrupalQueue::get('opencalais_tagging_queue');
  
  $nids = db_select('node', 'node')
    ->fields('node', array('nid'))
    ->condition('type', $content_type, '=')
    ->condition('nid', $last_added, '>')
    ->orderBy('nid')
    ->execute();

  $count = 0;  
  foreach ($nids as $nid) {
    $nid = $nid->nid;
    // Add an item - going to make this an assoc array in case we need to add more data later
    $queue->createItem(array('nid' => $nid));
    $count++;
  }
  
  // I am not crazy about this. We set the maximum at the time of queueing instead of
  // at the time it is processed. Our clear function should mitigate the need to change
  // this, but it is something to keep an eye on.
  if (isset($nid)) {
    $processed = variable_get('opencalais_processed', array());
    $processed[$content_type] = $nid;
    variable_set('opencalais_processed', $processed);
  }
  return $count;
}

/**
 * Process a queue item.
 *
 * Load the node and resave it - autotagging stuff in presave will take care of the rest
 *
 * @param object $data
 *   The item retrieved from the cache.
 *
 * @return bool $success
 *   Whether or not the process was successful.
 */ 
function opencalais_queue_process_node($data){
  $nid = $data->data['nid'];
  try { 
    // We need to set this so that ARC2 will work and be able to connect with 
    // the OpenCalais service.
    if (!isset($_SERVER['SERVER_NAME'])) {
      global $base_url;
      $parts = parse_url($base_url);
      $_SERVER['SERVER_NAME'] = $parts['host'];
      $_SERVER['SERVER_PORT'] = 80;
    }
    $node = node_load($nid);
    node_save($node);
  } catch (Exception $e) {
    opencalais_set_message(t('Could not process node with id %id. You may try to manually resave the node to resolve this issue.', array('%id' => $nid)), 'error');
    return FALSE;
  }
  return TRUE;
}

/**
 * Utility function to get the current number of nodes remaining.
 * 
 * @return string $result
 *   An allready translated string of the results of queue progress.
 */
function opencalais_queue_get_progress() {
  $queue = DrupalQueue::get('opencalais_tagging_queue');
  $items = $queue->numberOfItems();
  
  // Only show what we have remaining in the queue.
  if ($items > 0) {
    $process_count = variable_get('opencalais_bulk_processing_count', 25);
    return t('%n nodes remaining in the queue. These items will be processed %max at a time during cron runs.', array('%n'=> $items, '%max' => $process_count));
  }
}

/**
 * Function to be called inside of a hook_cron (if auto bulk processing is enabled).
 * Processed queued nodes as per our configured rate
 */
function opencalais_queue_cron() {
  $queue = DrupalQueue::get('opencalais_tagging_queue');
  $count = variable_get('opencalais_bulk_processing_count', 25);
  while ($count--) {
    $item = $queue->claimItem(15);
    if (!$item) {
      return; 
    }
    else if (opencalais_queue_process_node($item)) {
      $queue->deleteItem($item);
    }
  }
}

/**
 * Menu Callback to add items for a content type and then go back to the bulk 
 * processing page.
 *
 * @param string $type
 *   The content type we're queueing.
 */
function opencalais_add_items_to_queue_callback($type){
  $valid_types = _opencalais_get_auto_content_types();
  if (!in_array($type, $valid_types)) {
    drupal_set_message(t('Invalid Content Type for Bulk Processing - Either %type is an invalid content type or the content type is not set to automatic tagging.', array('%type'=>$type)), 'error');
  } 
  else {
    $processed = opencalais_queue_add_items($type);
    drupal_set_message(t('%n nodes successfully queued for processing.', array('%n' => $processed)));
  }
  drupal_goto('admin/config/content/opencalais/bulk');
}

/**
 * Create a form for bulk processing
 */
function opencalais_bulk_operations_form() {
  //print out any messages we have
  opencalais_get_messages();

  $form = array();
  $available_types = _opencalais_get_auto_content_types();
  
  drupal_set_message(opencalais_queue_get_progress(), 'status');
  
  $form['types'] = array(
    '#type' => 'fieldset',
    '#attributes' => array(
      'style' => 'padding: 1.5em 0 0',
    ),
  );
  
  $form['types']['message'] = array(
    '#markup' => '<h2 style="padding: 0px; margin: 0px;">Bulk Operations</h1>Content types marked for autotagging will be shown below. You can queue all the currently created nodes for processing by clicking the corresponding link below. OpenCalais will keep track of the latest nodes (per content-type) that it has processed to avoid processing them again.<br /><br /><ul>',
  );
  
  foreach ($available_types as $type) {
    $form['types']['opencalais_queue_content_type_'.$type] = array(
      '#markup' => '<li><b>' . $type . '</b>: ' . 
        l('Process All Remaining', 'admin/config/content/opencalais/bulk/' . $type, array('absolute' => TRUE, 'class' => array('button'))) . ' - ' .
        l('Remove All In Queue', 'admin/config/content/opencalais/bulk-clear/' . $type, array('absolute' => TRUE, 'class' => array('button'))) . ' - ' .
        l('Remove Tags & Re-Queue', 'admin/config/content/opencalais/bulk-re-queue/' . $type, array('absolute' => TRUE, 'class' => array('button'))) . '</div>',
    );
  }
  return $form;
}

/**
 * Utility function to get an array of the content types that have autotagging turned on
 */
function _opencalais_get_auto_content_types() {
  $auto_settings = variable_get('opencalais_autotagging', array());
  return array_keys(array_filter($auto_settings));
} 
  
/**
 * Utility function to create opencalais specific message queue
 */ 
function opencalais_set_message($message, $type = 'status') {
  $queue = variable_get('opencalais_message_queue', array());
  $queue[$type][] = $message;
  variable_set('opencalais_message_queue', $queue);
} 

/**
 * Utility function to print out all the opencalais messages
 */
function opencalais_get_messages() {
  $queue = variable_get('opencalais_message_queue', array());
  
  foreach ($queue as $type => $messages) {
    foreach ($messages as $message) {
      drupal_set_message($message, $type);
    }
  }
  //empty the queue
  variable_set('opencalais_message_queue', array());
}

/**
 * A function to re-queue the content type for indexing.
 *
 * @param string $type
 *   The node type to clear and re-queue
 */
function opencalais_queue_reset($type) {
  // Reset the content type in our message queue variable. Do this by clearing the high
  // node id (nid) in the system variable.
  $last_added = variable_get('opencalais_processed', array());
  if (!empty($last_added[$type])) {
    unset($last_added[$type]);
  }
  variable_set('opencalais_processed', $last_added);
  
  // Re-add all of the items to the queue.
  opencalais_queue_add_items($type);
  
  // Inform on what we have done.
  drupal_set_message('The ' . $type . ' node type has been re-queued for bulk processing');

  drupal_goto('admin/config/content/opencalais/bulk');
}