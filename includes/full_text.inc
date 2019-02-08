<?php
/**
 * @file
 * Custom relationship handling for full text.
 */

// Relationships.
@include_once 'sites/all/libraries/tuque/FedoraRelationships.php';
define('LIB4RIDORA_RELS_EXT_NAMESPACE', ISLANDORA_RELS_EXT_URI);
const LIB4RIDORA_FULL_TEXT_RELATIONSHIP = 'fullText';


/**
 * Update an object's full text relationship with the information from RELS-INT.
 */
function lib4ridora_update_full_text(AbstractObject $object) {
  module_load_include('inc', 'lib4ridora', 'includes/utilities');
  $is_open_access = FALSE;
  $accepted_or_published = FALSE;
  $pdf_datastreams = lib4ridora_retrieve_filtered_pdfs($object);
  $pdfa_suffix = variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A');
  foreach ($pdf_datastreams as $datastream) {
    $info = lib4ridora_get_embargo_info($datastream);
    // skip evaluating PDF/A files at Lib4RI if they are 'private' (default)
    if ( $info['availability'] == "private" && substr(strval($datastream->id),0-strlen($pdfa_suffix)) == $pdfa_suffix ) {
      continue;
    }
    if (in_array($info['document_version'], array(
      'accepted version',
      'published version',
    ))) {
    	$accepted_or_published = true;
    	if ( $info['availability'] == "public" ) {
          $is_open_access = TRUE;
        }
    }
    if ($info['availability'] == 'date') {
      $embargo_date = $datastream->relationships->get(LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate('embargo_date'));
      $embargo_date = reset($embargo_date);
      // Check if embargo date has passed.
      if (REQUEST_TIME > strtotime($embargo_date['object']['value'])) {
        $is_open_access = TRUE;
      }
    }
  }
  if (!$pdf_datastreams || !$accepted_or_published) {
    $relationship_value = 'No Full Text';
  }
  elseif ($is_open_access) {
    $relationship_value = 'Open Access';
  }
  else {
    $relationship_value = 'Restricted';
  }
  $current_relationship_triples = $object->relationships->get(
    LIB4RIDORA_RELS_EXT_NAMESPACE,
    LIB4RIDORA_FULL_TEXT_RELATIONSHIP
  );
  $current_relationship_value = isset($current_relationship_triples[0]['object']['value']) ?
    $current_relationship_triples[0]['object']['value'] :
    NULL;
  if ($relationship_value != $current_relationship_value) {
    $object->relationships->autoCommit = FALSE;
    $object->relationships->remove(
      LIB4RIDORA_RELS_EXT_NAMESPACE,
      LIB4RIDORA_FULL_TEXT_RELATIONSHIP
    );
    $object->relationships->add(
      LIB4RIDORA_RELS_EXT_NAMESPACE,
      LIB4RIDORA_FULL_TEXT_RELATIONSHIP,
      $relationship_value,
      RELS_TYPE_PLAIN_LITERAL
    );
    $object->relationships->commitRelationships();
  }
}

/**
 * Ingest step callback to update the full text.
 */
function lib4ridora_update_full_text_ingest_step(array &$form_state) {
  module_load_include('inc', 'islandora', 'ingest.form');

  $object = islandora_ingest_form_get_object($form_state);
  lib4ridora_update_full_text($object);
}
