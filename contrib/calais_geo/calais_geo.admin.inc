<?php
/*
  Copyright (C) 2008 by Phase2 Technology.
  Author(s): Frank Febbraro

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY. See the LICENSE.txt file for more details.

  $Id$
 */
/**
 * @file
 */

/**
 * Settings form to configure which Calais vocabularies are presented for mapping
 */
function calais_geo_settings_form() {
  $entities = calais_get_entity_vocabularies();

  $form['calais_geo_vocabularies'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Which Calais Vocabularies do you want available for geomapping?'),
    '#default_value' => variable_get('calais_geo_vocabularies', $entities),
    '#options' => array_flip($entities),
    '#description' => t('Terms in these vocabularies will be available for inclusion in the geomapping settings on the node edit page.'),
  );
  
  return system_settings_form($form);
}