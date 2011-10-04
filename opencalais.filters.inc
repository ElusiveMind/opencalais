<?php
/**
 *  This includes file provides the ability to create filters to limit the possible terms that can be returned as
 *  suggestions.
 *  
 *  Will start out simple, with the hope to create a generalized framework for creating a filter
 */
 
/**
 *  Apply all available filters
 *
 *  This is different than simply implementing the alter hook (which you certainly may do) because it allows for filters to be applied by
 *  content type and turned on or off.
 *
 *  Its signature should be mymodule_opencalais_filter(&$terms, $content_type) and it should return the altered $terms array
 */
function opencalais_filters_apply_filters(&$terms, $content_type){
  $filters = opencalais_filters_get_enabled_filters($content_type);
  
  foreach($filters as $filter){
    $function = $filter['callback'];
    if(function_exists($function)){
      $function($terms, $content_type);
    }
  }
} 

/**
 *  Allow other functions to declare filters
 *  Collect the list and cache it
 *
 *  Defines a hook: hook_opencalais_filter_info
 *  @see opencalais.filters.api.php for more info
 */
function opencalais_filters_get_filters(){
  $data = cache_get('opencalais_filters');
  //see if we have cached data - if not get the filters and cache them
  if(!$data){
    $filters = module_invoke_all('opencalais_filter_info');
    cache_set('opencalais_filters', $filters);
    return $filters;
  }
  //otherwise return the cached data
  return $data->data;

} 

/**
 *  Return a list of filters enabled for a particular content type
 */
function opencalais_filters_get_enabled_filters($content_type){
  $settings = variable_get('opencalais_filters_settings', array());
  
  if(isset($settings[$content_type])){
    $filters = opencalais_filters_get_filters();
    foreach($filters as $name=>$filter){
      if(!in_array($name, $settings[$content_type])){
        unset($filters[$name]);
      }
    }

    return $filters;
  } else {
    return array();
  }
}

/**
 *  Provide a menu callback to show the configure form for the filter
 */
function opencalais_filters_configure_filter($filter_name){
  $filters = opencalais_filters_get_filters();

  if(isset($filters[$filter_name]['configuration'])){   
    return drupal_get_form($filters[$filter_name]['configuration']);
  } 
  //we should probably handle some sort of 404 action here if the filter doesn't exist
}

/**
 *  Provides the configuration form for filters which basically allows you to apply filters
 *  to content types
 */
function opencalais_filters_config_form(){
  $form = array();
  
  $content_types = _opencalais_get_content_types();
  $filter_options = opencalais_filters_get_filters();
  
  $form['filters'] = array(
    '#type' => 'fieldset',
    '#title' => t('Configure Filters'),
  );
  
  foreach($filter_options as $machine=>$filter){
    $form['filters'][$machine] = array(
      '#markup' => '<div class="filter-section">' .
        l(t('Configure @name Filter', array('@name'=>$filter['title'])), 'admin/config/content/opencalais/filters/'.$machine) .
        '</div>',
    );
  }
  
  $form['config'] = array(
    '#type' => 'vertical_tabs',
    '#title' => t('Apply Filters'),
  );
  
  $filter_options = _opencalais_filters_make_options_array($filter_options);
  foreach($content_types as $type=>$info){
    $filters = opencalais_filters_get_enabled_filters($type);
    $filters = _opencalais_filters_make_options_array($filters);
    $form['config'][$type.'tab'] = array(
      '#type' => 'fieldset',
      '#title' => $info['label'],
    );
    $form['config'][$type.'tab'][$type] = array(
      '#type' => 'checkboxes',
      '#options' => $filter_options,
      '#title' => t('@type Filters', array('@type' => $info['label'])),
      '#description' => t("Filters to be run on this content type"),
      '#default_value' => array_keys($filters),
    );
  }
    
  $form = system_settings_form($form);
  $form['#submit'] = array('opencalais_filters_save_settings');

  return $form;
}

/**
 *  Save the settings off the main configuration form
 */
function opencalais_filters_save_settings($form, &$form_state){
  $content_types = _opencalais_get_content_types();
  
  $settings = variable_get('opencalais_filters_settings', array());
  foreach($content_types as $type=>$info){
    $settings[$type] = array_filter($form_state['values'][$type]);
  }
  variable_set('opencalais_filters_settings', $settings);
  drupal_set_message(t('Configuration successfully updated.'));
}

#################### Define filters and their callbacks ####################
/**
 *  Define a filter type
 */
function opencalais_opencalais_filter_info(){
  return array(
    'global_blacklist' => array(
      'title' => t('Blacklist'),
      'description' => t('Use a taxonomy vocabularly as a global black list. Terms in the vocabulary will not be applied.'),
      'callback' => 'opencalais_filters_blacklist',
      'configuration' => 'opencalais_filters_blacklist_config'
    ),
  );
} 

function opencalais_filters_blacklist(&$terms, $content_type){
  $vocab = variable_get('opencalais_filters_blacklist_vocab', FALSE);
  if($vocab){
    $blacklist = taxonomy_get_tree($vocab);
    //create an array keyed by name to avoid having to loop through repeatedly
    $blacklist = _opencalais_filters_create_name_array($blacklist);
    
    //filter the terms
    foreach($terms as $name=>$tags){
      foreach($tags as $term=>$info){
       
        if(isset($blacklist[$term])){
          unset($terms[$name][$term]);
        }
      }
    }  
  }
  //otherwise do nothing
}

function opencalais_filters_blacklist_config(){
  $form = array();
  
  $vocabs = taxonomy_get_vocabularies();
  foreach($vocabs as $vocab){
    $options[$vocab->vid] = $vocab->name;
  }
  
  $form['opencalais_filters_blacklist_vocab'] = array(
    '#type' => 'select',
    '#title' => 'Vocabulary to use as Blacklist',
    '#options' => $options,
    '#default_value' => variable_get('opencalais_filters_blacklist_vocab', FALSE),
  );  
  
  return system_settings_form($form);
}

##################### Utility functions #####################
/**
 *  Utility Function to create an array keyed by name
 *  Accepts arrays of taxonomy term objects
 */
function _opencalais_filters_create_name_array($array){
  $to_ret = array();
  
  foreach($array as $key=>$value){
    $to_ret[$value->name] = $value->tid;
  }
  return $to_ret;
}

/**
 *  Take an array of filters and make it into an options array
 */
function _opencalais_filters_make_options_array($filters){
  $to_ret = array();
  foreach($filters as $machine=>$filter){
    $to_ret[$machine]=$filter['title'];
  }
  return $to_ret;
}