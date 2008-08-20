<?php
// $Id$
/**
 * @file Calais.inc
 * The main interface to the calais web service
 */

class Calais {
  
  const PATH = '/enlighten/calais.asmx/Enlighten';
  
  private $defaults = array(
    'host' => 'http://api.opencalais.com',
    'contentType' => 'TEXT/HTML',
    'outputFormat' => 'XML/RDF',
    'externalID' => '',
    'submitter' => "Drupal Calais",
    'calculateRelevanceScore' => 'true',
    'allowSearch' => 'false',
    'allowDistribution' => 'false',
  );
  
  public $parameters;
  public $rdf;
  
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
    $this->defaults['allowSearch'] = variable_get('calais_api_allow_searching', FALSE) ? 'true' : 'false';
    $this->defaults['allowDistribution'] = variable_get('calais_api_allow_distribution', FALSE) ? 'true' : 'false';
    
    $this->parameters = array_merge($this->defaults, $options);
  }
  
  /**
   * Analyze the provided content, passing it to Calais in XML format for more accurate data processing.
   *
   * @param $title  The title of the content to process
   * @param $body   The body ofd the content to process
   * @param $date   The date of the content, if left blank/null analysis will use "today"
   * @return The processed Calais results. The raw RDF result is contained in the $this->rdf field.
   */
  public function analyzeXML($title, $body, $date) {
    $content = $this->build_xml_content($title, $body, $date);
    $this->parameters['contentType'] = 'TEXT/XML';
    return $this->analyze($content);
  }
  
  /**
   * Analyze the content via Calais.
   *
   * @param $content The content to ship off to Calais for analysis
   * @return The processed Calais results. The raw RDF result is contained in the $this->rdf field.
   */
  public function analyze($content) {

    $headers = array('Content-Type' => 'application/x-www-form-urlencoded');
    $data    = array(
      'licenseID' => variable_get('calais_api_key', NULL),
      'content' => $content,
      'paramsXML' => $this->build_xml_params(),
    );

    $data_enc = http_build_query($data, '', '&');
    $uri = $this->parameters['host'] . self::PATH;
    $ret = drupal_http_request($uri, $headers, 'POST', $data_enc);
    if (isset($ret->error)) {
      self::log_calais_error($ret);
      return array();
    }

    $xml       = $ret->data;    
    $this->rdf = $this->extract_rdf($xml);
    $keywords  = $this->parse_rdf($this->rdf);    
    return $keywords;    
  }

  private static function log_calais_error($ret) {
    $msg = t('Calais processing error: @msg', array('@msg' => $ret->data));
    drupal_set_message($msg, 'error');
    watchdog('calais', 'Calais processing error: (@code - @error) @msg', array('@code' => $ret->code, '@error' => $ret->error, '@msg' => $ret->data), WATCHDOG_ERROR);
  }
  
  /**
   * Build the XML Parameters required by the Calais Web-Service
   *
   * @return XML document of Calais parameters.
   */
  protected function build_xml_params() {
    $attributes = $this->parameters;
    $ret = <<<EOD
    <c:params xmlns:c="http://s.opencalais.com/1/pred/" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
    <c:processingDirectives c:contentType="$attributes[contentType]" 
                            c:outputFormat="$attributes[outputFormat]"
                            c:calculateRelevanceScore="$attributes[calculateRelevanceScore]">
    </c:processingDirectives>
    <c:userDirectives c:allowDistribution="$attributes[allowDistribution]" 
                      c:allowSearch="$attributes[allowSearch]"           
                      c:externalID="$attributes[externalID]" 
                      c:submitter="$attributes[submitter]">
    </c:userDirectives>
    <c:externalMetadata>
    </c:externalMetadata>
    </c:params>
EOD;
    return $ret;
  }
  
  /**
   * Build the XML document request format expected by Calais
   *
   * @return an xml string to be submitted to Calais
   * @see http://opencalais.com/APIcalls#inputcontentformat
   */
  protected function build_xml_content($title, $body, $date) {
    $req = "<DOCUMENT>";
    $req .= "<TITLE>$title</TITLE>";
    $req .= "<DATE>$date</DATE>";
    $req .= "<BODY>$body</BODY>";
    $req .= "</DOCUMENT>";
    return $req;
  }
  
  /**
   * Calais returns an XML envelope around the RDF, instead of
   * pure RDF/XML, so we need to remove it.
   *
   * @return RDF/XML
   */
  protected function extract_rdf($data) {
    $xml     = html_entity_decode($data);
    $success = preg_match('/<rdf:RDF.+\/rdf:RDF>/ims', $xml, $matches);
    return $matches[0];
  }
  
  /**
   * You need to understand how Calais responses are structured to parse its
   * RDF.
   *
   * Samle element:
   *  [http://d.opencalais.com/pershash-1/fc0ac3c8-5d95-3625-bbd2-6e6fc3f1d703] => Array (
   *    'http://www.w3.org/1999/02/22-rdf-syntax-ns#type' =>
   *    array (
   *       0 => 'http://s.opencalais.com/1/type/em/e/Person',
   *    ),
   *    'http://s.opencalais.com/1/pred/name' =>
   *    array (
   *       0 => 'Diana',
   *    )
   *  )
   */
  protected function parse_rdf($rdf_xml) {

    $parser = ARC2::getRDFXMLParser();
    $parser->parse(NULL, $rdf_xml);
    $indexed_triples = $parser->getSimpleIndex();
    //return $indexed_triples;

    $keywords = new stdClass();
    foreach ($indexed_triples as $guid => $indx) {
      $this->extract_entities($keywords, $guid, $indx);
      $this->extract_events($keywords, $guid, $indx);
      $this->extract_relevance($keywords, $indx);
    }

    return $keywords;
  }

  /**
   * Extracts the entities from the returned data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The guid for the current Calais Term
   * @param $indx The indexed triple for the current Calais Term/GUID
   */
  protected function extract_entities(&$keywords, $guid, $indx) {
    $type = $indx['http://www.w3.org/1999/02/22-rdf-syntax-ns#type'];
    if (isset($type) && array_key_exists("http://s.opencalais.com/1/pred/name", $indx)) {
      
      $keyword_type_guid = $type[0];
      $keyword_type = preg_replace('/.*\//ims', '', $keyword_type_guid);

      $keyword_value = $indx['http://s.opencalais.com/1/pred/name'][0];
      $keyword_value = db_escape_string($keyword_value);
      
      if (!property_exists($keywords, $keyword_type)) {
        $keywords->$keyword_type = new CalaisMetadata($keyword_type_guid, $keyword_type);
      }

      $metadata = &$keywords->$keyword_type;
      $metadata->add_term($guid, $keyword_value);        
    }
  }

  /**
   * Extracts the events & facts from the returned data. For now it is considered best that all
   * Events & Facts are put into one Vocabulary and identified by their type.
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The guid for the current Calais Term
   * @param $indx The indexed triple for the current Calais Term/GUID
   */
  protected function extract_events(&$keywords, $guid, $indx) {
    $type = $indx['http://www.w3.org/1999/02/22-rdf-syntax-ns#type'];
    if (isset($type) && strpos($type[0], 'http://s.opencalais.com/1/type/em/r') === 0) {
      
      $type_guid = $type[0];
      $type_value = preg_replace('/.*\//ims', '', $type_guid);
      $type_value = calais_api_make_readable($type_value);
      $type_value = db_escape_string($type_value);
      
      if (!property_exists($keywords, 'EventsFacts')) {
        $keywords->EventsFacts = new CalaisMetadata('http://drupal.org/project/opencalais/EventsFacts', 'EventsFacts');
      }

      $metadata = &$keywords->EventsFacts;
      // Not sure if the best number for relevance of an Event/Fact, for now 1 will always include
      $metadata->add_term($type_guid, $type_value, 1.0);      
    }
  }

  /**
   * Extracts the relevance score from the returned data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $indx The indexed triple for the current Calais Term
   */
  protected function extract_relevance(&$keywords, $indx) {
    $type = $indx['http://www.w3.org/1999/02/22-rdf-syntax-ns#type'];
    if (isset($type) && $type[0] == 'http://s.opencalais.com/1/type/sys/RelevanceInfo') {
      $subject = $indx['http://s.opencalais.com/1/pred/subject'][0];
      $relevance = $indx['http://s.opencalais.com/1/pred/relevance'][0];
      $this->assign_relevance($keywords, $subject, $relevance);
    }
  }

  /**
   *
   */
  protected function assign_relevance(&$keywords, $subject, $relevance) {
    foreach ($keywords as &$entity) {
      if ($entity->has_guid($subject)) {
        $entity->add_relevance($subject, $relevance);      
      }
    }
  }
  
}