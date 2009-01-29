<?php
// $Id$
/**
 * @file Calais.inc
 * The main interface to the calais web service
 */

class Calais {
  
  const PATH = '/enlighten/rest/';
  const RDF_TYPE = 'http://www.w3.org/1999/02/22-rdf-syntax-ns#type';
  const CALAIS_SUBJECT = 'http://s.opencalais.com/1/pred/subject';
  const CALAIS_NAME = 'http://s.opencalais.com/1/pred/name';
  const CALAIS_LAT = 'http://s.opencalais.com/1/pred/latitude';
  const CALAIS_LON = 'http://s.opencalais.com/1/pred/longitude';
  const CALAIS_SCORE = 'http://s.opencalais.com/1/pred/score';
  
  private $defaults = array(
    'protocol' => 'http',
    'contentType' => 'TEXT/HTML',
    'outputFormat' => 'XML/RDF',
    'externalID' => '',
    'submitter' => 'Drupal',
    'calculateRelevanceScore' => 'true',
    'allowSearch' => 'false',
    'allowDistribution' => 'false',
    'caller' => 'Drupal',
  );
  
  public $parameters;
  public $rdf;
  public $triples;
  public $flatTriples;
  public $keywords;
  
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
    $this->defaults['allowSearch'] = variable_get('calais_api_allow_searching', TRUE) ? 'true' : 'false';
    $this->defaults['allowDistribution'] = variable_get('calais_api_allow_distribution', TRUE) ? 'true' : 'false';
    $this->defaults['host'] = variable_get('calais_api_server', 'api1.opencalais.com');
    
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
    $uri = $this->parameters['protocol'] . '://' . $this->parameters['host'] . self::PATH;
    $ret = drupal_http_request($uri, $headers, 'POST', $data_enc);
    if (isset($ret->error)) {
      self::log_calais_error($ret);
      return array();
    }

    $this->rdf = $ret->data;
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
      <rdf:description>
        <c:caller>$attributes[caller]</c:caller>
      </rdf:description>
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
    $req .= "<TITLE><![CDATA[$title]]></TITLE>";
    $req .= "<DATE>$date</DATE>";
    $req .= "<BODY><![CDATA[$body]]></BODY>";
    $req .= "</DOCUMENT>";
    return $req;
  }
  
  /**
   * Parse the RDF.  First render it into an indexed fashion, then have at it. We need to process
   * in 2 stages. The first stage identifies all of the entities, events, and facts. The second
   * stage then adds relevance and geo info to those previously identified terms.  The 2nd pass i
   * is required b/c sometimes the relevance/geo data appears in the document before the term
   * has been identified.
   *
   * @param $rdf_xml
   *    The RDF to parse
   * @return
   *    An array of CalaisMetadata objects.
   * @see 
   *    CalaisMetadata.inc
   */
  protected function parse_rdf($rdf_xml) {

    $parser = ARC2::getRDFXMLParser();
    $parser->parse(NULL, $rdf_xml);
    $this->triples = $parser->getSimpleIndex();
    // Use this method once http://drupal.org/node/348758 is resolved
    //$this->flatTriples = array_map('_rdf_deconstruct_arc2_triple', $parser->getTriples());
    $this->flatTriples = array_map('_calais_api_deconstruct_arc2_triple', $parser->getTriples());

    $this->keywords = new stdClass();
    $this->build_entities($this->keywords);
    $this->extract_entity_metadata($this->keywords);
    return $this->keywords;
  }

  /**
   * Build the set of entities from this RDF triples returned from Calais.
   *
   * @param $keywords 
   *    The object containing type keyed CalaisMetadata
   */
  protected function build_entities(&$keywords) {
    foreach ($this->triples as $guid => $data) {
      $type = $data[self::RDF_TYPE];
      
      if (!isset($type)) 
        continue;
        
      if (strpos($type[0], "http://s.opencalais.com/1/type/em/e") !== FALSE) {
        $this->extract_entities($keywords, $type, $guid, $data);
      }
      else if (strpos($type[0], 'http://s.opencalais.com/1/type/em/r') !== FALSE) {
        $this->extract_events($keywords, $type, $guid, $data);
      }
      else if ($type[0] == 'http://s.opencalais.com/1/type/cat/DocCat') {
        $this->extract_categories($keywords, $type, $guid, $data);
      }      
    }
  }

  /**
   * Process the RDF triple and grab the additional metadata on entities.
   * Additional metadata is relevance score and a slew of resolved entity dismbiguation
   *
   * @param $keywords 
   *    The object containing type keyed CalaisMetadata
   */
  protected function extract_entity_metadata(&$keywords) {
    foreach ($this->triples as $guid => $data) {
      $type = $data[self::RDF_TYPE];
      
      if (!isset($type)) 
        continue;
        
      if ($type[0] == 'http://s.opencalais.com/1/type/sys/RelevanceInfo') {
        $this->set_relevance($keywords, $data);
      }
      else if (strpos($type[0], 'http://s.opencalais.com/1/type/er') !== FALSE) {
        $er_type = preg_replace('@http://s.opencalais.com/1/type/er/@ims', '', $type[0]);
        $func = 'apply_resolved_' . drupal_strtolower(str_replace('/', '_', $er_type));
        
        if (!method_exists($this, $func)) {
          $func = 'apply_resolved_data';
        }
        
        $this->{$func}($keywords, $guid, $data);
      }
    }
  }

  /**
   * Extracts the entities from the returned data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $type The RDF type for this record
   * @param $guid The guid for the current Calais Term
   * @param $data The indexed triple for the current Calais Term/GUID
   */
  protected function extract_entities(&$keywords, $type, $guid, $data) {
    $entity_type_guid = $type[0];
    $entity_type = preg_replace('/.*\//ims', '', $entity_type_guid);

    $entity_value = $data[self::CALAIS_NAME][0];
    
    if (!property_exists($keywords, $entity_type)) {
      $keywords->{$entity_type} = new CalaisMetadata($entity_type_guid, $entity_type);
    }

    $metadata = &$keywords->{$entity_type};
    $metadata->set_term_data($guid, $entity_value);        
  }
  
  /**
   * Extracts the events & facts from the returned data. For now it is considered best that all
   * Events & Facts are put into one Vocabulary and identified by their type.
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $type The RDF type for this record
   * @param $guid The guid for the current Calais Term
   * @param $data The indexed triple for the current Calais Term/GUID
   */
   protected function extract_events(&$keywords, $type, $guid, $data) {
     $type_guid = $type[0];
     $type_value = preg_replace('/.*\//ims', '', $type_guid);
     $type_value = calais_api_make_readable($type_value);
   
     if (!property_exists($keywords, 'EventsFacts')) {
       $keywords->EventsFacts = new CalaisMetadata('http://drupal.org/project/opencalais/EventsFacts', 'EventsFacts');
     }

     // Not sure if the best number for relevance of an Event/Fact, for now 1 will always include
     $keywords->EventsFacts->set_term_data($type_guid, $type_value, 1.000);      
   }
   
  /**
   * Extracts the document level categorization from the returned data.
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $type The RDF type for this record
   * @param $guid The guid for the current Calais Term
   * @param $data The indexed triple for the current Calais Term/GUID
   */
   protected function extract_categories(&$keywords, $type, $guid, $data) {
     $cat_guid = $data['http://s.opencalais.com/1/pred/category'][0];
     $cat_val = $data['http://s.opencalais.com/1/pred/categoryName'][0];
     $cat_score = $data['http://s.opencalais.com/1/pred/score'][0];
     $cat_score = $cat_score ? $cat_score : 0;

     if (!property_exists($keywords, 'CalaisDocumentCategory')) {
       $keywords->CalaisDocumentCategory = new CalaisMetadata($type[0], 'CalaisDocumentCategory');
     }

     $cat_val = preg_replace('/_.*/ims', '', $cat_val); // remove everything after the first underscore
     $keywords->CalaisDocumentCategory->set_term_data($cat_guid, $cat_val, $cat_score);      
   }

  /**
   * Extracts the relevance score from the returned data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $data The indexed triple for the current Calais Term
   */
  protected function set_relevance(&$keywords, $data) {
    $subject = $data[self::CALAIS_SUBJECT][0];
    $relevance = $data['http://s.opencalais.com/1/pred/relevance'][0];
    foreach ($keywords as &$entity) {
      if ($entity->has_guid($subject)) {
        $entity->set_term_relevance($subject, $relevance);      
      }
    }
  }

  /**
   * Extracts the basic resolved entity data.
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The resolved entity guid
   * @param $data The indexed triple for the current Calais Term
   * @param $extra Any extra domain specific data
   */
  protected function apply_resolved_data(&$keywords, $guid, $data, $extra = array()) {
    $subject = $data[self::CALAIS_SUBJECT][0];
    $resolved_name = $data[self::CALAIS_NAME][0];
      
    foreach ($keywords as &$entity) {
      if ($entity->has_guid($subject)) {
        $entity->set_term_resolved_data($subject, $resolved_name, $guid, $extra);      
      }
    }
  }

  /**
   * Extracts the disambiguation geo city data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The resolved entity guid
   * @param $data The indexed triple for the current Calais Term
   */
  protected function apply_resolved_geo_city(&$keywords, $guid, $data) {
    $extra = array(
      'lat' => $data[self::CALAIS_LAT][0],
      'lon' => $data[self::CALAIS_LON][0],
      'shortname' => $data['http://s.opencalais.com/1/pred/shortname'][0],
      'containedbystate' => $data['http://s.opencalais.com/1/pred/containedbystate'][0],
      'containedbycountry' => $data['http://s.opencalais.com/1/pred/containedbycountry'][0],
    );
    $this->apply_resolved_data($keywords, $guid, $data, $extra);
  }
  
  /**
   * Extracts the disambiguation geo province or state data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The resolved entity guid
   * @param $data The indexed triple for the current Calais Term
   */
  protected function apply_resolved_geo_provinceorstate(&$keywords, $guid, $data) {
    $extra = array(
      'lat' => $data[self::CALAIS_LAT][0],
      'lon' => $data[self::CALAIS_LON][0],
      'shortname' => $data['http://s.opencalais.com/1/pred/shortname'][0],
      'containedbycountry' => $data['http://s.opencalais.com/1/pred/containedbycountry'][0],
    );
    $this->apply_resolved_data($keywords, $guid, $data, $extra);
  }
  
  /**
   * Extracts the disambiguation geo country data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The resolved entity guid
   * @param $data The indexed triple for the current Calais Term
   */
  protected function apply_resolved_geo_country(&$keywords, $guid, $data) {
    $extra = array(
      'lat' => $data[self::CALAIS_LAT][0],
      'lon' => $data[self::CALAIS_LON][0],
    );
    $this->apply_resolved_data($keywords, $guid, $data, $extra);
  }
  
  /**
   * Extracts the disambiguated company data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The resolved entity guid
   * @param $data The indexed triple for the current Calais Term
   */
  protected function apply_resolved_company(&$keywords, $guid, $data) {
   $extra = array(
     'score' => $data[self::CALAIS_SCORE][0],
     'ticker' => $data['http://s.opencalais.com/1/pred/ticker'][0],
   );
   $this->apply_resolved_data($keywords, $guid, $data, $extra);
  }
  
  /**
   * Extracts the disambiguation geo country data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The resolved entity guid
   * @param $data The indexed triple for the current Calais Term
   */
  protected function apply_resolved_product_electronics(&$keywords, $guid, $data) {
    $extra = array(
      'score' => $data[self::CALAIS_SCORE][0],
    );
    $this->apply_resolved_data($keywords, $guid, $data, $extra);
  }
}
