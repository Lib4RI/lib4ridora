<?php

/**
 * @file
 * XML form selector step form.
 */

/**
 * Form with ingest options to override the xml form association.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 * @param bool $editing
 *   If the form is being called from editing.
 *
 * @return array
 *   The Drupal form state.
 */
function lib4ridora_ingest_selector_form(array $form, array &$form_state, $editing = FALSE) {
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/citation.subtypes');
  $default = array(
    'ingest_selector' => 'journal_article',
    'journal_import_method' => 'manual',
    'doi' => "",
  );
  if (!$editing) {
    // Load the defaults from the form step storage if returning to the form.
    $association_step_storage = &islandora_ingest_form_get_step_storage($form_state, 'xml_form_builder_association_step');
    foreach ($default as $index => &$value) {
      if (isset($association_step_storage[$index])) {
        $value = $association_step_storage[$index];
      }
    }
  }
  $form['ingest_selector'] = array(
    '#title' => t('Select a Publication Type'),
    '#type' => 'radios',
    '#options' => array(),
    "#default_value" => $default['ingest_selector'],
  );

  $test_form_association = function ($form_name) {
    // Test that an association exists for the given form (and it is a MODS
    // form).
    module_load_include('inc', 'xml_form_builder', 'includes/associate');
    return xml_form_builder_get_associations(array($form_name), array(), array('MODS'));
  };
  foreach (lib4ridora_citation_form_subtypes() as $type => $info) {
    $form['ingest_selector']['#options'][$type] = $info['name'];
    $form['ingest_selector'][$type]['#description'] = $info['description'];
    if ($info['form'] === NULL || !$test_form_association($info['form'])) {
      $form['ingest_selector'][$type]['#disabled'] = TRUE;
      $form['ingest_selector'][$type]['#description'] .= ' ' . t('(No form associated)');
    }
  }

  // Handle using this form for editing when required, if the object is passed
  // then the form needs to add a submit button and use $form_state['object']
  // for processing.
  if ($editing) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Next'),
    );
  }
  else {
    // On ingest allow for DOI for Journal Articles.
    $form['journal_import_method'] = array(
      '#title' => t('Import Method'),
      '#type' => 'radios',
      '#options' => array(
        "manual" => t("Manually enter publication details"),
        "doi" => t("Import via DOI"),
      ),
      "#default_value" => $default['journal_import_method'],
      '#states' => array(
        'visible' => array(
          ':input[name="ingest_selector"]' => array('value' => 'journal_article'),
        ),
      ),
    );
    $form['doi'] = array(
      '#title' => t('Digital Object Identifier'),
      '#type' => 'textfield',
      '#description' => t('Enter an identifier for which to attempt to acquire metadata, such as: 10.1016/j.jorganchem.2011.11.018, 10.1016/j.tiv.2011.10.017, 10.1111/j.1540-4560.2012.01733.x, http://doi.org/10.1111/eva.12339, or http://doi.org/10.1111/eva.12340'),
      '#element_validate' => array('lib4ridora_doi_validate_id'),
      "#default_value" => $default['doi'],
      '#states' => array(
        'visible' => array(
          ':input[name="journal_import_method"]' => array('value' => 'doi'),
          ':input[name="ingest_selector"]' => array('value' => 'journal_article'),
        ),
      ),
    );
  }
  return $form;
}

/**
 * Element validation callback; ensure the submitted DOI is valid.
 *
 * If it is valid, put it in the MODS datastream so it can be loaded in the
 * next step.
 */
function lib4ridora_doi_validate_id($element, &$form_state, $form) {
  if (isset($element['#value']) && $form_state['values']['journal_import_method'] == 'doi') {
    module_load_include('inc', 'lib4ridora', 'includes/utilities');
    $mods = lib4ridora_doi_validate($element, $form_state, $form, TRUE);
    if ($mods !== FALSE) {
      $object = islandora_ingest_form_get_object($form_state);
      if (isset($object["MODS"])) {
        $datastream = $object["MODS"];
        $datastream->setContentFromString(trim($mods->saveXML()));
        if ($datastream->label != "MODS Record") {
          $datastream->label = "MODS Record";
        }
        if ($datastream->mimetype != 'text/xml') {
          $datastream->mimetype = 'text/xml';
        }
      }
      else {
        $datastream = $object->constructDatastream("MODS", 'M');
        $datastream->setContentFromString(trim($mods->saveXML()));
        $datastream->label = "MODS Record";
        $datastream->mimetype = 'text/xml';
        $object->ingestDatastream($datastream);
      }
    }
  }
}

/**
 * Change the xml form association depending on the selector value.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function lib4ridora_ingest_selector_form_submit(array $form, array &$form_state) {
  module_load_include('inc', 'xml_form_builder', 'includes/associations');
  module_load_include('inc', 'lib4ridora', 'includes/utilities');
  module_load_include('inc', 'lib4ridora', 'includes/citation.subtypes');
  $association_step_storage = &islandora_ingest_form_get_step_storage($form_state, 'xml_form_builder_association_step');
  $association_step_storage['journal_import_method'] = $form_state['values']['journal_import_method'];
  $association_step_storage['doi'] = $form_state['values']['doi'];
  $association_step_storage['ingest_selector'] = $form_state['values']['ingest_selector'];

  $subtype = $form_state['values']['ingest_selector'];
  $subtypes = lib4ridora_citation_form_subtypes();
  $form_name = $subtypes[$subtype]['form'];
  foreach (xml_form_builder_get_associations(array($form_name), array(), array('MODS')) as $key => $association) {
    // Update the content_model to be ir:citationCModel so that regardless of
    // the form used it will only have the ir:citationCModel.
    $association['content_model'] = "ir:citationCModel";
    $association_step_storage['association'] = $association;
    break;
  }
  if (!isset($association_step_storage['association'])) {
    form_set_error('ingest_selector', t('A form could not be determined for the selected publication type.'));
  }
}

/**
 * Undo's any changes the regular submit handler did to the association.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function lib4ridora_ingest_selector_form_undo_submit(array $form, array &$form_state) {
  $association_step_storage = &islandora_ingest_form_get_step_storage($form_state, 'xml_form_builder_association_step');
  unset($association_step_storage['association']);
  $object = islandora_ingest_form_get_object($form_state);
  if (isset($object["MODS"])) {
    unset($object["MODS"]);
  }
}
