<?php

namespace Drupal\opencalais\Form;

use Drupal\Core\Form\FormStateInterface;
use Drupal\Core\Form\ConfigFormBase;

/**
 * Class OpenCalaisFields.
 */
class OpenCalaisFields extends ConfigFormBase {

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return [
      'opencalais.opencalaisnodeconfig',
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'open_calais_fields';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    // @TODO: Actually divine the node type
    $node_type = \Drupal::routeMatch()->getCurrentRouteMatch()->getRawParameter('node_type');
    $config = $this->config('opencalais.opencalaisnodeconfig')->get($node_type);
    $entityManager = \Drupal::service('entity_field.manager');
    $fields = $entityManager->getFieldDefinitions('node', $node_type);
    foreach ($fields as $fieldName => $field) {
      // @TODO: Just search for text fields so we aren't doing this messy nonsense
      if (strpos($fieldName, 'field_') !== false) {
        $form[$fieldName] = [
          '#type' => 'checkbox',
          '#title' => $this->t($field->getLabel()),
          '#weight' => '0',
        ];
        if (isset($config[$fieldName])) {
          $form[$fieldName]['#default_value'] = TRUE;
        }
      } elseif($fieldName == 'title' || $fieldName == 'body') {
       $form[$fieldName] = [
          '#type' => 'checkbox',
          '#title' => $this->t(ucfirst($fieldName)),
          '#weight' => '0',
        ];
        if (isset($config[$fieldName])) {
          $form[$fieldName]['#default_value'] = TRUE;
        } 
      }
   }

    $form['submit'] = [
      '#type' => 'submit',
      '#value' => $this->t('Submit'),
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function validateForm(array &$form, FormStateInterface $form_state) {
    parent::validateForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    // @TODO: Actually divine the content type
    $node_type = 'event';
        parent::submitForm($form, $form_state);
    // Display result.
    $field_values = [];
    foreach ($form_state->getValues() as $field => $value) {
      if ((strpos($field, 'field_') !== false || $field == 'body' || $field == 'title') && $value == 1) {
        $field_values[$field] = $value;
      }
        $this->config('opencalais.opencalaisnodeconfig')->set($node_type , $field_values)->save();
    }
  }
}
