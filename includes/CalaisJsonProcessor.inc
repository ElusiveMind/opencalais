<?php

/**
 * @file
 * The parser for Calais JSON responses.
 */

class CalaisJsonProcessor {

  public $docId;
  public $keywords;
  public $json;

  /**
   * Our constructor calls the parse_json method by default.
   *
   * @param string $json
   * The json from the Calais API to be parsed.
   *
   * @return array $metadata;
   * An array of CalaisMetadata objects.
   */
  public function __construct($json = NULL) {
    if (!empty($json)) {
      return $this->parse_json($json);
    }
  }

  /**
   * Parse the json.
   *
   * @param $json
   * The json from the Calais API to be parsed.
   *
   * @return
   * An array of CalaisMetadata objects.
   *
   * @see CalaisMetadata.inc
   */
  public function parse_json($json) {
    $json = (array) $json;
    $this->docId = $json['doc']->info->docId;
    $this->json = $json;
    $this->keywords = new stdClass();
    $this->build_entities($this->keywords);
    $this->extract_entity_metadata($this->keywords);
    return $this->keywords;
  }

  /**
   * Build the set of entities from this json nodes returned from Calais.
   *
   * @param $keywords
   * The object containing type keyed CalaisMetadata
   */
  protected function build_entities(&$keywords) {
    foreach ($this->json as $guid => $data) {

      if (empty($data->_typeGroup)) {
        continue;
      }
      $type = (!empty($data->_type)) ? $data->_type : NULL;
      $type_group = $data->_typeGroup;

      switch ($type_group) {
        case 'socialTag':
          $this->extract_tags($keywords, $type, $guid, $data);
          break;
        case 'entities':
          $this->extract_entities($keywords, $type, $guid, $data);
          break;
        case 'relations':
          //$this->extract_relations($keywords, $type, $guid, $data);
          break;
        case 'categories':
          //$this->extract_categories($keywords, $type, $guid, $data);
          break;
        case 'events':
          //$this->extract_events($keywords, $type, $guid, $data);
          break;
        default:
          break;
      }
    }
  }
  protected function extract_relations(&$keywords, $type, $guid, $data) {
    switch ($type) {
      case 'PersonCareer':

      case 'ManMadeDisaster':
        $name = $data->manmadedisaster;
        break;
      case 'Quotation':
        $name = $this->quotation;
        break;
      case 'PersonCareer':
        $name = $this->position;
        $extras = array(
          'careertype' => $this->careertype,
          'status' => $this->status,
        );
        break;
      default:
        break;
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
    foreach ($this->json as $guid => $data) {

      if (empty($data->_type)) {
        continue;
      }

      if (!empty($data->relevance)) {
        $this->set_relevance($keywords, $guid, $data);
      }

      $er_type = drupal_strtolower($data->_type);
      $method = 'apply_resolved_' . $er_type;

      if (!method_exists($this, $method)) {
        $method = 'apply_resolved_data';
      }

      $this->{$method}($keywords, $guid, $data, $er_type);
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
    $entity_type_guid = $guid;
    $entity_type = $data->_type;
    $entity_value = $data->name;

    if (!property_exists($keywords, $entity_type)) {
      $keywords->{$entity_type} = new CalaisMetadata($entity_type_guid, $entity_type);
    }

    $metadata = &$keywords->{$entity_type};
    $metadata->set_term_data($guid, $entity_value, $data->relevance);
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
    $type_value = opencalais_api_make_readable($type_value);

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

    $cat_guid = $data['http://s.opencalais.com/1/pred/docId'][0];
    $cat_val = $data['http://s.opencalais.com/1/pred/name'][0];
    $cat_score = array_key_exists('http://s.opencalais.com/1/pred/score', $data) ? $data['http://s.opencalais.com/1/pred/score'][0] : 0;

    if (!property_exists($keywords, 'CalaisDocumentCategory')) {
      $keywords->CalaisDocumentCategory = new CalaisMetadata($type[0], 'CalaisDocumentCategory');
    }

    $cat_val = preg_replace('/_.*/ims', '', $cat_val); // remove everything after the first underscore
    $keywords->CalaisDocumentCategory->set_term_data($cat_guid, $cat_val, $cat_score);
  }

  /**
   * Extracts the Social Tags from the returned data.
   *
   * @param $keywords The result array for CalaisMetadata
   * @param $type The RDF type for this record
   * @param $guid The guid for the current Calais Term
   * @param $data The indexed triple for the current Calais Term/GUID
   */
  protected function extract_tags(&$keywords, $type, $guid, $data) {
    $tag_value = $data->name;
    $importance = $data->importance;
    // This is built off of the Calais documentation:
    // A topic extracted by Categorization with a score higher than 0.6
    // will also be extracted as a SocialTag. If its score is higher
    // than 0.8, its importance (as a SocialTag) will be set to 1. If
    // the score is between 0.6 and 0.8 its importance is set to 2.
    switch ($importance) {
      case 1:
        $tag_score = 0.9;
        break;
      case 2:
        $tag_score = 0.7;
        break;
      default:
        $tag_score = 0;
    }
    if (!property_exists($keywords, 'SocialTags')) {
      $keywords->SocialTags = new CalaisMetadata($guid, 'SocialTags');
    }

    // When certain Document Category tags get here, they have underscores, remove them
    $tag_value = preg_replace('/_.*/ims', '', $tag_value); // remove everything after the first underscore
    $keywords->SocialTags->set_term_data($guid, $tag_value, $tag_score);
  }

  /**
   * Extracts the relevance score from the returned data
   *
   * @param $keywords The result array for CalaisMetadata
   * @param $data The indexed triple for the current Calais Term
   */
  protected function set_relevance(&$keywords, $guid, $data) {
    $subject = $guid;
    $relevance = $data->relevance;
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
  protected function apply_resolved_data(&$keywords, $guid, $data, $type, $extra = array()) {
    $subject = $guid;

    if (empty($data->name)) {
      return;
    }

    $resolved_name = $data->name;
    foreach ($keywords as &$entity) {
      if ($entity->has_guid($subject)) {
        $entity->set_term_resolved_data($subject, $resolved_name, $guid, $type, $extra);
      }
    }
  }

  /**
   * Extracts the disambiguation geo city data
   *
   * @param $keywords
   * The result array for CalaisMetadata
   *
   * @param $guid
   * The resolved entity guid
   *
   * @param $data
   * The indexed json node for the current Calais Term
   */
  protected function apply_resolved_city(&$keywords, $guid, $data) {
    $latitude = (!empty($data->resolutions[0]->latitude)) ? $data->resolutions[0]->latitude : NULL;
    $longitude = (!empty($data->resolutions[0]->longitude)) ? $data->resolutions[0]->longitude : NULL;
    $shortname = (!empty($data->resolutions[0]->shortname)) ? $data->resolutions[0]->shortname : NULL;
    $containedbycountry = (!empty($data->resolutions[0]->containedbycountry)) ? $data->resolutions[0]->containedbycountry : NULL;
    $containedbystate = (!empty($data->resolutions[0]->containedbystate)) ? $data->resolutions[0]->containedbystate : NULL;

    $extra = array(
      'latitude' => $latitude,
      'longitude' => $longitude,
      'shortname' => $shortname,
      'containedbycountry' => $containedbycountry,
      'containedbystate' => $containedbystate,
    );

    $this->apply_resolved_data($keywords, $guid, $data, 'city', $extra);
  }

  /**
   * Extracts the disambiguation geo province or state data
   *
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The resolved entity guid
   * @param $data The indexed json node for the current Calais Term
   */
  protected function apply_resolved_provinceorstate(&$keywords, $guid, $data) {
    $containedbycountry = isset($data->resolutions[0]->containedbycountry) ? $data->resolutions[0]->containedbycountry : NULL;
    $shortname = isset($data->resolutions[0]->shortname) ? $data->resolutions[0]->shortname : NULL;

    $extra = array(
      'latitude' => $data->resolutions[0]->latitude,
      'longitude' => $data->resolutions[0]->longitude,
      'shortname' => $data->resolutions[0]->shortname,
      'containedbycountry' => $data->resolutions[0]->containedbycountry,
      'permid' => $data->resolutions[0]->permid,
    );
    $this->apply_resolved_data($keywords, $guid, $data, 'province_or_state', $extra);
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
      'latitude' => $data->resolutions[0]->latitude,
      'longitude' => $data->resolutions[0]->longitude,
      'permid' => $data->resolutions[0]->permid,
    );
    $this->apply_resolved_data($keywords, $guid, $data, 'country', $extra);
  }

  /**
   * Extracts the disambiguated company data
   *
   * @param $keywords The result array for CalaisMetadata
   * @param $guid The resolved entity guid
   * @param $data The indexed triple for the current Calais Term
   */
  protected function apply_resolved_company(&$keywords, $guid, $data) {
    if (!empty($data->resolutions)) {
      $score = $data->resolutions[0]->score;
      $ticker = (!empty($data->resolutions[0]->ticker)) ? $data->resolutions[0]->ticker : NULL;
      $permid_url = (!empty($data->resolutions[0]->id)) ? $data->resolutions[0]->id : NULL;
      $permid = (!empty($data->resolutions[0]->permid)) ? $data->resolutions[0]->permid : NULL;
      $legal_name = (!empty($data->resolutions[0]->name)) ? $data->resolutions[0]->name : NULL;

      $extra = array(
        'ticker' => $ticker,
        'score' => $score,
        'permid_url' => $permid_url,
        'permid' => $permid,
        'legalname' => $legal_name,
      );
      $this->apply_resolved_data($keywords, $guid, $data, 'company', $extra);
    }
  }
}
