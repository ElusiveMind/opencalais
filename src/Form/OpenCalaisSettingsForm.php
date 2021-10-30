<?php

namespace Drupal\opencalais\Form;

use Drupal\Core\Form\ConfigFormBase;
use Drupal\Core\Form\FormStateInterface;


class OpenCalaisSettingsForm extends ConfigFormBase { 

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'opencalais_admin_settings';
  }

  public function getEditableConfigNames() {
    return [
      'opencalais.settings',
    ];
  }
  
  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $config = $this->config('opencalais.settings');
    $form['opencalais_api_key'] = array(
      '#type' => 'textfield',
      '#size' => 60,
      '#title' => $this->t('Open Calais API Key'),
      '#default_value' => $config->get('opencalais_api_key'),
    );

    return parent::buildForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $this->config('opencalais.settings')
        ->set('opencalais_api_key', $form_state['values']['opencalais_api_key'])
        ->save();

    parent::submitForm($form, $form_state);
  }

}
