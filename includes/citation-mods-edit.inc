<?php

/**
 * @file
 * Contains the callback functions for citation MODS manipulations.
 */

/**
 * Grab the correct MODS edit form for citation objects and load it.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 * @param AbstractObject $object
 *   The object to edit.
 *
 * @return array
 *   Returns the drupal form to load.
 */
function lib4ridora_citation_select_mods_edit_form($form, &$form_state, AbstractObject $object) {
  form_load_include($form_state, 'inc', 'xml_form_builder', 'includes/associations');
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/utilities');
  $form_state['pid'] = $object->id;

  $form_name = lib4ridora_determine_form_name($object);
  if (empty($form_name) && isset($form_state['form_name'])) {
    $form_name = $form_state['form_name'];
  }
  if ($form_name) {
    foreach (xml_form_builder_get_associations(array($form_name), array(), array('MODS')) as $key => $association) {
      // Update the content_model to be ir:citationCModel so that regardless
      // of the form used it will only have the ir:citationCModel.
      $association['content_model'] = "ir:citationCModel";
      // Load the form builder form.
      $metadata_form = xml_form_builder_get_form(array(), $form_state, $association['form_name'], $object['MODS']->content, array());
      // Need to make the #type markup because forms within forms are bad.
      $metadata_form['#type'] = 'markup';
      $metadata_form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Update'),
      );
      $form_state['association'] = $association;
      return $metadata_form;
    }
    drupal_set_message(t('The XML form could not be loaded. Verify that the %form_name form has an association.', array(
      '%form_name' => $form_name,
    )), 'error');
  }

  // Fallback to load the ingest form if there is no current association so that
  // an association can be set.
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/ingest-selector.form');
    foreach (lib4ridora_citation_form_subtypes() as $type => $info) {
      module_load_include('inc', 'lib4ridora', 'includes/citation.subtypes');
      $subtypes = lib4ridora_determine_subtypes($object);
      $subtypes = reset($subtypes);
      if (in_array($subtypes['name'], $info['genres'])) {
        foreach (xml_form_builder_get_associations(array($info['form']), array(), array('MODS')) as $key => $association) {
          // Update the content_model to be ir:citationCModel so that regardless
          // of the form used it will only have the ir:citationCModel.
          $association['content_model'] = "ir:citationCModel";
          // Load the form builder form.
          $metadata_form = xml_form_builder_get_form(array(), $form_state, $association['form_name'], $object['MODS']->content, array());
          // Need to make the #type markup because forms within forms are bad.
          $metadata_form['#type'] = 'markup';
          $metadata_form['submit'] = array(
            '#type' => 'submit',
            '#value' => t('Update'),
          );
          $form_state['association'] = $association;
          return $metadata_form;
        }
      }
      else {
        $form = lib4ridora_ingest_selector_form($form, $form_state, $object);
      }
    }
  return $form;
}

/**
 * Handle the submit for the ingest-selector and the loaded form builder form.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function lib4ridora_citation_select_mods_edit_form_submit($form, &$form_state) {
  if (isset($form_state['values']['ingest_selector'])) {
    $subtype = $form_state['values']['ingest_selector'];
    $subtypes = lib4ridora_citation_subtypes();
    $form_state['form_name'] = $subtypes[$subtype]['form'];
    $form_state['rebuild'] = TRUE;
  }
  else {
    $pid = $form_state['pid'];
    $object = islandora_object_load($pid);
    // Handle all formbuilder things.
    $xml_form = new XMLForm($form_state, array());
    // Pass only the portion of the form that contains the formbuilder markup
    // and spoof the form_state['values'] so formbuilder operates as normal.
    $document = $xml_form->submit($form, $form_state);
    xml_form_builder_update_object($object, $form_state['association'], $document);
    $form_state['redirect'] = "islandora/object/{$object->id}/manage";
  }
}
