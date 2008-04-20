<?php
// $Id$

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
      unset($form['taxonomy']['tags'][$vocabulary->vid]);
    }
  }
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
    $keywords = calais_get_keywords($node->nid, $vocabulary->vid);
    $suggestions = theme('calais_suggestions', $vocabulary->vid, $keywords[$vocabulary->vid]);
    $current_tags = _calais_get_current_tags($node, $vocabulary->vid);
    $has_keywords = sizeof($keywords[$vocabulary->vid]);
    
    $form['taxonomy']['tags'][$vocabulary->vid] = array(
      '#type' => 'textfield',
      '#title' => $vocabulary->name,
      '#description' => $suggestions,
      '#required' => $vocabulary->required,
      '#default_value' => $current_tags,
      '#autocomplete_path' => 'taxonomy/autocomplete/'. $vocabulary->vid,
      '#weight' => $vocabulary->weight,
      '#size' => 75,
      '#maxlength' => 500,
    );
    
    if ($has_keywords) {
      $form['taxonomy']['tags'][$vocabulary->vid]['#attributes'] = array(
        'class' => 'keywords_available',
      );
    }
  }
  
  $form['taxonomy']['#tree'] = TRUE;
  $form['submit'] = array('#type' => 'submit', '#value' => t('Submit'), '#weight' => 45);
  
  return $form;
}

/**
 * Provide validation for the Calais free tagging taxonomy terms.
 */
function calais_keywords_form_validate($form, &$form_state) {
  $node = (object)$form_state['values'];
  
  taxonomy_node_validate($node);
}

/**
 * Update the node for any Calais keyword modifications.
 */
function calais_keywords_form_submit($form, &$form_state) {
  
  //____debug($form_state);
  $node = (object)$form_state['values'];
  //____debug($node);
  taxonomy_node_save($node, $node->taxonomy);
  $form_state['redirect'] = "node/{$node->nid}";
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
      $suggestions .= "<label class='calais_keyword' for='edit-taxonomy-tags-$vid'>$term</label>";
    }
    $suggestions .= "</div>";
  }
  
  return $suggestions;
}

