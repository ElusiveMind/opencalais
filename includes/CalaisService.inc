<?php

/**
 * @file Calais.inc
 * The main interface to the calais web service
 */

class CalaisService implements Calais {
  
  const PATH = '/permid/calais';
  
  private $defaults = array(
    'protocol' => 'http',
    'contentType' => 'TEXT/HTML',
    'outputFormat' => 'XML/RDF',
    'externalID' => '',
    'submitter' => 'Drupal',
    'calculateRelevanceScore' => 'true',
    'enableMetadataType' => 'SocialTags',
    'allowSearch' => 'false',
    'allowDistribution' => 'false',
    'caller' => 'Drupal',
  );
  
  public $parameters;
  public $rdf;
  public $triples = array();
  public $flatTriples = array();
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
   *
   * @see http://opencalais.com/APIcalls#inputparameters
   */
  function __construct($options = array()) {
    $this->defaults['externalID'] = time();
    $this->defaults['host'] = variable_get('opencalais_api_server', 'api.thomsonreuters.com');
    
    $this->parameters = array_merge($this->defaults, $options);
  }
  
  /**
   * Analyze the provided content, passing it to Calais in HTML format .
   *
   * @param $content  
   *    The HTML content to process
   * @return 
   *    The processed Calais results. The raw RDF result is contained in the $this->rdf field.
   */
  public function analyzeHTML($content, $api_key) {
    if (!empty($api_key)) {
      $this->parameters['contentType'] = 'TEXT/HTML';
      return $this->analyze($content, $api_key);
    }
    else {
      return array();
    }
  }

  /**
   * Analyze the content via Calais.
   *
   * @param $content The content to ship off to Calais for analysis
   * @return The processed Calais results. The raw RDF result is contained in the $this->rdf field.
   */
  public function analyze($content, $api_key) {

    $headers = array(
      'Content-Type: text/raw',
      'X-AG-Access-Token: ' . $api_key,
      'Content-Length: ' . strlen($content),
      'outputformat:XML/RDF',
    );
    
    $uri = $this->parameters['protocol'] . 's://' . $this->parameters['host'] . self::PATH;
    
    $curlOptions = array (
      CURLOPT_URL => $uri,
      CURLOPT_HTTPHEADER => $headers,
      CURLOPT_SSL_VERIFYPEER => FALSE,
      CURLOPT_RETURNTRANSFER => 1,
      CURLOPT_POSTFIELDS => $content,
    );

    $ch = curl_init();
    curl_setopt_array($ch,$curlOptions);

    // send request and get response from api..........
    $response = curl_exec($ch);

    if (isset($response->error)) {
      self::log_calais_error($response);
      return array();
    }

    $this->rdf = $response;
    $this->processor = new CalaisRdfProcessor();
    $this->keywords = $this->processor->parse_rdf($this->rdf);    
    $this->triples = $this->processor->triples;
    if (isset($this->processor->flatTriples)) {  
      $this->flatTriples = $this->processor->flatTriples;
    }
    
    return $this->keywords;    
  }

  private static function log_calais_error($ret) {
    $msg = t('OpenCalais processing error: @msg', array('@msg' => $ret->data));
    drupal_set_message($msg, 'error');
    watchdog('opencalais', 'OpenCalais processing error: (@code - @error) @msg', array('@code' => $ret->code, '@error' => $ret->error, '@msg' => $ret->data), WATCHDOG_ERROR);
  }
}
