<?php

/**
 * @file
 * Implementation of Drush hooks.
 */

/**
 * Implements hook_drush_command().
 */
function lib4ridora_drush_command() {
  return array(
    'lib4ridora_lift_embargoes' => array(
      'description' => 'Lifts expired embargoes.',
      'drupal dependencies' => array(
        'islandora',
        'lib4ridora',
      ),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    ),
  );
}

/**
 * Lift expired embargoes.
 */
function drush_lib4ridora_lift_embargoes() {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
  module_load_include('inc', 'lib4ridora', 'includes/full_text');
  date_default_timezone_set('CET');			// required!
  $now = date("Y-m-d\TH:i:s\Z", time());		// intentionally not gmdate()
  $rels_ext_namespace = LIB4RIDORA_RELS_EXT_NAMESPACE;
  $rels_int_namespace = LIB4RIDORA_RELS_URI;
  $embargo_restricted = variable_get('lib4ridora_term_embargo_restricted', 'Restricted (Embargo)');
  // Look for any embargo's which have expired, and update their RELS-EXT as
  // appropriate.
  // We don't limited this to a date range, as the default behaviour
  // is to set the object's full text to 'Open Access', if any single PDF has an
  // expired embargo.
  // Force the cron to run as user 1 so we don't get locked out of objects.
  $user = user_load(1);
  $repository = islandora_get_tuque_connection($user);

  $query = <<<EOT
PREFIX lib4ridora-rels-ext: <$rels_ext_namespace>
PREFIX lib4ridora-rels-int: <$rels_int_namespace>
SELECT ?object WHERE {
  ?object <fedora-model:state> <fedora-model:Active> ;
          <lib4ridora-rels-ext:fullText> "$embargo_restricted" ;
          <fedora-view:disseminates> ?ds .
  ?ds <fedora-view:mimeType> "application/pdf" ;
      <lib4ridora-rels-int:lib4ridora-multi-embargo-embargo_date> ?date .
  FILTER(?date <= '$now'^^xsd:dateTime)
}
EOT;
  $results = $repository->repository->ri->sparqlQuery($query);
  foreach ($results as $result) {		// new, if embargo-resticted
    $object = islandora_object_load($result['object']['value']);
    if ($object) {
      lib4ridora_update_full_text($object);
    }
  }

  $query = <<<EOT
PREFIX lib4ridora-rels-ext: <$rels_ext_namespace>
PREFIX lib4ridora-rels-int: <$rels_int_namespace>
SELECT ?object WHERE {
  ?object <fedora-model:state> <fedora-model:Active> ;
          <lib4ridora-rels-ext:fullText> "Restricted" ;
          <fedora-view:disseminates> ?ds .
  ?ds <fedora-view:mimeType> "application/pdf" ;
      <lib4ridora-rels-int:lib4ridora-multi-embargo-embargo_date> ?date .
  FILTER(?date <= '$now'^^xsd:dateTime)
}
EOT;
  $results = $repository->repository->ri->sparqlQuery($query);
  foreach ($results as $result) {		// old (before August 2019), quite obsolete!?
    $object = islandora_object_load($result['object']['value']);
    if ($object) {
      lib4ridora_update_full_text($object);
    }
  }
}

