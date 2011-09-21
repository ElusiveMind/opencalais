<?php
/**
 *  Handles bulk processing of nodes for opencalais.
 *
 *  Makes use of the bulk operations api
 */

//Controls how many nodes will be processed per cron run
define('OPENCALAIS_PROCESS_COUNT', 50);
 
/**
 *  Create a queue of all the nodes that need to be tagged, 
 *  This will allow for cron to process the nodes
 */
function opencalais_create_queue(){
  $queue = DrupalQueue::get('opencalais_tagging_queue');
  $queue->createQueue();
  
  $content_types = _opencalais_get_auto_content_types();
  foreach ($content_types as $type) {
      }
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
    $node = node_load($nid);
//    drupal_set_message('<pre>'.print_r($node, true).'</pre>');
    node_save($node);
  } catch(Exception $e){
    drupal_set_message(t('Could not process node with id %id. You may try to manually resave the node to resolve this issue.', array('%id'=>$nid)), 'error');
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
    return t('All nodes have been processed');
  }
  else {
    return t('%n nodes remaining.', array('%n'=> $items));
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
    drupal_set_message(t('%n nodes queued for processing.', array('%n'=>$processed)));
  }
  
  drupal_goto('admin/config/content/opencalais/bulk');
}

/**
 *  Create a form for bulk processing
 */
function opencalais_bulk_operations_form(){
  $form = array();
  $available_types = _opencalais_get_auto_content_types();
  
  $form['current_count'] = array(
    '#markup' => '<div class="messages status>'.opencalais_queue_get_progress().'</div>'
  );
  
  $form['types'] = array(
    '#type' => 'fieldset',
    '#title' => t('Process Content Types'),
  );
  foreach($available_types as $type){
    $form['types']['opencalais_queue_content_type_'.$type] = array(
      '#markup' => '<div>' . l('Process all '. $type . ' nodes.', 'admin/config/content/opencalais/bulk/'.$type, array('absolute'=>TRUE)) . '</div>',
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
  
 