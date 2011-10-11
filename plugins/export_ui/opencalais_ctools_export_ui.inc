<?php
/**
 *  Provides CTools Export UI plugin for presets 
 *
 *  Gets us a UI for managing the presets
 *  See http://api.acquia.com/api/ctools/help--export-ui.html/7.x-1.x/source (export-ui.html in ctools) for more info.
 *  Also see: http://drupal.org/node/928026 for a walkthrough
 */
 
$plugin = array(
  'schema' => 'opencalais_preset',
  'access' => 'administer opencalais',
  'menu' => array(
    'menu prefix' => 'admin/config/content',
    'menu item' => 'opencalais_presets',
    'menu title' => 'Manage OpenCalais Presets',
    'menu description' => 'Manage the presets for OpenCalais',
  ),
  'title singular' => t('OpenCalais Preset'),
  'title singular proper' => t('OpenCalais Preset'),
  'title plural' => t('OpenCalais Presets'),
  'title plural proper' => t('OpenCalais Presets'),
  
  'form' => array(
    'settings' => 'opencalais_export_presets_form',
    'validate' => 'opencalais_export_presets_form_validate',
    'submit' => 'opencalais_export_presets_form_submit',
  ),
); 

function opencalais_export_presets_form(&$form, &$form_state){
  $item = isset($form_state['item']) ? $form_state['item'] : new stdClass();
  
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Preset Title'),
    '#description' => t(' A unique title for this preset.'),
    '#default_value' => isset($item->title) ? $item->title : '',
    '#weight' => 0,
  );
  
  $form['name'] = array(
    '#type' => 'machine_name',
    '#title' => t('Preset Machine Name'),
    '#required' => TRUE,
    '#description' => t('A unique machine name for this preset.'),
    '#default_value' => isset($item->name) ? $item->name : '',
    '#machine_name' => array(
      'exists' => 'opencalais_export_check_name_exists',
      'source' => array('title'),
    ),
    '#weight' => 1,
  );
  
  $form['description'] = array(
    '#type' => 'textfield',
    '#title' => t('Description'),
    '#description' => t('A description of this preset.'),
    '#default_value' => isset($item->description) ? $item->description : '',
    '#weight' => 2,
  );
  
  //$form = array();
  $opencalais_entities = opencalais_get_all_entities();  
  _opencalais_make_keys($opencalais_entities);    
  
  $form['info'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#title' => t('Category Level Settings'),
    '#weight' => 10,
  );
  
  $form['info']['config'] = array('#tree' => TRUE);
  $form['info']['config']['entities'] = array('#tree' => TRUE);
  
  $entities = $opencalais_entities;
  sort($entities);
  
  //drupal_set_message('<pre>'.print_r($item, true).'</pre>');
  
  $entity_defaults = isset($item->config) ? $item->config : array();
  
  foreach ($entities as $entity) {
    //load in defaults for existing fields
    $machine_name = _opencalais_make_machine_name($entity);
    $defaults = array();

    $defaults['enabled'] = isset($entity_defaults[$entity]);
    $defaults['threshold'] = isset($entity_defaults[$entity]['threshold']) ? $entity_defaults[$entity]['threshold'] : 0;

    $form['info']['config']['entities'][$entity]['existing'] = array(
      '#type' => 'hidden',
      '#value' => $defaults['enabled']
    );
    
    $form['info']['config']['entities'][$entity]['enabled'] = array(
      '#type' => 'checkbox',
      '#title' => t('Create'),
      '#title_display' => 'invisible',
      '#default_value' => $defaults['enabled'],
      '#description' => t('Should this content type be tagged with this category'),
    );
    $form['info']['config']['entities'][$entity]['threshold'] = array(
      '#type' => 'textfield',
      '#size' => 5,
      '#title' => t('Minimum suggested tag relevancy'),
      '#title_display' => 'invisible',
      '#default_value' => $defaults['threshold'],
      '#description' => t('Determine how relevant a term must be in order for Calais to suggest it for a particular node.  Based on a 0.00-1.00 scale, with 0.00 being least relevant (i.e. many terms appear).'),
      '#attributes' => array('class' => array('threshold_slider')),
    );
  }
  $form['info']['#theme'] = 'opencalais_add_fields_entities';
  
}

function opencalais_export_presets_form_validate(&$form, &$form_state){

}

function opencalais_export_presets_form_submit(&$form, &$form_state){

  $form_state['item']->name = $form_state['values']['name'];
  $form_state['item']->title = $form_state['values']['title'];
  $form_state['item']->description = $form_state['values']['description'];
  $form_state['item']->process = 0;
  $form_state['item']->config = _opencalais_export_presets_process_config($form_state['values']['config']);

}

function _opencalais_export_presets_process_config($config){
  $settings = array();
  foreach($config['entities'] as $name=>$item){
    if($item['threshold']){
      $settings[_opencalais_make_machine_name($name)] = array(
        'threshold' => $item['threshold']
      );
    }  
  }
  return $settings; 
}