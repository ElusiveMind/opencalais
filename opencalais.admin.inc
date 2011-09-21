<?php
/**
 * @file opencalais.admin.inc
 */
 
/**
 * Present some configuration options.
 */ 
function opencalais_admin_general_settings() {
  $form = array();
  $form['opencalais_tags_in_verticaltab']  = array(
    '#type' => 'checkbox', 
    '#title' => t('Collect Tags into Vertical Tab'), 
    '#description' => t('This will move all of the OpenCalais related Tag fields on the edit form into a Vertical Tab toward the bottom of the form entitled "Show OpenCalais Tags"'), 
    '#default_value' => variable_get('opencalais_tags_in_verticaltab', 1),
  );    
  $form['opencalais_lookup_uri'] = array(
    '#type' => 'checkbox',
    '#title' => t('Lookup Linked Open Data URI'),
    '#description' => t('If checked, an additional HTTP request will be made for each disambiguated entity to find the Linked Open Data URIs to DBPedia, Freebase, etc.  It does involve additional requests to external servers.  If you don\'t know what this is, you should leave it disabled.'),
    '#default_value' => variable_get('opencalais_lookup_uri', 0),
  );
  
  $form['opencalais_bulk_processing_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Bulk Processing on Cron'),
    '#description' => t('Enable bulk processing of nodes for content types with auto tagging enabled.  This processing will happen in batches
      during cron runs until complete.'),
    '#default_value' => variable_get('opencalais_bulk_processing_enabled', FALSE)
  );
  
    
  
  $form['help_text'] = array(
    '#markup' => '<div class="messages status">' . t('To configure tagging visit an individual content type\'s edit area and click the OpenCalais Fields tab.') . 
      '<br/><br/>' . l('Content Types Edit Screen', 'admin/structure/types') .
      '</div>'
  );
  
  return system_settings_form($form);
}

/**
*  A form to add the fields necessary for OpenCalais to a content type.
*/
function opencalais_add_fields_form($content_type){
  //Handle any errors on the form
  if (!empty($_POST) && form_get_errors()) {
    drupal_set_message(t('The fields have not been created because of the errors.'), 'error');
  }
  
  //try to get the content type from the arguments (or passed via menu system) (/admin/structure/types/manage/<typename>)
  //can't do anything if its not there so just return
  $content_type = $content_type ? $content_type : arg(4);
  $content_type = str_ireplace('-', '_', $content_type);
  if(!$content_type){ 
  
    drupal_set_message(t('An error has occured, no content type was specified'), 'error');
    return array(); 
  }
  
  $form = array();
  
  $opencalais_entities = opencalais_get_all_entities();  
  _opencalais_make_keys($opencalais_entities);    
  
  $existing = opencalais_get_fields_for_content_type($content_type);
  
  $form['content_type'] = array(
    '#type' => 'hidden',
    '#value' => str_ireplace('-', '_', $content_type),
  );
  
  $auto = variable_get('opencalais_autotagging', array());
  $auto = (isset($auto[$content_type]) ? $auto[$content_type] : 0);
  
  //Settings for auto tagging
  $form[$content_type . '_autotagging'] = array(
    '#type' => 'radios',
    '#title' => t('Tagging Method'),
    '#description' => t('How should OpenCalais manage the tagging?'),
    '#default_value' => $auto,
    '#options' => array( 0 => t('Manual Tagging'), 1 => t('Automatic Tagging'))
  );
  
  $form['info']['config'] = array('#tree' => TRUE);
  $form['info']['config']['entities'] = array('#tree' => TRUE);
  
  $entities = $opencalais_entities;
  sort($entities);
  
  $entity_defaults = isset($item->config['entities']) ? $item->config['entities'] : array('enabled' => FALSE, 'threshold' => 0.0);
  
  foreach ($entities as $entity) {
    $machine_name = _opencalais_make_machine_name($entity);
    $defaults = isset($entity_defaults[$entity]) ? $entity_defaults[$entity] : $entity_defaults;
    //load in defaults for existing fields
    $defaults['enabled'] = isset($existing[$machine_name]) ? TRUE : FALSE;
    $defaults['threshold'] = isset($existing[$machine_name]) ? $existing[$machine_name]['threshold'] : $defaults['threshold'];
    
    $form['info']['config']['entities'][$entity]['existing'] = array(
      '#type' => 'hidden',
      '#value' => isset($existing[$machine_name]) ? TRUE : FALSE
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
  //create the buttons and submit handlers and such
  $form['actions']['#type'] = 'actions';
  $form['actions']['submit'] = array(
   '#type' => 'submit',
   '#value' => t('Save Fields'),
  );
  
  // This is to create a help block to explain some things about the form
  
  $help_text  = 'This form will allow you to create taxonomy term references on your content type that correspond to ';
  $help_text .= 'OpenCalais categories (<em>taxonomy vocabularies</em>).';
  
  $usage_text  = 'Simply select the categories that apply to your content by checking their checkbox and then use the slider to set ';
  $usage_text .= 'the relevancy threshold which a term must meet to be applied.'; 
  
  $notes_text = 'Note: When saving with already existing fields, a message will appear that the field(s) have been successfully updated.  Each field is always resaved so this is normal.';
  
  $form['help'] = array(
    '#type' => 'item',
    '#title' => t('OpenCalais Fields Help'),
    '#description' => t('Help with this form'),
    '#markup' => '<br/><p>' . t($help_text) . '</p><br/><p>' . t($usage_text) . '</p><br/><p><em>' . t($notes_text) . '</em></p>',
    '#prefix' => '<div class="">',
    '#suffix' => '</div>'
  );
  
  $form['#submit'][] = 'opencalais_add_fields_submit';
  
  $form['#theme'] = 'system_settings_form';

  return $form;
}
