<?php
/**
 * @file
 * Helper functions to deal with ir:citationCModel subtypes.
 */

/**
 * Get information about the different display subtypes.
 *
 * @return array
 *   An associative array mapping a stable machine name to an associative array
 *   containing:
 *   - name: A human-readable name for the given type.
 *   - description: A verbose description of the given type.
 *   - genres: An array of genres which map to the given type.
 *   - form: The name of the form to use for the given type. NOTE: May be NULL
 *     if there is none mapped for the given type.
 *   - display: The machine name of the Solr metadata display to use for the
 *     given type. NOTE: May be NULL if there is none mapped for the given
 *     type.
 */
function lib4ridora_citation_subtypes() {
  static $map = NULL;
  if ($map === NULL) {
    $map = array(
      'journal_article' => array(
        'name' => t('Journal Article'),
        'description' => t('An article in a scientific journal.  Not necessarily peer-reviewed. May be an electronic-only medium such as online journal.'),
      ),
      'book_chapter' => array(
        'name' => t('Book Chapter'),
        'description' => t('A chapter or contribution in a book.'),
      ),
      'book' => array(
        'name' => t('Book'),
        'description' => t('A monograph, edited volume or textbook.'),
      ),
      'edited_book' => array(
        'name' => t('Edited Book'),
        'description' => t('An edited book.'),
      ),
      'newspaper_or_magazine_article' => array(
        'name' => t('Newspaper or Magazine Article'),
        'description' => t('An article in a newspaper or magazine. Not peer-reviewed.'),
      ),
      'conference_object' => array(
        'name' => t('Conference Item'),
        'description' => t('A paper, poster or presentation given at a conference. If the conference item has been published in a journal or book please use "Journal Article" or "Book Chapter" instead.'),
      ),
      'proceedings_paper' => array(
        'name' => t('Proceedings Paper'),
        'description' => t('A proceedings paper.'),
      ),
      'master_thesis' => array(
        'name' => t('Master Thesis'),
        'description' => t('A master thesis.'),
      ),
      'bachelor_thesis' => array(
        'name' => t('Bachelor Thesis'),
        'description' => t('A bachelor thesis.'),
      ),
      'dissertation' => array(
        'name' => t('Dissertation'),
        'description' => t('A dissertation.'),
      ),
      'patent' => array(
        'name' => t('Patent'),
        'description' => t('A published patent.  Do not include as yet unpublished patent applications.'),
      ),
      'report' => array(
        'name' => t('Report'),
        'description' => t('A report or expert opinion.'),
      ),
    );

    // Map MODS genres to our machine names...
    $genre_map = array(
      'Journal Article' => 'journal_article',
      'Book Chapter' => 'book_chapter',
      'Book' => 'book',
      'Edited Book' => 'edited_book',
      'Newspaper or Magazine Article' => 'newspaper_or_magazine_article',
      'Conference Item' => 'conference_object',
      'Proceedings Paper' => 'proceedings_paper',
      'Report' => 'report',
      'Thesis' => 'thesis',
      'Dissertation' => 'dissertation',
      'Master Thesis' => 'master_thesis',
      'Bachelor Thesis' => 'bachelor_thesis',
      'Patent' => 'patent',
    );
    $form_map = variable_get('lib4ridora_citation_subtype_form_associations', array());
    $display_map = variable_get('lib4ridora_islandora_solr_metadata_mappings', array());
    $test_form_association = function ($form_name) {
      // Test that an association exists for the given form (and it is a MODS
      // form).
      module_load_include('inc', 'xml_form_builder', 'includes/associations');
      return xml_form_builder_get_associations(array($form_name), array(), array('MODS'));
    };
    foreach ($map as $name => &$info) {
      $info['genres'] = array_keys($genre_map, $name);
      $info['display'] = isset($display_map[$name]) ? $display_map[$name] : NULL;

      // Use the previously configured form name; otherwise, default to the form
      // with the same human-readable name.
      $info['form'] = isset($form_map[$name]) ? $form_map[$name] : $info['name'];
      if (!$test_form_association($info['form'])) {
        // Association for the form does not exist: Wipe it.
        $info['form'] = NULL;
      }
    }
    // XXX: Deal with leaked reference.
    unset($info);
  }

  return $map;
}

/**
 * Get information about the different form subtypes.
 *
 * @return array
 *   An associative array mapping a stable machine name to an associative array
 *   containing:
 *   - name: A human-readable name for the given type.
 *   - description: A verbose description of the given type.
 *   - genres: An array of genres which map to the given type.
 *   - form: The name of the form to use for the given type. NOTE: May be NULL
 *     if there is none mapped for the given type.
 *   - display: The machine name of the Solr metadata display to use for the
 *     given type. NOTE: May be NULL if there is none mapped for the given
 *     type.
 */
function lib4ridora_citation_form_subtypes() {
  static $map = NULL;
  if ($map === NULL) {
    $map = array(
      'journal_article' => array(
        'name' => t('Journal Article'),
        'description' => t('An article in a scientific journal.  Not necessarily peer-reviewed. May be an electronic-only medium such as online journal.'),
      ),
      'book_chapter' => array(
        'name' => t('Book Chapter'),
        'description' => t('A chapter or contribution in a book.'),
      ),
      'book' => array(
        'name' => t('Book'),
        'description' => t('A monograph, edited volume or textbook.'),
      ),
      'newspaper_or_magazine_article' => array(
        'name' => t('Newspaper or Magazine Article'),
        'description' => t('An article in a newspaper or magazine. Not peer-reviewed.'),
      ),
      'conference_object' => array(
        'name' => t('Conference Item'),
        'description' => t('A paper, poster or presentation given at a conference. If the conference item has been published in a journal or book please use "Journal Article" or "Book Chapter" instead.'),
      ),
      'thesis' => array(
        'name' => t('Thesis'),
        'description' => t('A thesis or dissertation.'),
      ),
      'patent' => array(
        'name' => t('Patent'),
        'description' => t('A published patent.  Do not include as yet unpublished patent applications.'),
      ),
      'report' => array(
        'name' => t('Report'),
        'description' => t('A report or expert opinion.'),
      ),
    );

    // Map MODS genres to our machine names...
    $genre_map = array(
      'Journal Article' => 'journal_article',
      'Book Chapter' => 'book_chapter',
      'Book' => 'book',
      'Edited Book' => 'book',
      'Newspaper or Magazine Article' => 'newspaper_or_magazine_article',
      'Conference Item' => 'conference_object',
      'Proceedings Paper' => 'conference_object',
      'Report' => 'report',
      'Thesis' => 'thesis',
      'dissertation' => 'thesis',
      'master thesis' => 'thesis',
      'bachelor thesis' => 'thesis',
      'Patent' => 'patent',
    );
    $form_map = variable_get('lib4ridora_citation_subtype_form_associations', array());
    $display_map = variable_get('lib4ridora_islandora_solr_metadata_mappings', array());
    $test_form_association = function ($form_name) {
      // Test that an association exists for the given form (and it is a MODS
      // form).
      module_load_include('inc', 'xml_form_builder', 'includes/associations');
      return xml_form_builder_get_associations(array($form_name), array(), array('MODS'));
    };
    foreach ($map as $name => &$info) {
      $info['genres'] = array_keys($genre_map, $name);
      $info['display'] = isset($display_map[$name]) ? $display_map[$name] : NULL;

      // Use the previously configured form name; otherwise, default to the form
      // with the same human-readable name.
      $info['form'] = isset($form_map[$name]) ? $form_map[$name] : $info['name'];
      if (!$test_form_association($info['form'])) {
        // Association for the form does not exist: Wipe it.
        $info['form'] = NULL;
      }
    }
    // XXX: Deal with leaked reference.
    unset($info);
  }

  return $map;
}

/**
 * Helper function to list Solr metadata options.
 */
function lib4ridora_citation_subtype_islandora_solr_metadata_options() {
  module_load_include('inc', 'islandora_solr_metadata', 'includes/db');
  $raw_associations = islandora_solr_metadata_get_associations();
  $associations = array();
  foreach ($raw_associations as $association) {
    $associations[$association['machine_name']] = $association['name'];
  }
  return $associations;
}

/**
 * Helper function to obtain eligible forms.
 *
 * @return array
 *   An associative array wherein keys are equal to their related values, which
 *   represent form names from XML forms.
 */
function lib4ridora_citation_subtype_form_options() {
  module_load_include('inc', 'xml_form_builder', 'includes/associations');
  $forms = array();
  foreach (xml_form_builder_get_associations(array(), array(), array('MODS')) as $id => $association) {
    $forms[$association['form_name']] = $association['form_name'];
  }
  return $forms;
}

/**
 * Form building function; permit the selection of mappings.
 */
function lib4ridora_citation_subtype_mapping_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/utilities');
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/citation.subtypes');

  $form['#tree'] = TRUE;
  $form['subtype_mappings'] = array(
    '#type' => 'vertical_tabs',
  );

  $subtypes = lib4ridora_citation_subtypes();
  $form_subtypes = lib4ridora_citation_form_subtypes();
  $map_for_forms = function ($subtype) {
    return $subtype['name'];
  };
  $variables = array(
    'lib4ridora_islandora_solr_metadata_mappings' => array(
      'options' => lib4ridora_citation_subtype_islandora_solr_metadata_options(),
      'strings' => array(
        'title' => t('Metadata Display'),
        'description' => t('Instead of using content models to choose which metadata display should be used, the mapping on this page can be used.'),
        'mapping_title' => t('Mapping for @type'),
      ),
    ),
    'lib4ridora_citation_subtype_form_associations' => array(
      'options' => lib4ridora_citation_subtype_form_options(),
      'strings' => array(
        'title' => t('XML Form'),
        'description' => t('Instead of using content models to choose which XML form should be used, the mapping on this page can be used.'),
        'mapping_title' => t('Mapping for @type'),
      ),
      // Matches present defaults with explicitly matching form names.
      'defaults' => array_map($map_for_forms, $form_subtypes),
    ),
  );
  foreach ($variables as $variable => $info) {
    $current_value = variable_get($variable, array());
    $defaults = isset($info['defaults']) ? $info['defaults'] : array();

    $base_select = array(
      '#type' => 'select',
      '#options' => array(
        NULL => t('-- Default/None --'),
      ) + $info['options'],
    );

    $form['settings'][$variable] = array(
      '#type' => 'fieldset',
      '#group' => 'subtype_mappings',
      '#title' => filter_xss($info['strings']['title']),
      '#description' => filter_xss($info['strings']['description']),
    );
    if ($variable == 'lib4ridora_islandora_solr_metadata_mappings') {
      foreach ($subtypes as $type => $type_info) {
        $form['settings'][$variable][$type] = $base_select + array(
          '#title' => filter_xss(format_string($info['strings']['mapping_title'], array(
            '@type' => $type_info['name'],
          ))),
          '#description' => filter_xss($type_info['description']),
          // Current value, defined default, or NULL.
          '#default_value' => ((isset($current_value[$type]) && isset($info['options'][$current_value[$type]])) ?
            $current_value[$type] :
            ((isset($defaults[$type]) && isset($info['options'][$defaults[$type]])) ?
              $defaults[$type] :
              NULL)),
        );
      }
    }
    else {
      foreach ($form_subtypes as $type => $type_info) {
        $form['settings'][$variable][$type] = $base_select + array(
          '#title' => filter_xss(format_string($info['strings']['mapping_title'], array(
            '@type' => $type_info['name'],
          ))),
          '#description' => filter_xss($type_info['description']),
          // Current value, defined default, or NULL.
          '#default_value' => ((isset($current_value[$type]) && isset($info['options'][$current_value[$type]])) ?
            $current_value[$type] :
            ((isset($defaults[$type]) && isset($info['options'][$defaults[$type]])) ?
              $defaults[$type] :
              NULL)),
        );
      }
    }

  }

  $form['actions'] = array(
    '#type' => 'actions',
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Submit mappings'),
    ),
  );

  return $form;
}

/**
 * Form submission handler; persist the mapping selection.
 */
function lib4ridora_citation_subtype_mapping_form_submit(&$form, &$form_state) {
  foreach ($form_state['values']['settings'] as $variable => $values) {
    variable_set($variable, $values);
  }
  drupal_set_message(t('Saved mappings.'));
}

/**
 * Helper to get the subtype(s) of a given object.
 *
 * @param AbstractObject $object
 *   The object to test.
 *
 * @return array
 *   An associative array similar to lib4ridora_citation_subtypes().
 */
function lib4ridora_determine_subtypes(AbstractObject $object) {
  if (!isset($object['MODS'])) {
    watchdog('lib4ridora', 'Attempted to determine the subtype of an object without MODS: @object_id', array(
      '@object_id' => $object->id,
    ));
    return;
  }
  $types = lib4ridora_citation_subtypes();

  // Read Genre from MODS to determine what form to use for editing.
  $mods = new DOMDocument();
  $mods->loadXML($object['MODS']->content);
  $xpath = new DOMXPath($mods);
  $xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');

  $subtypes = array();
  foreach ($xpath->query('//mods:mods/mods:genre[@authorityURI="info:eu-repo/semantics"]') as $genre) {
    $genre_text = $genre->nodeValue;
    $test_match = function ($genres) use ($genre_text) {
      return in_array($genre_text, $genres);
    };
    $subtypes += array_filter($types, $test_match);
  }
  return $subtypes;
}
