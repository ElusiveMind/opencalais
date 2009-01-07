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
  
  private $defaults = array(
    'host' => 'http://api.opencalais.com',
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
    foreach ($this->triples as $guid => $indx) {
      $this->extract_entities($this->keywords, $guid, $indx);
      $this->extract_events($this->keywords, $guid, $indx);
      $this->extract_categories($this->keywords, $guid, $indx);
    }

    // Once we have the entities, then get additional metadata about them
    foreach ($this->triples as $guid => $indx) {
      $this->extract_relevance($this->keywords, $indx);
      $this->extract_geo($this->keywords, $indx);
      $this->disambiguate_company($this->keywords, $indx);
    }

    return $this->keywords;
  }

  /**
   * Extracts the entities from the returned data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The guid for the current Calais Term
   * @param $indx The indexed triple for the current Calais Term/GUID
   */
  protected function extract_entities(&$keywords, $guid, $indx) {
    $type = $indx[self::RDF_TYPE];
    if (isset($type) && strpos($type[0], "http://s.opencalais.com/1/type/em/e") !== FALSE) {
      
      $keyword_type_guid = $type[0];
      $keyword_type = preg_replace('/.*\//ims', '', $keyword_type_guid);

      $keyword_value = $indx['http://s.opencalais.com/1/pred/name'][0];
      
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
     $type = $indx[self::RDF_TYPE];
     if (isset($type) && strpos($type[0], 'http://s.opencalais.com/1/type/em/r') === 0) {
     
       $type_guid = $type[0];
       $type_value = preg_replace('/.*\//ims', '', $type_guid);
       $type_value = calais_api_make_readable($type_value);
     
       if (!property_exists($keywords, 'EventsFacts')) {
         $keywords->EventsFacts = new CalaisMetadata('http://drupal.org/project/opencalais/EventsFacts', 'EventsFacts');
       }
 
       $metadata = &$keywords->EventsFacts;
       // Not sure if the best number for relevance of an Event/Fact, for now 1 will always include
       $metadata->add_term($type_guid, $type_value, 1.000);      
     }
   }
   
  /**
   * Extracts the document level categorization from the returned data.
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The guid for the current Calais Term
   * @param $indx The indexed triple for the current Calais Term/GUID
   */
   protected function extract_categories(&$keywords, $guid, $indx) {
     $type = $indx[self::RDF_TYPE];
     if (isset($type) && $type[0] == 'http://s.opencalais.com/1/type/cat/DocCat') {
     
       $cat_guid = $indx['http://s.opencalais.com/1/pred/category'][0];
       $cat_val = $indx['http://s.opencalais.com/1/pred/categoryName'][0];
       $cat_score = $indx['http://s.opencalais.com/1/pred/score'][0];

       if (!property_exists($keywords, 'CalaisDocumentCategory')) {
         $keywords->CalaisDocumentCategory = new CalaisMetadata($type[0], 'CalaisDocumentCategory');
       }

       $cat_val = preg_replace('/_.*/ims', '', $cat_val); // remove everything after the first underscore
       $keywords->CalaisDocumentCategory->add_term($cat_guid, $cat_val, $cat_score);      
     }
   }

  /**
   * Extracts the relevance score from the returned data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $indx The indexed triple for the current Calais Term
   */
  protected function extract_relevance(&$keywords, $indx) {
    $type = $indx[self::RDF_TYPE];
    if (isset($type) && $type[0] == 'http://s.opencalais.com/1/type/sys/RelevanceInfo') {
      $subject = $indx[self::CALAIS_SUBJECT][0];
      $relevance = $indx['http://s.opencalais.com/1/pred/relevance'][0];
      $this->assign_relevance($keywords, $subject, $relevance);
    }
  }

  /**
   * Assign a relevance score to the supplied subject.
   */
  protected function assign_relevance(&$keywords, $subject, $relevance) {
    foreach ($keywords as &$entity) {
      if ($entity->has_guid($subject)) {
        $entity->add_relevance($subject, $relevance);      
      }
    }
  }

  /**
   * Extracts the geo disambiguation data
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $indx The indexed triple for the current Calais Term
   */
  protected function extract_geo(&$keywords, $indx) {
    $type = $indx[self::RDF_TYPE];
    if (isset($type) && strpos($type[0], 'http://s.opencalais.com/1/type/er/Geo') === 0) {
      $subject = $indx[self::CALAIS_SUBJECT][0];
      $normalized = $indx['http://s.opencalais.com/1/pred/name'][0];
      $lat = $indx['http://s.opencalais.com/1/pred/latitude'][0];
      $lon = $indx['http://s.opencalais.com/1/pred/longitude'][0];
      
      foreach ($keywords as &$entity) {
        if ($entity->has_guid($subject)) {
          $entity->add_geo($subject, $normalized, $lat, $lon);      
        }
      }
    }
  }
  
  /**
   * Extracts the disambiguated company name from the response
   * 
   * @param $keywords The result array for CalaisMetadata
   * @param $indx The indexed triple for the current Calais Term
   */
  protected function disambiguate_company(&$keywords, $indx) {
    $type = $indx[self::RDF_TYPE];
    if (isset($type) && $type[0] == 'http://s.opencalais.com/1/type/er/Company') {
      $subject = $indx[self::CALAIS_SUBJECT][0];
      $resolvedName = $indx['http://s.opencalais.com/1/pred/name'][0];
      $keywords->Company->set_resolved_name($subject, $resolvedName);
    }
  }
}
