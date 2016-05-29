<?php

/**
 * @file
 * The main interface to the Calais web service
 */

class Calais {

  private $defaults = array(
    'protocol' => 'https',
    'contentType' => 'TEXT/HTML',
    'outputFormat' => 'application/json',
    'externalID' => '',
    'submitter' => 'Drupal',
    'calculateRelevanceScore' => 'true',
    'enableMetadataType' => 'SocialTags',
    'allowSearch' => 'false',
    'allowDistribution' => 'false',
    'caller' => 'Drupal',
    'apiKey' => '',
    'host' => 'api.thomsonreuters.com',
    'path' => '/permid/calais',
  );

  public $parameters;
  public $keywords = array();

  /**
   * Constructs an instance of the Calais facade.
   *
   * Valid parameters are specified in the options array as key/value pairs with the
   * parameter name being the key and the parameter setting being the value
   * e.g. array('allowSearch' => 'false')
   *
   * @param options  An array of parameter options for the Calais Web Service.
   *                  These will override the defaults.
   */
  function __construct($options = array()) {
    $this->defaults['externalID'] = time();
    $this->parameters = array_merge($this->defaults, $options);
  }

  /**
   * Analyze the content via Calais.
   *
   * @param $content The content to ship off to Calais for analysis
   * @return The processed Calais results. The raw RDF result is contained in the $this->rdf field.
   */
  public function analyze($content) {

    if (empty($this->parameters['apiKey'])) {
      $ret = new stdClass();
      $ret->error = 'API Key';
      $ret->code = '01';
      $ret->data = 'API key not provided, unable to process.';
      self::log_calais_error($ret);
      return array();
    }

    $headers = array(
      'Content-Type: ' . $this->parameters['contentType'],
      'X-AG-Access-Token: ' . $this->parameters['apiKey'],
      'Content-Length: ' . drupal_strlen($content),
      'outputformat: ' . $this->parameters['outputFormat'],
    );

    $uri = $this->parameters['protocol'] . '://' . $this->parameters['host'] . $this->parameters['path'];

    $curlOptions = array(
      CURLOPT_URL => $uri,
      CURLOPT_HTTPHEADER => $headers,
      CURLOPT_SSL_VERIFYPEER => FALSE,
      CURLOPT_RETURNTRANSFER => 1,
      CURLOPT_POSTFIELDS => $content,
    );

    $ch = curl_init();
    curl_setopt_array($ch, $curlOptions);

    // Send request and get response from API.
    $response = curl_exec($ch);

    if (isset($response->error)) {
      self::log_calais_error($response);
      return array();
    }

    $this->json = json_decode($response);
    $this->processor = new CalaisJsonProcessor();
    $this->keywords = $this->processor->parse_json($this->json);

    return $this->keywords;
  }

  private static function log_calais_error($ret) {
    $msg = t('Calais processing error: @msg', array('@msg' => $ret->data));
    drupal_set_message($msg, 'error');
    watchdog('calais', 'Calais processing error: (@code - @error) @msg', array('@code' => $ret->code, '@error' => $ret->error, '@msg' => $ret->data), WATCHDOG_ERROR);
  }
}
