<?php
/**
 * @file calais.views.inc
 */

/**
 * Implementation of hook_views_data()
 */
function calais_views_data() {
  $data = array();
  $data['term_data']['guid'] = array(
    'title' => t('Calais GUID'),
    'help' => t('A dereferencable GUID provided by Calais'), 
    'field' => array(
      'handler' => 'calais_handler_field_term_data_guid',
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
      'calais_handler_field_term_data_guid' => array(
        'parent' => 'views_handler_field',
      ),
    ),
  );
}
