<?php

/**
 * @file
 * Single PDF upload form.
 */

/**
 * Form building function; build form to upload a single PDF.
 */
function lib4ridora_single_pdf_upload_form($form, &$form_state, AbstractObject $object) {
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/pdf-upload.form');
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/embargo.form');
  $options = lib4ridora_multi_pdf_get_options();
  $defaults = lib4ridora_multi_pdf_defaults_properties(TRUE);

  $form['#tree'] = TRUE;
  $form['pdf'] = array(
    '#type' => 'managed_file',
    '#title' => t('Upload PDF'),
    '#required' => TRUE,
    '#upload_validators' => array(
      'file_validate_extensions' => array('pdf'),
    ),
  );
  $form['file'] = array(
    '#type' => 'fieldset',
    '#title' => t('PDF Information'),
    'document_version' => array(
      '#type' => 'select',
      '#title' => t('Document Version'),
      '#options' => $options['document_version'],
      '#default_value' => $defaults['document_version'],
    ),
    'availability' => array(
      '#type' => 'select',
      '#title' => t('Document Availablility'),
      '#options' => $options['availability'],
      '#attributes' => array(
        'class' => array(
          'lib4ridora-embargo-select',
        ),
      ),
      '#default_value' => $defaults['availability'],
    ),
    'embargo_date' => array(
      '#type' => 'date',
      '#title' => t('Embargo Date'),
      '#title_display' => 'invisible',
      '#states' => array(
        'visible' => array(
          'fieldset#edit-file select.lib4ridora-embargo-select' => array(
            'value' => 'date',
          ),
        ),
      ),
      '#default_value' => $defaults['embargo_date'],
    ),
    'use_permission' => array(
      '#type' => 'select',
      '#title' => t('Conditions for Reuse'),
      '#options' => $options['use_permission'],
      '#default_value' => $defaults['use_permission'],
    ),
  );

  $form['add'] = array(
    '#type' => 'submit',
    '#value' => t('Add PDF'),
    '#prefix' => '<br>',
    '#suffix' => '<br><br><br>',
  );
  
  // Adding a JS function to switch the availability to 'private' for 'internal documents'.
  // This shall help the user, to enforce 'private' in DORA we better should use a valiation or submit hook.
  // drupal_add_js( drupal_get_path('module','lib4ridora') . '/js/lib4ridora.internal_as_private.js', 'file' );
  $jsPath = drupal_get_path('module','lib4ridora') . '/js/lib4ridora.internal_as_private.txt';
  drupal_add_js( file_get_contents($jsPath), 'inline' );	// this is less 'caching-resistent'
  $form['file']['document_version']['#attributes']['onchange'] = "PdfInternalAsPrivate('');"; // here we not need an UUID or a PDF-ID

  return $form;
}

/**
 * Determine the index of the next PDF.
 *
 * @param array $info
 *   An associative array mapping PDF* datastream IDs to values which are not
 *   used here.
 *
 * @return int
 *   The index of the currently highest PDF index. If there are none (or only
 *   a "PDF", then we return "1".
 */
function lib4ridora_determine_max_extra_pdf_index($info) {
  unset($info['PDF']);
  if (empty($info)) {
    return 1;
  }

  $max_dsid = max(array_keys($info));
  return substr($max_dsid, strlen(variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF')));
}

/**
 * Form submission handler; attach the uploaded PDF to the object.
 */
function lib4ridora_single_pdf_upload_form_submit(&$form, &$form_state) {
  $object = islandora_object_load($form_state['build_info']['args'][0]);

  $info = lib4ridora_assemble_pdf_info($object);
  $max_index = lib4ridora_determine_max_extra_pdf_index($info);

  $forced_intdoc_as_priv = false;
  foreach( array('values','input') as $idxMain ) {
    if ( stripos($form_state[$idxMain]['file']['document_version'],'internal') !== false && stripos($form_state[$idxMain]['file']['availability'],'private') === false ) {
      $form_state[$idxMain]['file']['availability'] = 'private';
      $forced_intdoc_as_priv = true;
    }
  }
  if ( $forced_intdoc_as_priv ) { drupal_set_message("The availability of the 'Internal Document' was set to 'Private'.", 'warning'); }

  $new_token = '--NEW--';
  $info[$new_token] = $form_state['values']['file'];

  $primary = lib4ridora_get_primary_file_for_datastream_info($info);
  $next_dsid = t('@prefix@index', array(
    '@prefix' => variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF'),
    '@index' => $max_index + 1,
  ));
  if ($primary == $new_token) {
    if (isset($object['PDF'])) {
      // Old "PDF" (if present) gets bumped to the end.
      $new_old = $object->constructDatastream($next_dsid, 'M');
      $props = array('label', 'mimetype', 'state');
      foreach ($props as $p) {
        $new_old->$p = $object['PDF']->$p;
      }
      $new_old->logMessage = "Copied from '{$object['id']}['PDF'].";
      $temp_file = drupal_tempnam('temporary://', 'datastream');
      $object['PDF']->getContent($temp_file);
      $new_old->setContentFromFile($temp_file);
      drupal_unlink($temp_file);
      $object->ingestDatastream($new_old);
      lib4ridora_update_datastream_relationships($new_old, $info['PDF']);
      $to_purge = array('PDF', 'TN', 'PREVIEW', 'FULL_TEXT', 'PDF'.variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A') );
      array_map(array($object, 'purgeDatastream'), $to_purge);
    }

    // New file becomes the "PDF".
    $new_dsid = 'PDF';
  }
  else {
    // New file gets jammed on the end.
    $new_dsid = $next_dsid;
  }

  $file = file_load($form_state['values']['pdf']);
  $new = $object->constructDatastream($new_dsid, 'M');
  $new->label = $file->filename;
  $new->mimetype = 'application/pdf';
  $new->setContentFromFile($file->uri, FALSE);
  $object->ingestDatastream($new);
  lib4ridora_update_datastream_relationships($new, $info[$new_token]);
  drupal_set_message(t('The PDF file has been added.'));
  $form_state['redirect'] = "islandora/object/{$object->id}";
}

/**
 * Implements validation from the Form API.
 * 
 * @param $form
 *   A copy as structured array containing the elements and properties of the form.
 * @param $form_state
 *   An array that hold the submitted information about the form's current state during processing.
 * 
 * Used here to force the 'private' availability for internal documents.
 */
function lib4ridora_single_pdf_upload_form_validate(&$form, &$form_state) {
  $forced_intdoc_as_priv = false;
  foreach( array('values','input') as $idxMain ) {
    if ( stripos($form_state[$idxMain]['file']['document_version'],'internal') !== false && stripos($form_state[$idxMain]['file']['availability'],'private') === false ) {
      $form_state[$idxMain]['file']['availability'] = 'private';
      $forced_intdoc_as_priv = true;
    }
  }
  if ( $forced_intdoc_as_priv ) { drupal_set_message("The availability of the 'Internal Document' was set to 'Private'.", 'warning'); }
}

/**
 * heloper function for lib4ridora_assemble_pdf_info()
 */
function lib4ridora_assemble_pdf_info_helper(AbstractDatastream $datastream) {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
  return lib4ridora_get_embargo_info($datastream, TRUE, TRUE);
}

/**
 * Get info for all PDFs on the object.
 *
 * @param AbstractObject $object
 *   An object from which to list the PDFs.
 *
 * @return array
 *   An associative array mapping datastream IDs to arrays of values as returned
 *   by lib4ridora_get_embargo_info().
 */
function lib4ridora_assemble_pdf_info(AbstractObject $object) {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');

  $datastreams = iterator_to_array($object);
  $pdf_datastreams = array_filter($datastreams, 'lib4ridora_multi_embargo_pdf_filter');

  return array_map('lib4ridora_assemble_pdf_info_helper', $pdf_datastreams);
}
