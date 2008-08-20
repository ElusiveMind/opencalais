<?php
/*
  Copyright (C) 2008 by Phase2 Technology.
  Author(s): Frank Febbraro, Irakli Nadareishvili

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY. See the LICENSE.txt file for more details.

  $Id$
*/
/**
 * @file
 */

define('CALAIS_TERM_MAXLENGTH', 1000);

/**
 * Implementation of hook_theme();
 */
function calais_theme() {
  return array(
    'calais_suggestions' => array(
      'arguments' => array('vid', 'terms'),
    ),
  );
}

/**
 * Alter the standard node edit form to remove calais taxonomies so that they are
 * only editable via the Calais tab.
 * TODO: Make this configurable so the user can specify how/where they want to edit calais terms.
 */
function calais_form_alter(&$form, $form_state, $form_id) {
  if (isset($form['type']) && $form['type']['#value'] .'_node_form' == $form_id) {
    
    $node = $form['#node'];
    $vocabs = calais_get_vocabularies($node->type);
    
    foreach ($vocabs as $vocabulary) {
      _calais_disable_taxonomy_field($form['taxonomy']['tags'][$vocabulary->vid]);
    }
  }
}

/**
 * This function implements what is essentially disabling of a taxonomy field, 
 * but leaving it on the form so that values can be recreated by 
 * <code>taxonomy_node_save()</code> upon save.
 */
function _calais_disable_taxonomy_field(&$field) {
  $field['#type'] = 'hidden';
  $field['#maxlength'] = CALAIS_TERM_MAXLENGTH;  
}

/**
 * Render the form for the Calais taxonomy & suggestions
 */
function calais_keywords_form(&$form_state, $node) {
  drupal_set_title(t('Calais Terms'));
  
  $path = drupal_get_path('module', 'calais');
  drupal_add_css("$path/calais.css");
  drupal_add_js("$path/calais.js", 'module');

  $vocabs = calais_get_vocabularies($node->type);

  $form          = array();
  $form['#node'] = $node;
  $form['nid']   = array('#type' => 'value', '#value' => $node->nid);
  $form['vid']   = array('#type' => 'value', '#value' => $node->vid);

  foreach ($vocabs as $vocabulary) {
    $keywords = calais_get_keywords($node->nid, $node->type, $vocabulary->vid);
    $suggestions = theme('calais_suggestions', $vocabulary->vid, $keywords[$vocabulary->vid]);
    $current_tags = _calais_get_current_tags($node, $vocabulary->vid);
    $has_keywords = sizeof($keywords[$vocabulary->vid]);
    
    // If there are keywords available, make these vocabs appear at the top of the long list
    if ($has_keywords) {
      $vocabulary->weight -= 100;
    }
    
    $form['calais'][$vocabulary->vid] = array(
      '#type' => 'textfield',
      '#title' => $vocabulary->name,
      '#description' => $suggestions,
      '#required' => $vocabulary->required,
      '#default_value' => $current_tags,
      '#autocomplete_path' => 'taxonomy/autocomplete/'. $vocabulary->vid,
      '#weight' => $vocabulary->weight,
      '#size' => 75,
      '#maxlength' => CALAIS_TERM_MAXLENGTH,
    );
    
    if ($has_keywords) {
      $form['calais'][$vocabulary->vid]['#attributes'] = array(
        'class' => 'keywords_available',
      );
    }
  }
  
  $form['calais']['#tree'] = TRUE;
  $form['submit'] = array('#type' => 'submit', '#value' => t('Save'), '#weight' => 45);
  
  return $form;
}

/**
 * Provide validation for the Calais free tagging taxonomy terms.
 *
 * The essence of this was clipped from <code>taxonomy_node_validate()</code>
 */
function calais_keywords_form_validate($form, &$form_state) {
  $node = (object)$form_state['values'];
  
  if (!empty($node->calais)) {
    $terms = $node->calais;
    foreach ($terms as $vid => $vid_value) {
      $vocabulary = taxonomy_vocabulary_load($vid);
      if (empty($vocabulary->tags)) {
        // see form_get_error $key = implode('][', $element['#parents']);
        // on why this is the key
        form_set_error("calais][$vid", t('The %name vocabulary can not be modified in this way. It is not setup for Free Tagging.', array('%name' => $vocabulary->name)));
      }
    }
  }
}

/**
 * Update the node for any Calais keyword modifications.
 */
function calais_keywords_form_submit($form, &$form_state) {
  $node = (object)$form_state['values'];

  foreach ($node->calais as $vid => $vid_value) {
    calais_remove_terms_for_node($node, $vid);
    
    $terms = drupal_explode_tags($vid_value);
    foreach ($terms as $term_name) {
      $term = calais_get_node_term($node->nid, $term_name);
      calais_assign_to_node($vid, $term, $node);
    }
  }

  $form_state['redirect'] = "node/{$node->nid}";
}

/**
 * Remove all terms on a node for a particular vocabulary.
 *
 * @param $node The node to remove terms
 * @param $vid The vocabulary whose terms will be removed from the node
 */
function calais_remove_terms_for_node($node, $vid) {
  $result = db_query('SELECT tid FROM {term_data} WHERE vid = %d', $vid);
  while ($term = db_fetch_object($result)) {
    db_query('DELETE FROM {term_node} WHERE vid = %d AND tid = %d', $node->vid, $term->tid);
  }  
}

/**
 * Process the node and get all specified terms for the current vocabulary
 */
function _calais_get_current_tags($node, $vid) {
  $current_tags = '';
  
  if (isset($node->taxonomy)) {
    $terms = $node->taxonomy;
    $typed_terms = array();
    foreach ($terms as $term) {
      if ($term->vid == $vid) {
        // Commas and quotes in terms are special cases, so encode 'em.
        if (strpos($term->name, ',') !== FALSE || strpos($term->name, '"') !== FALSE) {
          $term->name = '"'. str_replace('"', '""', $term->name) .'"';
        }
        $typed_terms[] = $term->name;
      }
      $current_tags = implode(',', $typed_terms) . (array_key_exists('tags', $terms) ? $terms['tags'][$vid] : NULL);
    }
  }
  
  return $current_tags;
}

/**
 * Theme function to render the suggestions for a particular vocabulary
 */
function theme_calais_suggestions($vid, $terms) {
  if (sizeof($terms)) {
    $suggestions .= "<div class='suggestions'>";
    $suggestions .= t('Calais Suggestions: ');
    foreach ($terms as $term) {
      $suggestions .= "<label class='calais_keyword' for='edit-calais-$vid'>$term</label>";
    }
    $suggestions .= "</div>";
  }
  
  return $suggestions;
}

