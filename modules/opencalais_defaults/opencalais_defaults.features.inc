<?php
/**
 * @file
 * opencalais_export.features.inc
 */

/**
 * Implementation of hook_ctools_plugin_api().
 */
function opencalais_defaults_ctools_plugin_api() {
  list($module, $api) = func_get_args();
  if ($module == "opencalais" && $api == "opencalais_default_presets") {
    return array("version" => "1");
  }
}
