<?php
/**
 *  Handles bulk processing of nodes for opencalais.
 *
 *  Makes use of the bulk operations api
 */

//Controls how many nodes will be processed per cron run
define('OPENCALAIS_PROCESS_COUNT', 25);
 
/**
 *  Create a queue of all the nodes that need to be tagged, 
 *  This will allow for cron to process the nodes
 */
function opencalais_create_queue(){
  $queue = DrupalQueue::get('opencalais_tagging_queue');
  $queue->createQueue();
}

/**
 *  Add items to a queue
 */
function opencalais_queue_add_items($content_type){
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
  foreach($nids as $nid){
    $nid = $nid->nid;
    //add an item - gonna make this an assoc array in case we need to add more data later
    $queue->createItem(array('nid' => $nid));
    $count++;
  }
  if(isset($nid)){
    $processed = variable_get('opencalais_processed', array());
    $processed[$content_type] = $nid;
    variable_set('opencalais_processed', $processed);
  }
  return $count;
}

/**
 *  Process a queue item
 *  Load the node and resave it - autotagging stuff in presave will take care of the rest
 */ 
function opencalais_queue_process_node($data){
  $nid = $data->data['nid'];
  try { 
    //We need to set this so that ARC2 will work and be able to connect with the OpenCalais service
    if(!isset($_SERVER['SERVER_NAME'])){
      global $base_url;
      $parts = parse_url($base_url);
      $_SERVER['SERVER_NAME'] = $parts['host'];
      $_SERVER['SERVER_PORT'] = 80;
    }
    $node = node_load($nid);
    node_save($node);
  } catch(Exception $e){
    opencalais_set_message(t('Could not process node with id %id. You may try to manually resave the node to resolve this issue.', array('%id'=>$nid)), 'error');
    return FALSE;
  }
  return TRUE;
}

/**
 *  Utility function to get the current number of nodes remaining.
 *  returns an already translated string.
 */
function opencalais_queue_get_progress(){
  $queue = DrupalQueue::get('opencalais_tagging_queue');
  $items = $queue->numberOfItems();
  
  if($items == 0){
    return t('All queued nodes have been processed.');
  }
  else {
    return t('%n nodes remaining in the queue.  These items will be processed %max at a time during cron runs.', array('%n'=> $items, '%max'=>OPENCALAIS_PROCESS_COUNT));
  }
}

/**
 *  Function to be called inside of a hook_cron (if auto bulk processing is enabled)
 *  Processed queued nodes - 10 at a time
 */
function opencalais_queue_cron(){
  $queue = DrupalQueue::get('opencalais_tagging_queue');
  $count = OPENCALAIS_PROCESS_COUNT;
  while($count--){
    $item = $queue->claimItem(15);
    if(!$item){
      return; //we're out of items
    }//Process the item and remove it from the queue
    else if(opencalais_queue_process_node($item)){
      $queue->deleteItem($item);
    }
  }
}

/**
 *  Menu Callback to add items for a content type and then go back to the bulk processing page
 */
function opencalais_add_items_to_queue_callback($type){
  $valid_types = _opencalais_get_auto_content_types();
  if(!in_array($type, $valid_types)){
    drupal_set_message(t('Invalid Content Type for Bulk Processing - Either %type is an invalid content type or the content type is not set to automatic tagging.', array('%type'=>$type)), 'error');
  } else {
    $processed = opencalais_queue_add_items($type);
    drupal_set_message(t('%n nodes successfully queued for processing.', array('%n'=>$processed)));
  }
  
  drupal_goto('admin/config/content/opencalais/bulk');
}

/**
 *  Create a form for bulk processing
 */
function opencalais_bulk_operations_form(){
  //print out any messages we have
  opencalais_get_messages();

  $form = array();
  $available_types = _opencalais_get_auto_content_types();
  
  $form['current_count'] = array(
    '#markup' => '<div class="messages status>'.opencalais_queue_get_progress().'</div>'
  );
  
  $form['types'] = array(
    '#type' => 'fieldset',
    '#title' => t('Process Content Types'),
  );
  
  $form['types']['message'] = array(
    '#markup' => '<div class="messages">'.t('Content types marked for autotagging will be shown below. You can queue all the currently created nodes for processing by clicking the corresponding link below. OpenCalais will keep track of the latest nodes (per content-type) that it has processed to avoid processing them again.'). '</div>'
  );
  
  foreach($available_types as $type){
    $form['types']['opencalais_queue_content_type_'.$type] = array(
      '#markup' => '<li>Process all (remaining) '. l($type . ' nodes', 'admin/config/content/opencalais/bulk/'.$type, array('absolute'=>TRUE, 'class'=>array('button'))) . '.</div>',
    );
  }
  return $form;
}

/**
 *  Utility function to get an array of the content types that have autotagging turned on
 */
function _opencalais_get_auto_content_types(){
  $auto_settings = variable_get('opencalais_autotagging', array());
  
  return array_keys(array_filter($auto_settings));
} 
  
/**
 *  Utility function to create opencalais specific message queue
 */ 
function opencalais_set_message($message, $type='status'){
  $queue = variable_get('opencalais_message_queue', array());
  $queue[$type][] = $message;
  variable_set('opencalais_message_queue', $queue);
} 

/**
 *  Utility function to print out all the opencalais messages
 */
function opencalais_get_messages(){
  $queue = variable_get('opencalais_message_queue', array());
  
  foreach($queue as $type=>$messages){
    foreach($messages as $message){
      drupal_set_message($message, $type);
    }
  }
  //empty the queue
  variable_set('opencalais_message_queue', array());
}
 