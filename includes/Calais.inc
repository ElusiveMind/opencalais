<?php

/**
 * @file Calais.inc
 * The main interface to the calais web service
 */

interface Calais {
  
  /**
   * Analyze the content via Calais.
   *
   * @param $content 
   *    The content to ship off to Calais for analysis.
   
   * @param $api_key
   *     The API key for OpenCalais.
   
   * @return object $results
   *    The processed Calais results. The raw RDF result is contained in the $this->rdf field.
   */
  public function analyze($content, $api_key);
  
  /**
   * Analyze the provided content, passing it to Calais in HTML format .
   *
   * @param $content  
   *    The HTML content to process.
   
   * @param $api_key
   *    The API key for OpenCalais.
   
   * @return object $results
   *    The processed Calais results. The raw RDF result is contained in the $this->rdf field.
   */
  public function analyzeHTML($content, $api_key);
}
