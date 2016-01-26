<?php
/**
 * @file
 * Custom relationship handling for full text.
 */

// Relationships.
const LIB4RIDORA_RELS_EXT_NAMESPACE = ISLANDORA_RELS_EXT_URI;
const LIB4RIDORA_FULL_TEXT_RELATIONSHIP = 'fullText';

/**
 * Update an object's full text relationship with the information from RELS-INT.
 */
function lib4ridora_update_full_text(AbstractObject $object) {
  module_load_include('inc', 'lib4ridora', 'includes/utilities');

  $is_open = NULL;
  $pdf_datastreams = lib4ridora_retrieve_filtered_pdfs($object);
  foreach ($pdf_datastreams as $datastream) {
    $info = lib4ridora_get_embargo_info($datastream);
    if (is_null($info['availability']) && in_array($info['document_version'], array('accepted version', 'published version'))) {
      $is_open = TRUE;
      break;
    }
  }
  if (!$pdf_datastreams) {
    $relationship_value = 'No Full Text';
  }
  elseif ($is_open) {
    $relationship_value = 'Open Access';
  }
  else {
    $relationship_value = 'Restricted';
  }

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

/**
 * Ingest step callback to update the full text.
 */
function lib4ridora_update_full_text_ingest_step(array &$form_state) {
  module_load_include('inc', 'islandora', 'ingest.form');

  $object = islandora_ingest_form_get_object($form_state);
  lib4ridora_update_full_text($object);
}