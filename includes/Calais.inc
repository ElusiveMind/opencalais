<?php

/**
 * @file
 * The main interface to the Calais web service.
 */
interface Calais {

  /**
   * Analyze the content via Calais passing it in as a raw text format.
   *
   * @param $content
   * The content to ship off to Calais for analysis.
   *
   * @param $api_key
   * The API key for OpenCalais.
   *
   * @return object $results
   * The processed Calais results. The raw RDF result is contained in the $this->rdf field.
   */
  public function analyzeRaw($content, $api_key);

  /**
   * Analyze the provided content, passing it to Calais in HTML format .
   *
   * @param $content
   * The HTML content to process.
   *
   * @param $api_key
   * The API key for OpenCalais.
   *
   * @return object $results
   * The processed Calais results. The raw RDF result is contained in the $this->rdf field.
   */
  public function analyzeHTML($content, $api_key);
}
