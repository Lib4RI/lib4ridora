<?php

/**
 * @file
 * Ingest step form and friends.
 */

// @XXX silencing and including like legacy tuque wrapper implementation.
@include_once 'sites/all/libraries/tuque/FedoraRelationships.php';
define('LIB4RIDORA_RELS_URI', ISLANDORA_RELS_INT_URI);

/**
 * Helper function to get the object during the form process.
 *
 * Gets either the object from ingest steps, or assumed the first parameter
 * passed to the form is an object, which we'll reload.
 */
function lib4ridora_multi_embargo_get_object($form_state) {
  list($object) = $form_state['build_info']['args'];
  if (!($object instanceof AbstractObject)) {
    module_load_include('inc', 'islandora', 'includes/ingest.form');
    $object = islandora_ingest_form_get_object($form_state);
  }
  else {
    // Reload, 'cause paranoia.
    $object = islandora_object_load($object->id);
  }

  return $object;
}

/**
 * Filter callback function.
 *
 * Matches datastreams we should deal with in this form... Both the "PDF"
 * and those starting with the "lib4ridora_extra_pdf_datastream_prefix" Drupal
 * variable.
 *
 * @param AbstractDatastream $datastream
 *   A datastream to test.
 *
 * @return bool
 *   TRUE if the datastream ID is "PDF" or starts with the string specified in
 *   the "lib4ridora_extra_pdf_datastream_prefix" Drupal variable; otherwise,
 *   FALSE.
 */
function lib4ridora_multi_embargo_pdf_filter_soft(AbstractDatastream $datastream) {		// original function, with '_all' attached onto its name.
  return $datastream->id == 'PDF' || strpos($datastream->id, variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF')) === 0;
}

function lib4ridora_multi_embargo_pdf_filter(AbstractDatastream $datastream) {	// modified version of the original function, now also excluding DS ending on 'PDF-A'. Mime-type check still skipped.
  $dsId = strval( $datastream->id );
  if ( $dsId == 'PDF' || strpos($dsId, variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF')) === 0 ) {
    return ( substr($dsId,-5) != "PDF-A" );
  }
  return FALSE;
}

function lib4ridora_multi_embargo_pdf_filter_hard(AbstractDatastream $datastream) {	// modified version of the original function, now also excluding DS ending on 'PDF-A'. Mime-type check still skipped.
  $dsId = strval( $datastream->id );
  if ( $dsId == 'PDF' || strpos($dsId, variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF')) === 0 ) {
    if ( substr($dsId,-5) != "PDF-A" ) { return TRUE; }
    // skipping PDF/A file if its availability is set to private
    $infoAry = lib4ridora_get_embargo_info($datastream);
    return ( @!empty($infoAry['availability']) && strtolower($infoAry['availability']) != strtolower(variable_get('lib4ri_pdfa_availability','private')) );
  }
  return FALSE;
}

/*
 * function to define the label of the PDF/A datastreams
 */
function lib4ridora_get_pdfa_label( $parent_filename, $parent_dsid = '', $is_final = TRUE ) {
  if ( $is_final ) {
    $pos = strrpos($parent_filename,".");
      return ( substr($parent_filename,0,$pos+1) . "PDF-A" . substr($parent_filename,$pos) );
  }
  return ( "PDF/A of " . ( $parent_dsid != '' ? $parent_dsid." " : "" ) . "'" . $parent_filename . "'" );
}

/**
 * Form building function; build multi-PDF embargo form.
 */
function lib4ridora_multi_embargo_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/embargo.form');
  module_load_include('inc', 'islandora', 'includes/ingest.form');
  $step_storage = islandora_ingest_form_get_step_storage($form_state, 'lib4ridora_multi_pdf_upload');

  $form['#tree'] = TRUE;
  $form['files'] = array(
    '#type' => 'markup',
  );
  lib4ridora_append_document_information($step_storage['files'], $form);
  lib4ridora_append_romeo_information($form, $form_state);
  return $form;
}

/**
 * Form for editing multi-PDF embargoes.
 *
 * @param array $form
 *   An array representing a form within Drupal.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array representing the form to be rendered.
 */
function lib4ridora_multi_embargo_edit_form($form, &$form_state) {
  $form['#tree'] = TRUE;
  $form['files'] = array(
    '#type' => 'markup',
  );
  $object = lib4ridora_multi_embargo_get_object($form_state);
  $datastreams = iterator_to_array($object);
  $pdf_datastreams = array_filter($datastreams, 'lib4ridora_multi_embargo_pdf_filter');
  lib4ridora_append_document_information($pdf_datastreams, $form);
  lib4ridora_append_romeo_information($form, $form_state);
  $form['actions'] = array(
    '#type' => 'actions',
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Update properties'),
    ),
  );
  return $form;
}


/**
 * Get info about the datastream.
 *
 * @return array
 *   An associative array containing:
 *   - availability: Indicate if/how the datastream is available.
 *   - embargo_date: Array containing the 'year', 'month' and 'day'.
 *   - document_version
 *   - use_permission
 */
function lib4ridora_get_embargo_info(AbstractDatastream $datastream) {
  // Add in defaults.
  $to_return = lib4ridora_multi_pdf_defaults_properties();

  foreach ($to_return as $pred => $default) {
    $value = $datastream->relationships->get(LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate($pred));
    // Should be at most one value... Just get it.
    $value = reset($value);
    if ($value) {
      if ($pred == 'embargo_date') {
        $to_return[$pred] = array_intersect_key(date_parse($value['object']['value']), drupal_map_assoc(array(
          'year',
          'month',
          'day',
        )));
      }
      else {
        $to_return[$pred] = $value['object']['value'];
      }
    }
  }
  return $to_return;
}

/**
 * Form submission handler; apply multi-PDF embargoes and properties.
 */
function lib4ridora_multi_embargo_form_submit(&$form, &$form_state) {
  module_load_include('inc', 'islandora', 'includes/ingest.form');

  $step_storage = islandora_ingest_form_get_step_storage($form_state, 'lib4ridora_multi_pdf_upload');
  $object = lib4ridora_multi_embargo_get_object($form_state);
  $primary_file = lib4ridora_get_primary_file_for_datastream($form_state);
  $file_info = $step_storage['files'];
  // The files are keyed in the values by uuid for fieldset uniqueness for
  // states, let's construct this mapping now to get back to the files.
  $mapped_files = array();
  foreach ($file_info as $file) {
    $mapped_files[$file->uuid] = $file;
  }

	$pdf_nr_suffix = 2;
	foreach ($form_state['values']['files'] as $filename => $values) {
		if ( substr($filename,-5) == "PDF-A" ) {
			continue;
		}
		// Construct the datastreams.
		$dsid_new = "PDF";		// = variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF');
		if ( $filename != $primary_file ) {
			$dsid_new = format_string('@prefix@index', array(
				'@prefix' => variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF'),
				'@index' => $pdf_nr_suffix,
			));
			$pdf_nr_suffix++;
		}
		$dsid_pdfa = $filename . "_" . "PDF-A";		// old/current name of PDF/A darastream
		if ( @isset($mapped_files[$dsid_pdfa]) ) {
			$dsid_new_pdfa = $dsid_new . "_" . "PDF-A";		// new datastream name
			$datastream = $object->constructDatastream($dsid_new_pdfa, 'M');
			$datastream->mimetype = $mapped_files[$dsid_pdfa]->filemime;
			$datastream->label = lib4ridora_get_pdfa_label( $mapped_files[$filename]->filename, $dsid_new );		// by default $mapped_files[$filename]->filename is the original PDF filename
			$datastream->setContentFromFile($mapped_files[$dsid_pdfa]->uri, FALSE);
			$object->ingestDatastream($datastream);
			$values_pdfa = $values; 	// $values is an array with the following indices: document_version | availability | embargo_date | use_permission
			if ( $avail_custom = variable_get('lib4ri_pdfa_availability','private') ) {
				if ( strlen($avail_custom) > 1 ) { $values_pdfa['availability'] = $avail_custom; }
			}
			lib4ridora_update_datastream_relationships($object[$dsid_new_pdfa], $values_pdfa);
		}
		$datastream = $object->constructDatastream($dsid_new, 'M');
		$datastream->mimetype = $mapped_files[$filename]->filemime;
		$datastream->label = $mapped_files[$filename]->filename;
		$datastream->setContentFromFile($mapped_files[$filename]->uri, FALSE);
		$object->ingestDatastream($datastream);
		lib4ridora_update_datastream_relationships($object[$dsid_new], $values);
	}
}

/**
 * Form "undo" submission handler; reverse application of multi-PDF embargoes.
 *
 * Also, get rid of some associated properties.
 */
function lib4ridora_multi_embargo_form_undo_submit(&$form, &$form_state) {
  $object = lib4ridora_multi_embargo_get_object($form_state);
  $datastreams = iterator_to_array($object);
  $pdf_datastreams = array_filter($datastreams, 'lib4ridora_multi_embargo_pdf_filter');
  foreach ($pdf_datastreams as $dsid => $ds) {
    foreach (lib4ridora_multi_pdf_defaults_properties() as $pred => $default) {
      $ds->relationships->remove(LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate($pred));
    }
    $object->purgeDatastream($dsid);
  }
}

/**
 * Submit handler for the edit multi-PDF form.
 *
 * @param array $form
 *   An array representing a form within Drupal.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */

function lib4ridora_multi_embargo_edit_form_submit(&$form, &$form_state) {

	$object = lib4ridora_multi_embargo_get_object($form_state);
	$primary_file = lib4ridora_get_primary_file_for_datastream($form_state);
	if ( $primary_file == 'PDF' ) {
		// just update the RELS-INT, and copy over the document version to the PDF/A
		foreach ($form_state['values']['files'] as $dsid => $values) {
			lib4ridora_update_datastream_relationships($object[$dsid], $values);
			if ( substr($dsid,-5) == "PDF-A" ) { continue; }
			$dsid_pdfa = $dsid . "_" . "PDF-A";
			if ( @isset($object[$dsid_pdfa]) ) {
				$values_pdfa = $values; 	// $values is an array with the following indices: document_version | availability | embargo_date | use_permission
				if ( $avail_custom = variable_get('lib4ri_pdfa_availability','private') ) {
					if ( strlen($avail_custom) > 1 ) { $values_pdfa['availability'] = $avail_custom; }
				}
				lib4ridora_update_datastream_relationships($object[$dsid_pdfa], $values_pdfa);
			}
		}
		return;
	}

	$mapped_files = array();
	$datastreams = iterator_to_array($object);
	$pid = $object->id;
	$pdf_datastreams = array_filter($datastreams, 'lib4ridora_multi_embargo_pdf_filter');

	foreach ($pdf_datastreams as $dsid => $ds) {
		$content_uri = drupal_tempnam('temporary://', 'lib4ripdf');
		$ds->getContent($content_uri);
		$mapped_files[$dsid] = array(
			'label' => $ds->label,
			'mime' => $ds->mimetype,
			'content' => $content_uri,
		);
		// Doing this because of an inherent Tuque RELS-INT bug.
		$tuque = islandora_get_tuque_connection();
		$tuque->cache->delete($pid);
		$non_cached_object = islandora_object_load($pid);
		foreach (lib4ridora_multi_pdf_defaults_properties() as $pred => $default) {
			$non_cached_object[$dsid]->relationships->remove(LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate($pred));
		}
		$non_cached_object->purgeDatastream($dsid);
	}
	$deriv_datastreams_to_purge = array(
		'TN',
		'FULL_TEXT',
		'PREVIEW',
	);
	foreach ($deriv_datastreams_to_purge as $purge_dsid) {
		if (isset($non_cached_object[$purge_dsid])) {
			$non_cached_object->purgeDatastream($purge_dsid);
		}
	}

	$pdf_nr_suffix = 2;
	foreach ($form_state['values']['files'] as $filename => $values) {		// $filename is the DS Id
		if ( substr($filename,-5) == "PDF-A" ) {
			continue;
		}
		$dsid_new = "PDF";		// = variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF');
		if ( $filename != $primary_file ) {
			$dsid_new = format_string('@prefix@index', array(
				'@prefix' => variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF'),
				'@index' => $pdf_nr_suffix,
			));
			$pdf_nr_suffix++;
		}
		// Construct the datastreams.
		$dsid_pdfa = $filename . "_" . "PDF-A";		// old/current name of PDF/A darastream
		if ( @isset($mapped_files[$dsid_pdfa]) ) {
			$dsid_new_pdfa = $dsid_new . "_" . "PDF-A";		// new datastream name
			$datastream = $non_cached_object->constructDatastream($dsid_new_pdfa, 'M');
			$datastream->mimetype = $mapped_files[$dsid_pdfa]['mime'];
			$datastream->label = lib4ridora_get_pdfa_label( $mapped_files[$filename]['label'], $dsid_new );		// by default $mapped_files[$filename]['label'] is the original PDF filename
			$datastream->setContentFromFile($mapped_files[$dsid_pdfa]['content'], FALSE);
			$non_cached_object->ingestDatastream($datastream);
			file_unmanaged_delete($mapped_files[$dsid_pdfa]['content']);
			$values_pdfa = $values; 	// $values is an array with the following indices: document_version | availability | embargo_date | use_permission
			if ( $avail_custom = variable_get('lib4ri_pdfa_availability','private') ) {
				if ( strlen($avail_custom) > 1 ) { $values_pdfa['availability'] = $avail_custom; }
			}
			lib4ridora_update_datastream_relationships($non_cached_object[$dsid_new_pdfa], $values_pdfa);
		}
		$datastream = $non_cached_object->constructDatastream($dsid_new, 'M');
		$datastream->mimetype = $mapped_files[$filename]['mime'];
		$datastream->label = $mapped_files[$filename]['label'];
		$datastream->setContentFromFile($mapped_files[$filename]['content'], FALSE);
		$non_cached_object->ingestDatastream($datastream);
		file_unmanaged_delete($mapped_files[$filename]['content']);
		lib4ridora_update_datastream_relationships($non_cached_object[$dsid_new], $values);
	}
}


/**
 * Set a relationship.
 *
 * Add a relationship such that it is the only relationship with the given
 * predicate for the given subject.
 */
function lib4ridora_set_relationships(FedoraRelationships $rels, $pred_uri, $pred, $object, $object_type = RELS_TYPE_URI) {
  $rels->remove($pred_uri, $pred);
  $rels->add($pred_uri, $pred, $object, $object_type);
}

/**
 * Helper function to build up RDF predicates.
 *
 * @param string $base
 *   The unique part of the predicate.
 *
 * @return string
 *   A predicate which should be unique for our use.
 */
function lib4ridora_multi_embargo_build_predicate($base) {
  return "lib4ridora-multi-embargo-$base";
}

/**
 * Helper function, to enumerate the defaults.
 *
 * @return array
 *   An array of defaults for multi-pdf settings.
 */
function lib4ridora_multi_pdf_defaults_properties() {
  $date = getdate();
  $embargo_date = array(
    'year' => $date['year'],
    'month' => $date['month'],
    'day' => $date['mday'],
  );
  return array(
    'availability' => variable_get('lib4ridora_multi_pdf_defaults_property_availability', 'intranet'),
    'embargo_date' => $embargo_date,
    'document_version' => 'published version',
    'use_permission' => NULL,
  );
}

/**
 * Helper function; mappings of "codes" to human-readable strings.
 *
 * @param bool $sort
 *   Whether the document versions are to be weighted per LIB4RI's requirements.
 *
 * @return array
 *   An array containing the options used in the form.
 */
function lib4ridora_multi_pdf_get_options($sort = TRUE) {
  if ($sort) {
    $document_versions = array(
      'published version' => t('Published Version'),
      'accepted version' => t('Accepted Version'),
 /*   'updated version' => t('Updated Version'),		<== not needed anymore, see PDF_Fix_Order.pdf ( http://lib-dora-dev1.emp-eaw.ch:3000/issues/79 ) */
      'erratum' => t('Erratum'),
      'supplemental material' => t('Supplemental Material'),
      'unspecified' => t('Unspecified PDF'),
    );
  }
  else {
    $document_versions = array(
      'accepted version' => t('Accepted Version'),
      'published version' => t('Published Version'),
 /*   'updated version' => t('Updated Version'),		<== not needed anymore, see PDF_Fix_Order.pdf ( http://lib-dora-dev1.emp-eaw.ch:3000/issues/79 ) */
      'erratum' => t('Erratum'),
      'supplemental material' => t('Supplemental Material'),
      'unspecified' => t('Unspecified PDF'),
    );
  }
  return array(
    'availability' => array(
      'public' => t('Public'),
      'intranet' => t('Intranet'),
      'date' => t('Embargo'),
      'private' => t('Private'),
    ),
    'document_version' => $document_versions,
    'use_permission' => array(
      NULL => t('Unspecified'),
      'CC BY' => t('Creative Commons: Attribution'),
      'CC BY-SA' => t('Creative Commons: Attribution-ShareAlike'),
      'CC BY-ND' => t('Creative Commons: Attribution-NoDerivs'),
      'CC BY-NC' => t('Creative Commons: Attribution-NonCommercial'),
      'CC BY-NC-SA' => t('Creative Commons: Attribution-NonCommercial-ShareAlike'),
      'CC BY-NC-ND' => t('Creative Commons: Attribution-NonCommercial-NoDerivs'),
    ),
  );
}

/**
 * Appends document information to the forms.
 *
 * @param array $files
 *   An array of files or datastream ids and their values to be used.
 * @param array $form
 *   An array representing a form within Drupal.
 *
 * @return array
 *   The array to be rendered within Drupal.
 */
function lib4ridora_append_document_information($files, &$form) {
  // See if we are ingesting or editing.
  $type_file = reset($files);
  if ($type_file instanceof IslandoraFedoraDatastream) {
    $edit = TRUE;
  }
  else {
    $edit = FALSE;
  }
  $options = lib4ridora_multi_pdf_get_options();
  foreach ($files as $file) {
    $defaults = $edit ? lib4ridora_get_embargo_info($file) : lib4ridora_multi_pdf_defaults_properties();
    $uuid = $edit ? $file->id : $file->uuid;
    $filename = $edit ? $file->label : $file->filename;
    $form['files'][$uuid] = array(
      '#type' => 'fieldset',
      '#title' => t('@filename', array(
        '@filename' => $filename,
      )),
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
            'fieldset#edit-files-' . strtolower($uuid) . ' select.lib4ridora-embargo-select' => array(
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
  }
  return $form;
}

/**
 * Append RoMeO information to the form.
 *
 * @param array $form
 *   An array representing a form within Drupal.
 *
 * @return array
 *   An array representing the form to be rendered.
 */
function lib4ridora_append_romeo_information(&$form, &$form_state) {
  $object = lib4ridora_multi_embargo_get_object($form_state);
  // SHERPA RoMEO.
  if (isset($object['MODS'])) {
    $mods_doc = new DOMDocument();
    $mods_doc->loadXML($object['MODS']->content);
    $mods_xpath = new DOMXPath($mods_doc);
    $mods_xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
    $issn_results = $mods_xpath->query('//mods:identifier[@type="issn" and normalize-space(text())]');
    if ($issn_results->length) {
      $issn = $issn_results->item(0)->textContent;
      if ($issn) {
        $romeo_results = drupal_http_request(
          url(
            variable_get('islandora_scholar_romeo_url', 'http://www.sherpa.ac.uk/romeo/api29.php'),
            array(
              'query' => array(
                'ak' => variable_get('islandora_scholar_romeo_key', ''),
                'issn' => $issn,
              ),
            )
          )
        );
      }
    }
  }
  if (isset($romeo_results)) {
    $romeo_doc = new DOMDocument();
    $romeo_doc->loadXML($romeo_results->data);
    $romeo_xpath = new DOMXPath($romeo_doc);
    $outcome_results = $romeo_xpath->query('/romeoapi/header/outcome');
    if ($outcome_results->length && $outcome_results->item(0)->nodeValue != 'failed') {
      $romeo_text_mappings = array(
        'can' => array(
          'pre' => t('Author can archive pre-print (i.e. pre-refereeing)'),
          'post' => t('Author can archive post-print (i.e. final draft post-refereeing)'),
          'version' => t("Author can archive publisher's version/PDF"),
        ),
        'cannot' => array(
          'pre' => t('Author cannot archive pre-print (i.e. pre-refereeing)'),
          'post' => t('Author cannot archive post-print (i.e. final draft post-refereeing)'),
          'version' => t("Author cannot archive publisher's version/PDF"),
        ),
        'restricted' => array(
          'pre' => t('Subject to restrictions, author can archive pre-print (i.e. pre-refereeing)'),
          'post' => t('Subject to restrictions, author can archive post-print (i.e. final draft post-refereeing)'),
          'version' => t("Subject to restrictions, author can archive publisher's version/PDF"),
        ),
        'unclear' => array(
          'pre' => t('Archiving status unclear'),
          'post' => t('Archiving status unclear'),
          'version' => t('Archiving status unclear'),
        ),
        'unknown' => array(
          'pre' => t('No information'),
          'post' => t('No information'),
          'version' => t('No information'),
        ),
      );
      // Everything always populated.
      $pre_status = $romeo_xpath->query('/romeoapi/publishers/publisher/preprints/prearchiving')->item(0)->nodeValue;
      $post_status = $romeo_xpath->query('/romeoapi/publishers/publisher/postprints/postarchiving')->item(0)->nodeValue;
      $version_status = $romeo_xpath->query('/romeoapi/publishers/publisher/pdfversion/pdfarchiving')->item(0)->nodeValue;
      $journal_title = $romeo_xpath->query('/romeoapi/journals/journal/jtitle')->item(0)->nodeValue;
      $publisher = $romeo_xpath->query('/romeoapi/publishers/publisher/name')->item(0)->nodeValue;

      $form['romeo_info'] = array(
        'romeo_info' => array(
          '#type' => 'item',
          'definition_list' => array(
            '#prefix' => '<dl>',
            'journal' => array(
              '#markup' => t(
                "<dt>Journal:</dt><dd>!journal_title</dd>",
                array('!journal_title' => $journal_title)
              ),
            ),
            'publisher' => array(
              '#markup' => t(
                "<dt>Publisher:</dt><dd>!publisher</dd>",
                array('!publisher' => $publisher)
              ),
            ),
            "pre_print" => array(
              '#markup' => t(
                "<dt>Author's Pre-print:</dt><dd>!romeo</dd>",
                array('!romeo' => $romeo_text_mappings[$pre_status]['pre'])
              ),
            ),
            "post_print" => array(
              '#markup' => t(
                "<dt>Author's Post-print:</dt><dd>!romeo</dd>",
                array('!romeo' => $romeo_text_mappings[$post_status]['post'])
              ),
            ),
            "version" => array(
              '#markup' => t(
                "<dt>Publisher's Version/PDF:</dt><dd>!romeo</dd>",
                array('!romeo' => $romeo_text_mappings[$version_status]['version'])
              ),
            ),
            '#suffix' => '</dl>',
          ),
        ),
        'details' => array(
          '#type' => 'item',
          '#markup' => t(
            "Further details about your publisher's policy on depositing a
              version of your article are available at !link",
            array(
              '!link' => l(
                t('SHERPA/RoMEO'),
                "http://www.sherpa.ac.uk/romeo/issn/$issn/"
              ),
            )
          ),
        ),
        'logo' => array(
          '#type' => 'item',
          '#theme' => 'image',
          '#path' => 'http://www.sherpa.ac.uk/images/romeosmall.jpg',
        ),
      );
    }
    else {
      $no_romeo = TRUE;
    }
  }
  else {
    $no_romeo = TRUE;
  }
  if (isset($no_romeo)) {
    $form['romeo'] = array(
      '#type' => 'item',
      '#markup' => t('No SHERPA/RoMEO data for this publication.'),
    );
  }
  return $form;
}

/**
 * Gets the primary file for the PDF datastream.
 *
 * @param array $form_state
 *   An an array containing the Drupal form state.
 *
 * @return string
 *   The file to be used as the PDF datastream.
 */
function lib4ridora_get_primary_file_for_datastream($form_state) {
  return lib4ridora_get_primary_file_for_datastream_info($form_state['values']['files']);
}

/**
 * Gets the primary file for the PDF datastream.
 *
 * @param array $info
 *   An associative array mapping some form of ID to associative arrays of
 *   datastream info.
 *
 * @return string
 *   The relevant ID for the entry to be used as the PDF datastream.
 */
function lib4ridora_get_primary_file_for_datastream_info($info) {
  $options = lib4ridora_multi_pdf_get_options(TRUE);
  $primary_group = array_fill_keys(array_keys($options['document_version']), FALSE);
  foreach ($info as $filename => $values) {
    if (!$primary_group[$values['document_version']]) {
      $primary_group[$values['document_version']] = $filename;
    }
  }
  $filtered_group = array_filter($primary_group);
  return reset($filtered_group);
}

/**
 * Updates datastream relationships.
 *
 * @param AbstractDatastream $datastream
 *   An AbstractDatastream representing a datastream on an object in Fedora.
 * @param array $values
 *   An array of values to be used.
 */
function lib4ridora_update_datastream_relationships(AbstractDatastream $datastream, $values) {
  if (!($datastream instanceof IslandoraNewFedoraDatastream)) {
    // Doing this because of an inherent Tuque RELS-INT bug.
    $pid = $datastream->parent->id;
    $dsid = $datastream->id;
    $tuque = islandora_get_tuque_connection();
    $tuque->cache->delete($pid);
    $non_cached_object = islandora_object_load($pid);
    $datastream = $non_cached_object[$dsid];
  }
  $filtered = array_intersect_key($values, lib4ridora_multi_pdf_defaults_properties());
  if (isset($filtered['embargo_date']) && $filtered['availability'] == 'date') {
    $date_value = gmdate('Y-m-d\TH:i:s\Z', gmmktime(0, 0, 0, $filtered['embargo_date']['month'], $filtered['embargo_date']['day'], $filtered['embargo_date']['year']));
    lib4ridora_set_relationships($datastream->relationships, LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate('embargo_date'), $date_value, RELS_TYPE_DATETIME);
  }
  else {
    $datastream->relationships->remove(LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate('embargo_date'));
  }
  unset($filtered['embargo_date']);
  foreach ($filtered as $pred => $value) {
    if (!empty($value)) {
      lib4ridora_set_relationships($datastream->relationships, LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate($pred), $value, RELS_TYPE_PLAIN_LITERAL);
    }
    else {
      $datastream->relationships->remove(LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate($pred));
    }
  }
}
