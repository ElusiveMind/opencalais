<?php

namespace Drupal\opencalais\Form;

use Drupal\Core\Form\ConfigFormBase;

class OpenCalaisSettingsForm extends ConfigFormBase { 

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'opencalais_admin_settings';
  }
  
  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, array &$form_state) {
    $config = $this->config('opencalais.settings');

    $elements = drupal_map_assoc(array('pre', 'code'));

    $form['opencalais_api_key'] = array(
      '#type' => 'text',
      '#title' => $this->t('Open Calais API Key'),
      '#default_value' => $config->get('calaisKey'),
    );

    return parent::buildForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, array &$form_state) {
    $this->config('opencalais.settings')
        ->set('apiKey', $form_state['values']['opencalais_api_key'])
        ->save();

    parent::submitForm($form, $form_state);
  }

}
