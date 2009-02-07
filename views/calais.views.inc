<?php
/**
 * @file calais.views.inc
 */

/**
 * Implementation of hook_views_data()
 */
function calais_views_data() {
  $data = array();

  //******* Calais Term ***********
  $data['calais_term']['table']['group']  = t('Calais');
  $data['calais_term']['table']['base'] = array(
    'field' => 'tid',
    'title' => t('Calais Term'),
    'help' => t('Calais terms are attached to nodes.'),
  );


  $data['calais_term']['table']['join'] = array(
    'node' => array(
      'left_table' => 'calais_term_node',
      'left_field' => 'tid',
      'field' => 'tid',
    ),
    'node_revision' => array(
      'left_table' => 'calais_term_node',
      'left_field' => 'tid',
      'field' => 'tid',
    ),
    // // This is provided for many_to_one argument
    // 'calais_term_node' => array(
    //   'field' => 'tid',
    //   'left_field' => 'tid',
    // ),
  );

  $data['calais_term']['tid'] = array(
    'title' => t('Term ID'),
    'help' => t('The Calais term ID'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'skip base' => array('node', 'node_revision'),
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_numeric',
      'skip base' => array('node', 'node_revision'),
    ),
  );

  $data['calais_term']['tid'] = array(
    'title' => t('Term ID'),
    'help' => t('The Calais term ID'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'skip base' => array('node', 'node_revision'),
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_numeric',
      'skip base' => array('node', 'node_revision'),
    ),
  );
  $data['calais_term']['vid'] = array(
    'title' => t('Vocabulary ID'),
    'help' => t('The Calais Entity Class that corresponds to a Taxonomy Vocabulary'),
    'field' => array(
      'handler' => 'views_handler_field_numeric',
      'skip base' => array('node', 'node_revision'),
    ),
    'argument' => array(
      'handler' => 'views_handler_argument_numeric',
      'skip base' => array('node', 'node_revision'),
    ),
  );

  $data['calais_term']['name'] = array(
    'title' => t('Entity Name'),
    'help' => t('Calais Entity'),
    'field' => array(
      'handler' => 'calais_handler_field_calais',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );
  
  $data['calais_term']['guid'] = array(
    'title' => t('GUID'),
    'help' => t('Calais Linked Data URI'),
    'field' => array(
      'handler' => 'calais_handler_field_guid',
    ),
  );
  
  $data['calais_term']['resolved_name'] = array(
    'title' => t('Resolved Name'),
    'help' => t('The disambiguated Calais Entity name'),
    'field' => array(
      'handler' => 'calais_handler_field_calais',
      'click sortable' => TRUE,
    ),
    'sort' => array(
      'handler' => 'views_handler_sort',
    ),
  );
  

  
  return $data;
}

/**
 * Implementation of hook_views_handlers().
 */
function calais_views_handlers() {
  return array(
    'info' => array(
      'path' => drupal_get_path('module', 'calais') . '/views/handlers',
    ),
    'handlers' => array(
      // fields
      'calais_handler_field_calais' => array(
        'parent' => 'views_handler_field',
      ),
      'calais_handler_field_guid' => array(
        'parent' => 'views_handler_field_url',
      ),
    ),
  );
}
