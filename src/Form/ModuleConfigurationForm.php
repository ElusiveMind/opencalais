<?php

namespace Drupal\opencalais\Form;

use Drupal\Core\Form\ConfigFormBase;
use Symfony\Component\HttpFoundation\Request;
use Drupal\Core\Form\FormStateInterface;

/**
 * Defines a form that configures forms module settings.
 */
class ModuleConfigurationForm extends ConfigFormBase {

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'opencalais_admin_settings';
  }

  /**
   * {@inheritdoc}
   */
  protected function getEditableConfigNames() {
    return [
      'opencalais.settings',
    ];
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state, Request $request = NULL) {
    $config = $this->config('opencalais.settings');
    $form['calaisKey'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('Your API Key'),
      '#default_value' => $config->get('calaisKey'),
    );
    $form['calaisHost'] = array(
      '#type' => 'textfield',
      '#title' => $this->t('OpenCalais Host'),
      '#default_value' => $config->get('calaisHost'),
    );
    return parent::buildForm($form, $form_state);
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $values = $form_state->getValues();
    $this->config('opencalais.settings')
      ->set('calaisKey', $values['calaisKey'])
      ->set('calaisHost', $values['calaisHost'])
      ->save();
  }

}