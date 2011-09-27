<?php
/**
 *  Provides default presets
 *  geo_tags - Provides geographic tagging
 */
function opencalais_defaults_opencalais_default_preset(){
  $export = array();

  $preset = new stdClass;
  $preset->disabled = FALSE; /* Edit this to true to make a default preset disabled initially */
  $preset->api_version = 1;
  $preset->name = 'geo_tags';
  $preset->title = 'Geographic Tagging';
  $preset->description = 'Applies geographical vocabularies useful for mapping stories';
  $preset->process = 0;
  $preset->config = array(
    'City' => array(
      'threshold' => '0.15',
    ),
    'Country' => array(
      'threshold' => '0.25',
    ),
    'ProvinceOrState' => array(
      'threshold' => '0.25',
    ),
    'Region' => array(
      'threshold' => '0.15',
    ),
    'Continent' => array(
      'threshold' => '0.25',
    ),
  );
  $export['geo_tags'] = $preset;
  
  $preset = new stdClass;
  $preset->disabled = FALSE; /* Edit this to true to make a default preset disabled initially */
  $preset->api_version = 1;
  $preset->name = 'news_tags';
  $preset->title = 'General News';
  $preset->description = 'Applies a group of settings useful for general news stories';
  $preset->process = 0;
  $preset->config = array(
    'City' => array(
      'threshold' => '0.15',
    ),
    'Country' => array(
      'threshold' => '0.25',
    ),
    'ProvinceOrState' => array(
      'threshold' => '0.25',
    ),
    'Person' => array(
      'threshold' => '0.25',
    ),
    'Organization' => array(
      'threshold' => '0.25',
    ),
    'Company' => array(
      'threshold' => '0.25',
    ),
    'SocialTags' => array(
      'threshold' => '0.50',
    ),
    'Holiday' => array(
      'threshold' => '0.50',
    ),
  );
  $export['news_tags'] = $preset;
  
  
  return $export;
}
 