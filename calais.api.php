<?php

/**
 * @file
 * Documentation of hooks.
 */

/**
 * Invoked after data has been returned from Calais but before it is processed.
 */
function hook_calais_preprocess(&$node, &$keywords) {
}

/**
 * Invoked after data has been returned from Calais and after it has been processed.
 */
function hook_calais_postprocess(&$node, &$keywords) {
}

/**
 * Invoked before a Node body is sent to Calais.
 */
function hook_calais_body_alter(&$body, $loaded_node) {
}

