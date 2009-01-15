<?php
/**
 * @file calais_handler_field_term_data_guid.inc
 */
 
class calais_handler_field_term_data_guid extends views_handler_field {

  /**
   * Constructor to provide additional field to add.
   *
   * This constructer assumes the term_data table. If using another
   * table, we'll need to be more specific.
   */
  function construct() {
    parent::construct();
    $this->additional_fields['name'] = 'name';
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['link_to_guid'] = array('default' => FALSE);
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['link_to_guid'] = array(
      '#title' => t('Link this field to the GUID'),
      '#type' => 'checkbox',
      '#default_value' => !empty($this->options['link_to_guid']),
    );
    $form['link_text'] = array(
      '#type' => 'textfield',
      '#title' => t('Text to use for link label'),
      '#description' => t('Enter any text. Other options: Leave blank to use the GUID as the label. Use [name] to use the term name. Use [image] to use a 16x16 Calais logo. Enter HTML in here for your own image tag, etc.'),
      '#default_value' => $this->options['link_text'],
    );
  }
  
  function render($values) {
    $data = $values->{$this->field_alias};
    if (!empty($this->options['link_to_guid']) && $data !== NULL && $data !== '') {
      return l($this->get_label($values), $data, array('absolute' => TRUE, 'html' => TRUE));
    }
    else {
      return $data;
    }
  }
  
  function get_label($values) {
    $label = $this->options['link_text'];
    switch ($label) {
      case '':
        return $values->{$this->field_alias};
      case '[name]':
        return $values->{$this->aliases['name']};
      case '[image]':
        return theme('image', drupal_get_path('module', 'calais') . '/images/calais-logo.jpg', 'Calais GUID', 'Calais GUID');
      default:
        return $label;
    }
  }
}
