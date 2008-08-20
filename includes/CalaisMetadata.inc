<?php
// $Id$
/**
 * @file CalaisMetadata.inc
 */
 
/**
 * This class represents results from the Calais Web Service.  These results are for 
 * Entities/Facts/Events.
 */
class CalaisMetadata {
  
  public $guid;
  public $type;
  public $terms = array();
  
  /**
   *
   */
  function __construct($guid, $type) {
    $this->guid = $guid;
    $this->type = $type;
  }
  
  function add_term($guid, $subject) {
    $this->terms[$guid] = new CalaisTerm($guid, $subject);
  }

  function add_relevance($guid, $relevance) {
    if ($this->has_guid($guid)) {
      $this->terms[$guid]->relevance = $relevance;      
    }
  }
  
  function has_guid($guid) {
    return array_key_exists($guid, $this->terms);
  }
  
  function has_term_value($value) {
    foreach ($this->terms as $term) {
      if ($term->value == $value) {
        return TRUE;
      }
    }
    return FALSE;
  }
  
  function readable_type() {
    return preg_replace('/(.*?[a-z]{1})([A-Z]{1}.*?)/', '${1} ${2}', $this->type);
  }
}

/**
 * This class represents a specific term result from the Calais Web Service.
 */
class CalaisTerm {
  public $guid;
  public $value;
  public $relevance = 0.0;
  
  /**
   *
   */
  function __construct($guid, $value, $relevance = NULL) {
    $this->guid = $guid;
    $this->value = $value;
    $this->relevance = $relevance;
  }
  
}