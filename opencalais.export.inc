<?php
/**
 *  Creates an interface for importing and exporting configuration for opencalais
 *
 *  Allows for preset creation and application to a content type
 *
 *  Example Preset:
 *
 *   Array
 *   (
 *     [pid] => 0 //should not be included in code versions
 *     [title] => Title
 *     [name] => machine_name
 *     [description] => The description
 *     [process] => manual|automatic
 *     [config] => Array
 *     (
 *        [calais_document_category] => Array
 *            (
 *                [threshold] => 0.25
 *            )
 *  
 *        [city] => Array
 *            (
 *                [threshold] => 0.27
 *            )
 *  
 *        [company] => Array
 *            (
 *                [threshold] => 0.36
 *            )
 *     )
 *   )
 *
 */

/**
 *  Applies a preset to a content type
 *  Preset format as describe above
 */
function opencalais_export_apply_preset($preset, $content_type, $level=1, $add_only=FALSE){
  //just make sure its an array;
  $preset = (array)$preset;
  
  $multiplier = .5 * (2 * $level); //0 = .5, 1 = 1, 2=2
  
  if($multiplier > 1){ $multiplier = 1; }
  
  foreach($preset['config'] as $key=>$value){
    $preset['config'][$key]['threshold'] = $value['threshold'] * $multiplier;
  }
  
  //figure out which fields (if any) are already attached
  $current_fields = opencalais_get_fields_for_content_type($content_type);
  if(!$add_only){
    $to_rem = _opencalais_export_array_diff($preset['config'], $current_fields);
    opencalais_remove_fields($content_type, $to_rem);
  }
  
  opencalais_create_fields($content_type, $preset['config']);
  
  //TODO: Possibly need to add in autocomplete stuff and possibly the preset name - variables;
} 

/**
 *  Helper function to find fields in current fields not in presets for removal
 */
function _opencalais_export_array_diff($preset, $current){
  $ret = array();
  //make sure we have comparable keys
  foreach($preset as $key=>$value){
    $preset[_opencalais_make_machine_name($key)] = $value;
  }
  
  foreach($current as $key => $setting){
    if(!isset($preset[$key])){
      $ret[$key] = $setting;
    }
  }
  return $ret;
}

/**
 *  Wrapper function to load a preset by name
 *  $name - the preset's machine name ($preset['name'])
 */
function opencalais_export_load_preset($name){
  $objects = ctools_export_load_object('opencalais_preset', 'names', array($name));
  return $objects;
}

/**
 *  Load all the presets
 */
function opencalais_export_load_all_presets(){
  $objects = ctools_export_load_object('opencalais_preset');

  return $objects;
}

/**
 *  Create or Update a preset
 *  if $preset has pid set it will be updated, otherwise it will be inserted - name must be unique
 */
function opencalais_export_save_preset($preset){
  $primary_keys = array();
  if(isset($preset['pid']) && !empty($preset['pid'])){
    $primary_keys = 'pid';
  } else if($pid = opencalais_export_check_name($preset['name'])){
    $preset['pid'] = $pid;
    $primary_keys = 'pid';
  }

  return drupal_write_record('opencalais_preset', $preset, $primary_keys);
}

/**
 *  Delete a preset
 *  $preset - must have either pid or name set
 */
function opencalais_export_delete_preset($preset){
  if(isset($preset['pid'])){
    $num = db_delete('opencalais_preset')
      ->condition('pid', $preset['pid'])
      ->execute();
    return $num > 0;
  } else if(isset($preset['name'])){
    $num = db_delete('opencalais_preset')
      ->condition('name', $preset['name'])
      ->execute();
    return $num > 0;
  } else {
    return FALSE;
  }
}

/**
 *  Check if a preset with the given name already exists
 */
function opencalais_export_check_name($name){
  $rows = db_select('opencalais_preset')
    ->fields('opencalais_preset', array('pid'))
    ->condition('name', $name)
    ->execute()
    ->fetchAssoc();

  return $rows && $rows['pid'];
}

/**
 *  Save a preset on submit from the preset button
 */
function opencalais_export_form_validate($form, &$form_state){
  if($form_state['values']['op'] == $form_state['values']['apply_preset']){

    $content_type = $form_state['values']['content_type'];
    $preset_name = $form_state['values']['preset_name'];
    
    $level = $form_state['values']['preset_value'];
    $add_only = ($form_state['values']['preset_add_only']) ? TRUE : FALSE;
            
    $preset = (array)opencalais_export_load_preset($preset_name);

    if($preset){
      $preset = $preset[$preset_name];
      
      opencalais_export_apply_preset($preset, $content_type, $level, $add_only);
      $form_state['opencalais_processed'] = TRUE;
    } else {
      form_set_error('preset_name', t('Invalid Preset'));
    }    
  }
}

/**
 *  Utility function to reformat the config from the form_state to be something that we can save
 */
function _opencalais_export_filter_values($values){
  $new_values = array();
  foreach($values as $type=>$value){
    if($value['enabled']){
      $new_values[$type] = array(
        'threshold' => $value['threshold']
      );
    }
  }
  return $new_values;
}


####################################### INTERFACE ELEMENTS #######################################
/**
 *  Implements hook_form_FORM_ID_alter().
 *  Add a way to save a setting as a preset from the content type fields form
 */
function opencalais_form_opencalais_add_fields_form_alter(&$form, &$form_state, $form_id){
  
  $form['preset'] = array(
    '#type' => 'fieldset',
    '#title' => t('Apply a Preset'),
    '#collapsible' => TRUE, 
    '#collapsed' => FALSE,
    '#weight' => -10,
    '#attributes' => array( 'class' => array('preset_fieldset') ),
  );
  
  $presets = opencalais_export_load_all_presets();
  $values = array(''=>'--');
  foreach($presets as $machine=>$preset){
    $values[$machine] = $preset->title;
  }

  $form['preset']['preset_name'] = array(
    '#type' => 'select',
    '#options' => $values,
    '#title' => t('Preset'),
    '#description' => t('The preset you would like to apply to this content type. This will overwrite all current settings with the settings from the preset'),
  );
  
  $form['preset']['preset_value'] = array(
    '#type' => 'select',
    '#options' => array(
      '0' => 'Low',
      '1' => 'Normal',
      '2' => 'High'
    ),
    '#title' => t('Threshold Level'),
    '#description' => t('The level to set the threshold at, compared to the default.  The lower the threshold, the more terms that will be returned.'),
    '#default_value' => 1
  );
  
  $form['preset']['preset_add_only'] = array(
    '#type' => 'checkbox',
    '#title' => t('Only Add and Update Fields'),
    '#description' => t('Do not remove any fields that aren\'t included in the selected preset')
  );
  
  $form['preset']['apply_preset'] = array(
    '#type' => 'submit',
    '#value' => 'Apply Selected Preset'
  );
    
  $form['#validate'][] = 'opencalais_export_form_validate';
}

/**
 *  Implement a ctools export ui plugin to get most of the interface we need for maintaining presets
 */
function opencalais_ctools_plugin_directory($module, $type){
  if($type == 'export_ui'){
    return 'plugins/export_ui';
  }
} 