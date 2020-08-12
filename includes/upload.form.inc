<?php

/**
 * @file
 * Ingest step form.
 */

/**
 * Form building function.
 */
function lib4ridora_upload_form($form, &$form_state) {
  $form['#attached']['js'] = array(
    drupal_get_path('module', 'islandora_plupload') . '/js/element.js' => array(
      'type' => 'file',
      'scope' => 'footer',
    ),
    drupal_get_path('module', 'lib4ridora') . '/js/lib4ridora.js' => array(
      'type' => 'file',
      'scope' => 'footer',
    ),
  );
  $form['#attributes'] = array('id' => 'lib4ridora_upload_form');

  $step_storage =& islandora_ingest_form_get_step_storage($form_state);
  $files = isset($step_storage['files']) ?
    $step_storage['files'] :
    array();
  if (isset($form_state['values']) && !empty($form_state['values'])) {
    $files = array_merge($files, lib4ridora_get_uploaded_files(array('wrapper', 'upload'), $form_state));
  }

  if (isset($form_state['clicked_button']['#parents'])){
      $button = end($form_state['clicked_button']['#parents']);
      if ($button == 'remove') {
        $selected = array_filter($form_state['values']['wrapper']['table_wrapper']['files']);
        $files = array_values(array_diff_key($files, $selected));
      }
  }
  $current_step = islandora_ingest_form_get_step($form_state);
  if (($current_step == 'lib4ridora_multi_pdf_upload' && $form_state['values']['next'] == t('Ingest') && !empty($files)) ||
    ($current_step == 'lib4ridora_multi_pdf_upload' && $form_state['values']['next'] == t('Next') && empty($files))) {

    // Reload the Ingest steps so that the form alters by either adding a next
    // button or reseting it to ingest.
    drupal_static_reset('islandora_ingest_form_get_steps');
  }

  $step_storage['files'] = array();
  foreach ($files as $file) {
    $step_storage['files'][$file->filename] = $file;
  }
  $row_map = function ($file) {
    return array('filename' => $file->filename);
  };

  $form['#tree'] = TRUE;
  $uploadable = explode(' ', variable_get('lib4ridora_uploadable_extensions', 'pdf'));
  $uploadable = array_unique($uploadable);
  $form['#prefix'] = '<div id="lib4ridora-ingest-form-wrapper">';
  $form['#suffix'] = '</div>';
  $form['wrapper'] = array(
    '#type' => 'fieldset',
    '#prefix' => '<div id="lib4ridora-ingest-datastream-upload-wrapper">',
    '#suffix' => '</div>',
    'upload' => array(
      // XXX: Need to explicitly specify the ID to work around a bug in the
      // Drupal "plupload" module: plupload chops of the suffix added to make
      // IDs unique across AJAX requests, which then break how it pulls out the
      // files.
      '#id' => 'edit-wrapper-upload',
      '#type' => 'plupload',
      '#title' => t('Upload files'),
      '#description' => t('Upload any number of files. You will be given a chance to select which is the primary resource later.'),
      '#upload_validators' => array(
        'file_validate_extensions' => array(implode(' ', $uploadable)),
      ),
      // "#autoupload", "#submit_element" values here combined with the
      // Drupal.pupload.uploadCompleteCallback callback to trigger AJAX to
      // refresh the form, so as to populate the table rendered at the bottom.
      '#autoupload' => FALSE,
      '#autosubmit' => TRUE,
      '#submit_element' => '#edit-wrapper-submitter',
      '#plupload_settings' => array(
        'init' => array(
          'UploadComplete' => 'Drupal.plupload.uploadCompleteCallback',
        ),
      ),
    ),
    'submitter' => array(
      // XXX: ID specified for convenience, so it can be specified in the
      // "#submit_element" bit on the "plupload" element.
      '#id' => 'edit-wrapper-submitter',
      '#type' => 'button',
      '#value' => t('Hidden submit'),
      '#ajax' => array(
        'callback' => 'lib4ridora_upload_ajax_submit_callback',
        'wrapper' => 'lib4ridora-ingest-form-wrapper',
        'event' => 'click',
      ),
      '#prefix' => '<div style="display: none;">',
      '#suffix' => '</div>',
    ),
    'table_wrapper' => array(
      '#prefix' => '<div id="edit-wrapper-table-wrapper-file">',
      '#suffix' => '</div>',
      'files' => array(
        '#type' => 'tableselect',
        '#title' => t('Current files'),
        '#header' => array('filename' => t('Filename')),
        '#options' => array_map($row_map, $step_storage['files']),
        '#empty' => t('None.'),
      ),
      'remove' => array(
        '#type' => 'button',
        '#value' => t('Remove selected'),
        '#ajax' => array(
          'callback' => 'lib4ridora_upload_ajax_submit_callback',
          'wrapper' => 'lib4ridora-ingest-form-wrapper',
        ),
      ),
    ),
  );
  return $form;
}

/**
 * AJAX callback for buttons.
 */
function lib4ridora_upload_ajax_submit_callback($form, $form_state) {
  return $form;
}

/**
 * Helper function to get all successfully uploaded objects.
 */
function lib4ridora_get_uploaded_files($parents, &$form_state) {
  $files = array_filter((array) drupal_array_get_nested_value($form_state['values'], $parents), function ($file) {
    return $file['status'] == 'done';
  });

  $map = function ($uploaded_file) {
    $source = $uploaded_file['tmppath'];
    $destination = file_stream_wrapper_uri_normalize(variable_get('file_default_scheme', 'public') . '://' . $uploaded_file['name']);
    // Rename it to its original name, and put it in its final home.
    // Note - not using file_move here because if we call file_get_mime
    // (in file_uri_to_object) while it has a .tmp extension, it horks.
    $destination = file_unmanaged_move($source, $destination, FILE_EXISTS_RENAME);
    $file = plupload_file_uri_to_object($destination);
    $file->status = 0;
    islandora_plupload_file_save($file);

    return $file;
  };

  return array_map($map, $files);
}

/**
 * Merging pdf upload and embargo forms.
 */
function lib4ridora_jpdf_form($form, &$form_state) {
    form_load_include($form_state,'inc', 'lib4ridora', 'includes/upload.form');
    form_load_include($form_state,'inc', 'lib4ridora', 'includes/embargo.form');
    $form = lib4ridora_upload_form($form, $form_state);
    
    $form['accepted_warning'] = array(
        '#type' => 'markup',
        '#markup' => '<div id="accepted-version-warning" class="accepted-version-warning-hidden"> </div>',
    );
    
    $form = lib4ridora_multi_embargo_form($form, $form_state);
    
    return $form;
}

function lib4ridora_jpdf_form_submit($form, &$form_state) {
    form_load_include($form_state,'inc', 'lib4ridora', 'includes/embargo.form');
    lib4ridora_multi_embargo_form_submit($form, $form_state);
}

function lib4ridora_jpdf_form_undo_submit($form, &$form_state) {
    form_load_include($form_state,'inc', 'lib4ridora', 'includes/embargo.form');
    lib4ridora_multi_embargo_form_undo_submit($form, $form_state);
}
