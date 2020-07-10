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
function lib4ridora_multi_embargo_pdf_filter_soft(AbstractDatastream $datastream) {		// original function, with '_soft' attached onto its name.
  return ( $datastream->id == 'PDF' || strpos($datastream->id, variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF')) === 0 );
}

function lib4ridora_multi_embargo_pdf_filter(AbstractDatastream $datastream) {	// modified version of the original function, now also excluding DS ending on 'PDF-A'. Mime-type check still skipped.
  $dsId = strval( $datastream->id );
  if ( $dsId == 'PDF' || strpos($dsId, variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF')) === 0 ) {
    $pdfa_suffix = variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A');
    return ( substr($dsId,0-strlen($pdfa_suffix)) != $pdfa_suffix );
  }
  return FALSE;
}

function lib4ridora_multi_embargo_pdf_filter_hard(AbstractDatastream $datastream) {	// modified version of the original function, now also excluding DS ending on 'PDF-A'. Mime-type check still skipped.
  $dsId = strval( $datastream->id );
  if ( $dsId == 'PDF' || strpos($dsId, variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF')) === 0 ) {
    $pdfa_suffix = variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A');
    if ( substr($dsId,0-strlen($pdfa_suffix)) != $pdfa_suffix ) { return TRUE; }
    // skipping PDF/A file if its availability is set to private
    $infoAry = lib4ridora_get_embargo_info($datastream);
    return ( @!empty($infoAry['availability']) && strtolower($infoAry['availability']) != 'private' );		// 'private' must be hard-coded here, as in lib4ridora_islandora_datastream_access()
  }
  return FALSE;
}

function lib4ridora_internal_document_as_private(&$form_state) {
	$warning = NULL;
	foreach( array('values','input') as $idxMain ) {
		if ( @empty($form_state[$idxMain]['files']) ) { continue; }
		foreach($form_state[$idxMain]['files'] as $pdfId => $pAry ) {
			if ( @stripos($form_state[$idxMain]['files'][$pdfId]['document_version'],'internal') !== false ) {
				if ( @stripos($form_state[$idxMain]['files'][$pdfId]['availability'],'private') === false ) {
					$form_state[$idxMain]['files'][$pdfId]['availability'] = 'private';	// enforce 'private' for internal documents
					$warning = "The availability of the 'Internal Document' was set to 'Private'.";
				}
			}
		}
	}
	return $warning;
}

/**
 * function to sort the PDF datastreams in order to get/keep PDF and its PDF/A together
 */
function lib4ridora_ds_pdf_sort( $pdf_datastreams, $try_dsid = TRUE ) {
  $pdf_sorted = array();
  foreach ($pdf_datastreams as $pdf_dsid => $pdf_datastream ) {
  	$dsid_tmp = ( !$try_dsid || @empty($pdf_datastream->id) ) ? strval($pdf_dsid) : strval($pdf_datastream->id);
	if ( strlen($dsid_tmp) < 3 || substr($dsid_tmp,3,1) == "_" ) {
	  $dsid_tmp = ( str_pad(substr($dsid_tmp,0,3),4,"!") . substr($dsid_tmp,3) );		// just to get e.g. 'PDF_PDF-A' above 'PDF2'.
	}
	$pdf_sorted[$dsid_tmp] = array( $pdf_dsid, $pdf_datastream );
  }
  ksort($pdf_sorted);
  unset($pdf_datastreams);
  $pdf_datastreams = array();
  foreach ($pdf_sorted as $pdf_ds_ary ) {
	$pdf_datastreams[$pdf_ds_ary[0]] = $pdf_ds_ary[1];
  }
  return $pdf_datastreams;
}

/**
 * function to define what term is inserted into the DS label to mark a PDF/A datastream (by default just 'PDF-A).
 * Set up as own function so other modules could easily refer on this quite special/proprietary function.
 */
function lib4ridora_get_pdfa_label_insert() {
  $pdfa_suffix = variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A');
  return strtr(trim(strtr($pdfa_suffix,"/:>?<\\_*\".","      \r\n\n\t"))," \r\n\t","-_'.");
}

/**
 * function to define the label of the PDF/A datastreams
 * Note: if we are going to download this file/ds, then this label will become the filename,
 * that's also why $label_pdf_orig is usually a filename (or a term we can use as a filename).
 */
function lib4ridora_get_pdfa_label( $label_pdf_orig, $parent_dsid = '', $term_new = 'PDF-A', $is_final = TRUE ) {
	if ( !$is_final ) {
		return ( "PDF/A of " . ( $parent_dsid != '' ? $parent_dsid." " : "" ) . "'" . $label_pdf_orig . "'" );
	}
	$label_insert = ( empty($term_new) || strtolower(substr($label_pdf_orig,-4)) == ".pdf" ) ? lib4ridora_get_pdfa_label_insert() : $term_new;
	$nameAry = explode(".",$label_pdf_orig);
	if ( sizeof($nameAry) > 1 ) {
		$ext = array_pop($nameAry);
		return ( implode(".",$nameAry) . "." . $label_insert . "." . $ext );
	}
	if ( !empty($parent_dsid) ) {
		return ( $parent_dsid . " (" . $label_insert . ")" ); 
	}
	if ( !empty($label_pdf_orig) ) {
		return ( $label_pdf_orig . " (" . $label_insert . ")" ); 
	}
	return $label_insert;		// aux.
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

  // Adding a JS function to switch the availability to 'private' for 'internal documents'.
  // This shall help the user, to enforce 'private' in DORA we better should use a valiation or submit hook.
  // drupal_add_js( drupal_get_path('module','lib4ridora') . '/js/lib4ridora.internal_as_private.js', 'file' );
  $jsPath = drupal_get_path('module','lib4ridora') . '/js/lib4ridora.internal_as_private.txt';
  drupal_add_js( file_get_contents($jsPath), 'inline' );	// this is less 'caching-resistent'
  foreach($form['files'] as $uuid => $uAry ) {
    if ( @is_array($uAry['document_version']) ) {
      $form['files'][$uuid]['document_version']['#attributes']['onchange'] = "PdfInternalAsPrivate('{$uuid}');";
    }
  }

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
  $pdf_datastreams = array_filter( $datastreams, 'lib4ridora_multi_embargo_pdf_filter_soft' );
  lib4ridora_append_document_information($pdf_datastreams, $form );
  lib4ridora_append_romeo_information($form, $form_state);
  $form['actions'] = array(
    '#type' => 'actions',
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Update properties'),
      '#suffix' => '<br>&nbsp;<br>',
    ),
  );

  // Adding a JS function to switch the availability to 'private' for 'internal documents'.
  // This shall help the user, to enforce 'private' in DORA we better should use a valiation or submit hook.
  // drupal_add_js( drupal_get_path('module','lib4ridora') . '/js/lib4ridora.internal_as_private.js', 'file' );
  $jsPath = drupal_get_path('module','lib4ridora') . '/js/lib4ridora.internal_as_private.txt';
  drupal_add_js( file_get_contents($jsPath), 'inline' );	// this is less 'caching-resistent'
  foreach($form['files'] as $pdfId => $pAry ) {
    if ( @is_array($pAry['document_version']) ) {
      $form['files'][$pdfId]['document_version']['#attributes']['onchange'] = "PdfInternalAsPrivate('".strtolower($pdfId)."');";
    }
  }

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
function lib4ridora_get_embargo_info(AbstractDatastream $datastream, $keep_defaults = TRUE ) {
  // Add in defaults.
  $to_return = lib4ridora_multi_pdf_defaults_properties();

  foreach ($to_return as $pred => $default) {
    if ( !$keep_defaults ) {
    	$to_return[$pred] = NULL;
    }
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
        $to_return[$pred] = $value['object']['value'];				// 'object' + 'value' are some predefined 'sub-keys' of RELS-INT
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

	if ( $warning = lib4ridora_internal_document_as_private($form_state) ) { drupal_set_message($warning, 'warning'); }

	$step_storage = islandora_ingest_form_get_step_storage($form_state, 'lib4ridora_multi_pdf_upload');
	$object = lib4ridora_multi_embargo_get_object($form_state);
	$pid = $object->id;
	$primary_file = lib4ridora_get_primary_file_for_datastream($form_state);
	$file_info = $step_storage['files'];
	// The files are keyed in the values by uuid for fieldset uniqueness for
	// states, let's construct this mapping now to get back to the files.
	$mapped_files = array();
	foreach ($file_info as $file) {
		$mapped_files[$file->uuid] = $file;
	}
	$pdf_nr_suffix = 2;
	$pdfa_suffix = variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A');
	foreach ($form_state['values']['files'] as $filename => $values) {
		if ( substr($filename,0-strlen($pdfa_suffix)) == $pdfa_suffix ) {
			continue;
		}
		// Construct the datastreams.
		$dsid_new = variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF');
		if ( $filename != $primary_file ) {
			$dsid_new = format_string('@prefix@index', array(
				'@prefix' => variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF'),
				'@index' => $pdf_nr_suffix,
			));
			$pdf_nr_suffix++;
		}
		$dsid_pdfa = $filename . $pdfa_suffix;		// old/current name of PDF/A darastream
		if ( @isset($mapped_files[$dsid_pdfa]) ) {
			$dsid_new_pdfa = $dsid_new . $pdfa_suffix;		// new datastream name
			$datastream = /* ( @isset($mapped_files[$dsid_new_pdfa]) ) ? $mapped_files[$dsid_new_pdfa] : */ $object->constructDatastream($dsid_new_pdfa, 'M');
			$datastream->mimetype = $mapped_files[$dsid_pdfa]->filemime;
			$datastream->label = lib4ridora_get_pdfa_label( $mapped_files[$filename]->filename, $dsid_new );		// by default $mapped_files[$filename]->filename is the original PDF filename
			$datastream->setContentFromFile($mapped_files[$dsid_pdfa]->uri, FALSE);
			$object->ingestDatastream($datastream);
			$values_pdfa = $values; 	// $values is an array with the following indices: document_version | availability | embargo_date | use_permission
			if ( $avail_custom = lib4ridora_pdfa_availability_default(FALSE) ) {
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
  $pdf_datastreams = array_filter( $datastreams, 'lib4ridora_multi_embargo_pdf_filter_soft' );		// soft! - do the following PDF and PDF/A as well!
  foreach ($pdf_datastreams as $dsid => $ds) {
    foreach (lib4ridora_multi_pdf_defaults_properties() as $pred => $default) {
      $ds->relationships->remove(LIB4RIDORA_RELS_URI, lib4ridora_multi_embargo_build_predicate($pred));
    }
    $object->purgeDatastream($dsid);
  }
}

function lib4ridora_multi_embargo_form_validate(&$form, &$form_state) {
	// dpm( 'lib4ridora_multi_embargo_form_validate: ' . print_r($form_state,TRUE) );
	if ( $warning = lib4ridora_internal_document_as_private($form_state) ) { drupal_set_message($warning, 'warning'); }
}

/**
 * function to instantly set open access or restricted
 *
function lib4ridora_handle_open_access( $pid, $chk ) {
    module_load_include('inc', 'lib4ridora', 'includes/full_text');
	$tuque = islandora_get_tuque_connection();
	$tuque->cache->delete($pid);
	$obj = islandora_object_load($pid);
	lib4ridora_update_full_text( $obj, TRUE );
}
 */

/**
 * function to compose a warning if the set embargo date in the past.
 */
function lib4ridora_get_note_embargo_date_passed( $embargo_date_unix, $ds_label ) {
	$msg_new = "<font color='red'>WARNING</font>: ";
	$msg_new .= "The current <b>embargo date</b> (currently '" . gmdate("Y-m-d", $embargo_date_unix ) . "') for '<b>" . $ds_label . "</b>' should be in the future!<br>";
	$msg_new .= "Please recheck it or set the availability to 'public', otherwise this PDF will be available for everybody soon after midnight! <br>\n";
	return $msg_new;
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

	$errorAry = array();	// to hold error messages
	if ( $warning = lib4ridora_internal_document_as_private($form_state) ) { $errorAry[] = $warning; }

	date_default_timezone_set('GMT');
	$object = lib4ridora_multi_embargo_get_object($form_state);
	$pid = $object->id;
	$primary_file = lib4ridora_get_primary_file_for_datastream($form_state);
	$pdfa_suffix = variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A');

	if ( $primary_file == 'PDF' ) {
		// just update the RELS-INT, and copy over the document version to its PDF/A
	//	$object->relationships->autoCommit = FALSE;		// part 1/2, will need commitRelationships()
	//	REVISED + disabled again. The following code may cause DS manipulations caught by hooks/functions that possibly want to rewrite/update RELS-INT/RELS-EXT too.
		foreach ($form_state['values']['files'] as $dsid => $values) {
			lib4ridora_update_datastream_relationships($object[$dsid], $values);
			if ( substr($dsid,0-strlen($pdfa_suffix)) != $pdfa_suffix ) {
				// Error message for embargo date in the past:
				if ( $values['availability'] == "date" ) {
					if ( $embargo_date_string = lib4ridora_get_embargo_date_string($values['embargo_date']) ) {
						$embargo_date_unix = strtotime($embargo_date_string);		// working but unpretty, to be tuned
						if ( $embargo_date_unix < time() ) { $error_msg .= lib4ridora_get_note_embargo_date_passed( $embargo_date_unix, $object[$dsid]->label ); }
					}
				}
				$dsid_pdfa = $dsid . $pdfa_suffix;
				if ( @isset($object[$dsid_pdfa]) ) {
					$values_pdfa = $values; 	// $values is an array with the following indices: document_version | availability | embargo_date | use_permission
					// Set Lib4RI's default availability for PDF/A:
					if ( $avail_custom = lib4ridora_pdfa_availability_default(FALSE) ) {
						if ( strlen($avail_custom) > 1 ) { $values_pdfa['availability'] = $avail_custom; }
					}
					// If somebody removed the embargo date manually (when edited in PDF Management) then remove it from RELS-INT too:
					if ( $values['availability'] != "date" || empty(lib4ridora_get_embargo_date_string($values['embargo_date'])) ) {
						$pred_full = lib4ridora_multi_embargo_build_predicate('embargo_date');
					//	$valueAry = $object[$dsid_pdfa]->relationships->get(LIB4RIDORA_RELS_URI,$pred_full);		// note: here it can be $valueAry[0]['object']['value']
					// TAKE CARE, here the structure can be $valueAry[#]['object']['value'] or just $valueAry['object']['value'] !
						$infoAry = lib4ridora_get_embargo_info($object[$dsid_pdfa],FALSE);
						if ( !empty(lib4ridora_get_embargo_date_string($infoAry['embargo_date'])) ) {
							$object[$dsid_pdfa]->relationships->remove(LIB4RIDORA_RELS_URI,$pred_full);
						}
						if ( @isset($values_pdfa['embargo_date']) ) { unset($values_pdfa['embargo_date']); }
					}
					lib4ridora_update_datastream_relationships($object[$dsid_pdfa], $values_pdfa);
				}
			}
		}
	//	$object->relationships->commitRelationships();		// part 2/2

		// If all datastreams keep their names/IDs, then islandora_derivative_logging() won't produce an update massage (about now/other derivatives), let's create an own one:
		if ( sizeof($errorAry) ) { drupal_set_message( trim(implode(" <br>\r\n",$errorAry)), 'warning' ); }
		else { drupal_set_message(t('Derivatives successfully <a href="./datastream/RELS-INT/version/0/view">updated</a>.'), 'status'); }

 		return;
	}

	$non_cached_object = $object;		// aux, will be replaced.
	$mapped_files = array();
	$datastreams = iterator_to_array($object);

	$pdf_datastreams = array_filter( $datastreams, 'lib4ridora_multi_embargo_pdf_filter_soft' );		// soft! - do the following PDF and PDF/A as well!
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
	foreach( @$form_state['values']['files'] as $filename => $values) {		// $filename is the DS Id
		if ( substr($filename,0-strlen($pdfa_suffix)) == $pdfa_suffix ) {
			continue;
		}
		$dsid_new = variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF');
		if ( $filename != $primary_file ) {
			$dsid_new = format_string('@prefix@index', array(
				'@prefix' => variable_get('lib4ridora_extra_pdf_datastream_prefix', 'PDF'),
				'@index' => $pdf_nr_suffix,
			));
			$pdf_nr_suffix++;
		}
		// Error message for embargo date in the past:
		if ( $values['availability'] == "date" ) {
			if ( $embargo_date_string = lib4ridora_get_embargo_date_string($values['embargo_date']) ) {
				$embargo_date_unix = strtotime($embargo_date_string);		// working but unpretty, to be tuned
				if ( $embargo_date_unix < time() ) { $errorAry[] = lib4ridora_get_note_embargo_date_passed( $embargo_date_unix, $non_cached_object[$filename]->label ); }
			}
		}
		// Construct the datastreams.
		$dsid_pdfa = $filename . $pdfa_suffix;		// old/current name of PDF/A darastream
		if ( @isset($mapped_files[$dsid_pdfa]) ) {
			$dsid_new_pdfa = $dsid_new . $pdfa_suffix;		// new datastream name
			$datastream = $non_cached_object->constructDatastream($dsid_new_pdfa, 'M');
			$datastream->mimetype = $mapped_files[$dsid_pdfa]['mime'];
			$datastream->label = lib4ridora_get_pdfa_label( $mapped_files[$filename]['label'], $dsid_new );		// by default $mapped_files[$filename]['label'] is the original PDF filename
			$datastream->setContentFromFile($mapped_files[$dsid_pdfa]['content'], FALSE);
			$non_cached_object->ingestDatastream($datastream);
			file_unmanaged_delete($mapped_files[$dsid_pdfa]['content']);

			$values_pdfa = $values; 	// $values is an array with the following indices: document_version | availability | embargo_date | use_permission
			// Set Lib4RI's default availability for PDF/A:
			if ( $avail_custom = lib4ridora_pdfa_availability_default(FALSE) ) {
				if ( strlen($avail_custom) > 1 ) { $values_pdfa['availability'] = $avail_custom; }
			}
			// If somebody removed the embargo date manually (when edited in PDF Management) then remove it from its PDF/A in RELS-INT too:
			if ( $values['availability'] != "date" || empty(lib4ridora_get_embargo_date_string($values['embargo_date'])) ) {
				$pred_full = lib4ridora_multi_embargo_build_predicate('embargo_date');
			//	$valueAry = $non_cached_object[$dsid_new_pdfa]->relationships->get(LIB4RIDORA_RELS_URI,$pred_full);		// note: here it can be $valueAry[0]['object']['value']
			// TAKE CARE, here the structure can be $valueAry[#]['object']['value'] or just $valueAry['object']['value'] !
				$infoAry = lib4ridora_get_embargo_info($non_cached_object[$dsid_new_pdfa],FALSE);
				if ( !empty(lib4ridora_get_embargo_date_string($infoAry['embargo_date'])) ) {
					$non_cached_object[$dsid_new_pdfa]->relationships->remove(LIB4RIDORA_RELS_URI,$pred_full);
				}
				if ( @isset($values_pdfa['embargo_date']) ) { unset($values_pdfa['embargo_date']); }
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

	// If all datastreams keep their names/IDs, then islandora_derivative_logging() won't produce an update massage (about now/other derivatives), let's create an own one:
	if ( sizeof($errorAry) ) { drupal_set_message( trim(implode(" <br>\r\n",$errorAry)), 'warning' ); }
	else { drupal_set_message(t('Derivatives successfully <a href="./datastream/RELS-INT/version/0/view">updated</a>.'), 'status'); }
}

function lib4ridora_multi_embargo_edit_form_validate(&$form, &$form_state) {
	// dpm( 'lib4ridora_multi_embargo_edit_form_validate: ' . print_r($form_state,TRUE) );
	if ( $warning = lib4ridora_internal_document_as_private($form_state) ) { drupal_set_message($warning, 'warning'); }
}

/**
 * Set a relationship.
 * Add a relationship such that it is the only relationship with the given predicate for the given subject.
 *
 * Revised Nov-2018: Only set the new value if it not exisits or if it is not equal - this should reduce the amount of RELS-INT datastream notably,
 * or at least - if we press update properties on the 'PDF Management' tab without any changes - now this will not produce additional/redundant RELS-INT versions.
 * See also Redmine issue #80: http://lib-dora-dev1.emp-eaw.ch:3000/issues/80
 */
function lib4ridora_set_relationships(FedoraRelationships $rels, $pred_uri, $pred, $object_value, $object_type = RELS_TYPE_URI) {

	$predAry = $rels->get( $pred_uri, $pred, NULL, TRUE );
	$num_found = sizeof($predAry);
	if ( $num_found > 1 ) {
		// sanity tuning, only allow one - this case basically matches the original function.
		$rels->remove($pred_uri, $pred);
		$rels->add($pred_uri, $pred, $object_value, $object_type);
		return;
	}
	// without sanity tuning (or then disabled) let's always take the last one (if a ds should have more than one entry for e.g. 'document_version'):
	// note: as tested the format of embargo_date for $value_found is e.g. '2019-04-28T00:00:00Z', and $object_value has the same format (luckily).
	$value_found = @$predAry[($num_found-1)]['object']['value'];
	if ( $num_found < 1 || $object_value != $value_found ) {
		if ( $num_found > 0 ) {
			$rels->remove($pred_uri, $pred);
		}
		$rels->add($pred_uri, $pred, $object_value, $object_type);
	}
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
 * Helper function, to return the default PDF/A availability (as string).
 */
function lib4ridora_pdfa_availability_default( $help_with_pdf_default = TRUE ) {
  $availability = variable_get('lib4ri_pdfa_availability','private');		// caution. as currently implemented we better should consider 1 single character as empty too.
  if ( $help_with_pdf_default && strlen($availability) < 2 ) {
    $availability = variable_get('lib4ridora_multi_pdf_defaults_property_availability', 'intranet');
  }
  return trim(strtolower($availability));
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
    'month' => $date['mon'],
    'day' => $date['mday'],
  );
  return array(
    'availability' => strtolower(variable_get('lib4ridora_multi_pdf_defaults_property_availability', 'intranet')),
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
   /* 'updated version' => t('Updated Version'),		<== not needed anymore, see PDF_Fix_Order.pdf ( http://lib-dora-dev1.emp-eaw.ch:3000/issues/79 ) */
      'erratum' => t('Erratum'),
      'supplemental material' => t('Supplemental Material'),
      'internal document' => t('Internal Document'),	/* only for admins, see below */
      'unspecified' => t('Unspecified PDF'),
    );
  }
  else {
    $document_versions = array(
      'accepted version' => t('Accepted Version'),
      'published version' => t('Published Version'),
      'supplemental material' => t('Supplemental Material'),
   /* 'updated version' => t('Updated Version'),		<== not needed anymore, see PDF_Fix_Order.pdf ( http://lib-dora-dev1.emp-eaw.ch:3000/issues/79 ) */
      'erratum' => t('Erratum'),
      'internal document' => t('Internal Document'),	/* only for admins, see below */
      'unspecified' => t('Unspecified PDF'),
    );
  }
  
  global $user;		// see http://lib-dora-dev1.emp-eaw.ch:3000/issues/314
  if ( !user_is_logged_in() || !$user || !in_array('administrator',array_values($user->roles)) ) {
	unset($document_versions['internal document']);
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
function lib4ridora_append_document_information($files, &$form ) {
  module_load_include('inc', 'lib4ridora', 'includes/utilities');
  // See if we are ingesting or editing.
  $type_file = reset($files);
  $edit = ( $type_file instanceof IslandoraFedoraDatastream );
  $options = lib4ridora_multi_pdf_get_options();
  $skip_pdfa = ( variable_get('lib4ri_pdfa_auto_managed',TRUE) && ( !$edit || @!isset($_GET['all']) ) );
  $pdfa_suffix = variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A');
  if ( sizeof($files) > 1 ) {
	$files = lib4ridora_ds_pdf_sort( $files );
  }
  foreach ($files as $fIdx => $file) {
    $uuid = $edit ? $file->id : $file->uuid;
    // check if we need to skip PDF/A:
	$is_pdfa = ( substr($uuid,0-strlen($pdfa_suffix)) == $pdfa_suffix );
    if ( $is_pdfa && $skip_pdfa ) { continue; }
 
    $defaults = $edit ? lib4ridora_get_embargo_info($file,(!$is_pdfa)) : lib4ridora_multi_pdf_defaults_properties();
    
	if ( !lib4ridora_internal_doc_access($defaults) ) { continue; }

    $filename = $edit ? $file->label : $file->filename;
    $form['files'][$uuid] = array(
      '#type' => 'fieldset',
      '#title' => t('@filename', array(
        '@filename' => ( ( $fIdx == '' || $fIdx == $filename ) ? $filename : "{$fIdx} : {$filename}" ),
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
    // PDF/A related form/display tunings:
    // Tricky situation if the original PDF has an embargo date (and so the PDF/A will get it too), however
    // we have decided that the PDF/A shell get the availablility 'private' to be hidden from public access.
    // For the time being let's leave a hint about this issue (if the user wants to treat PDF/As here).
    if ( !$skip_pdfa && substr($uuid,0-strlen($pdfa_suffix)) == $pdfa_suffix ) {
      if ( empty(lib4ridora_get_embargo_date_string($defaults['embargo_date'])) ) {
        $form['files'][$uuid]['embargo_date']['#title'] .= " (so far unassigned, hence can be ignored)";
      } else {
	  $form['files'][$uuid]['embargo_date']['#title'] .= " (recently assigned for '" . $files[substr($uuid,0,0-strlen($pdfa_suffix))]->label . "')";
	}
    	unset($form['files'][$uuid]['embargo_date']['#title_display']);
    }
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
  return @lib4ridora_get_primary_file_for_datastream_info($form_state['values']['files']);
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
  $primary_group = @array_fill_keys(array_keys($options['document_version']), FALSE);
  foreach ($info as $filename => $values) {
    if (@!$primary_group[$values['document_version']]) {
      $primary_group[$values['document_version']] = $filename;
    }
  }
  $filtered_group = array_filter($primary_group);
  return reset($filtered_group);
}

/**
 * Compose the string how the embargo date is saved in the RELS-INT of Lib4RI.
 * Outsourced from lib4ridora_update_datastream_relationships() so other modules easier can access this special/ustomized string format.
 *
 * @param array $date_array
 *   An associative array containing keys 'year', 'month' and 'day'.
 *
 * @return string
 *   Date string (as used in RELS-INT datastream).
 */
function lib4ridora_get_embargo_date_string( $date_array, $treat_empty_array = FALSE ) {
	if ( gettype($date_array) == "string" ) {
		return $date_array;	// already a string!(?)
	}
	$time_unix = gmmktime(0, 0, 0, @$date_array['month'], @$date_array['day'], @$date_array['year']);
	if ( !$treat_empty_array && ( !sizeof($date_array) || empty($time_unix) ) ) {
		return "";		// $date_array may be empty e.g. right after ingesting without an embargo date.
	}
	return gmdate('Y-m-d\TH:i:s\Z', $time_unix);
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

  $pdfa_suffix = variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A');
  $filtered = array_intersect_key($values, lib4ridora_multi_pdf_defaults_properties());
  foreach ($filtered as $pred => $value) {
    $pred_full = lib4ridora_multi_embargo_build_predicate($pred);
    if ( $pred == 'embargo_date' && ( $filtered['availability'] == 'date' || 
    ( substr($datastream->id,0-strlen($pdfa_suffix)) == $pdfa_suffix && strtolower($filtered['availability']) == lib4ridora_pdfa_availability_default() ) ) ) {
      $date_value = lib4ridora_get_embargo_date_string($value);
      lib4ridora_set_relationships($datastream->relationships, LIB4RIDORA_RELS_URI, $pred_full, $date_value, RELS_TYPE_DATETIME);
    }
    else if ( $pred != 'embargo_date' && !empty($value)) {
     lib4ridora_set_relationships($datastream->relationships, LIB4RIDORA_RELS_URI, $pred_full, $value, RELS_TYPE_PLAIN_LITERAL);
    }
    elseif ( $datastream->relationships->get(LIB4RIDORA_RELS_URI,$pred_full) !== NULL ) {
      $datastream->relationships->remove(LIB4RIDORA_RELS_URI,$pred_full);
    }
  }
}
