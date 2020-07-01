<?php

/**
 * @file
 * Utility functions for the module.
 */

/**
 * Retrieve the journal identified via an ISSN or EISSN.
 *
 * @param string $identifier
 *   An ISSN or EISSN.
 *
 * @return mixed
 *   The object that is the journal or FALSE if none found.
 */
function lib4ridora_get_journal($identifier) {
  $journal = FALSE;
  $issn_field = variable_get('lib4ridora_issn_solr_field', 'mods_identifier_issn_s');
  $e_issn_field = variable_get('lib4ridora_e_issn_solr_field', 'mods_identifier_e-issn_s');
  $solr = new IslandoraSolrQueryProcessor();
  $solr->buildQuery("$issn_field:\"$identifier\" OR $e_issn_field:\"$identifier\"");
  $solr->solrLimit = 1;
  $solr->solrParams['fl'] = implode(
    ',',
    array(
      'PID',
      $issn_field,
      $e_issn_field,
    )
  );
  $solr->executeQuery(FALSE);
  if (isset($solr->islandoraSolrResult['response']['objects'][0])) {
    $journal = $solr->islandoraSolrResult['response']['objects'][0]['solr_doc']['PID'];
  }
  return islandora_object_load($journal);
}

/**
 * Get an impact factor.
 *
 * @param AbstractObject $journal
 *   The journal object to get the factor for.
 * @param string $year
 *   A four character string indicating the year of the factor.
 *
 * @return mixed
 *   The impact factor string or FALSE if not set.
 */
function lib4ridora_get_impact_factor(AbstractObject $journal, $year) {
  if (!isset($journal[LIB4RIDORA_FACTOR_DSID])) {
    return FALSE;
  }
  $impact_factors = json_decode($journal[LIB4RIDORA_FACTOR_DSID]->content, TRUE);
  return isset($impact_factors[$year]) ? $impact_factors[$year] : FALSE;
}

/**
 * Set the impact factor on a journal's year.
 *
 * @param AbstractObject $journal
 *   The journal object to set the factor for.
 * @param string $year
 *   A four character string indicating the year of the factor.
 * @param string $factor
 *   The factor to set.
 */
function lib4ridora_set_impact_factor(AbstractObject $journal, $year, $factor) {
  if (isset($journal[LIB4RIDORA_FACTOR_DSID])) {
    $factor_datastream = $journal[LIB4RIDORA_FACTOR_DSID];
    $factors = json_decode($factor_datastream->content, TRUE);
  }
  else {
    $factor_datastream = $journal->constructDatastream(LIB4RIDORA_FACTOR_DSID, 'M');
    $factor_datastream->label = 'Journal Impact Factors';
    $factor_datastream->mimetype = 'application/json';
    $factors = array();
  }

  $factors[$year] = $factor;
  $factor_datastream->content = json_encode($factors);

  // Doesn't do anything if ingested already.
  $journal->ingestDatastream($factor_datastream);
}

/**
 * Get factor with fallback to previous years.
 *
 * @param AbstractObject $object
 *   A journal object from which to attempt to fetch an impact factor.
 * @param array|string $issued_years
 *   Either a string or an array of strings, each numerical, representing years
 *   in which to test for impact factors. It was indicated that we should
 *   fallback to obtain factors for one year preceding, if a factor for the
 *   actual year could not be found.
 *   Allowing an array is for convenience, as silly things could happen with
 *   different date encodings we basing things on.
 *
 * @return array|bool
 *   If a factor could be found, an array containing the factor for the first
 *   year (in the order they were provided in $issued_years) and the year;
 *   otherwise, FALSE.
 */
function lib4ridora_get_factor_with_fallback(AbstractObject $object, $issued_years) {
  $issued_years = (array) $issued_years;
  $issued_years_with_fallbacks = $issued_years;
  foreach ($issued_years as $year) {
    // They wish to fallback to trying to use values from a preceding year.
    $issued_years_with_fallbacks[] = $year - 1;
    $issued_years_with_fallbacks[] = $year - 2;		// ...and the year before, accordin to Lib4RI Jour Fix from 2019-Jan-15 about Redmine Issue #48 / http://lib-dora-dev1.emp-eaw.ch:3000/issues/48
  }
  $issued_years_with_fallbacks = array_unique($issued_years_with_fallbacks, SORT_NUMERIC);

  $factor = FALSE;
  $factor_year = FALSE;
  foreach ($issued_years_with_fallbacks as $year) {
    $factor = lib4ridora_get_impact_factor($object, $year);
    if ($factor !== FALSE) {
      $factor_year = $year;
      break;
    }
  }

  if ( $factor === FALSE ) {
    	return FALSE;
  }

  // Try to replace the term for 'not available (yet)' from the CSV (or since imported from the datastream now) by a custom term:
  // Let's also consider the (future) possibility that this field remains empty or carries another value, but we still want a/our custom term.
  $termRepList = explode( ",", variable_get('lib4ridora_pseudo_solr_field_factor_none_yet', 'Not available = not available') );
  foreach( $termRepList as $termTmp ) {
	if ( strchr($termTmp,"=") ) {
		$termAry = explode("=",$termTmp,2);
		$termTmp = trim($termAry[1]);
		if ( !empty( trim($factor) ) ) {
		  	if ( stristr(strval($factor),trim($termAry[0])) ) {
				$factor = $termTmp;
		  	}
		} elseif ( !empty($termTmp) ) { $factor = $termTmp; }
	}
  }

  return array($factor, $factor_year);
}


/*
 * Function to return an array where the keys are the names of the PDF datastreams, and each key will carry an array with document-version and availability as defined in RELS-INT
 *
 */
function lib4ridora_get_embargo_data( $relsIntCode, $dsPdfAry ) {		// $dsPdfAry may be: array( 'PDF2', 'PDF', 'PDF_PDF-A', 'PDF2_PDF-A' )

	// very important: the array we are going to return must have as many key as there were items inside the input array!
	$dataAry = array_flip($dsPdfAry);

	// DOM/XML tweak (to help with minus signs and colons):
	$repList = array( "rdf_" => "rdf:", "dora_lib4ri" => "dora:lib4ri", "_multi_embargo_" => "-multi-embargo-" );
	foreach( $repList as $new => $orig ) { $relsIntCode = str_replace($orig,$new,$relsIntCode); }

	$xmlObj = new SimpleXMLElement($relsIntCode);
	
	// now going through RELS-INT looking for information about the PDF datastreams:
	foreach($xmlObj->rdf_Description as $dIdx => $dElem ) {
		if ( $dsLink = @strval( $dElem->attributes()->rdf_about ) ) {		// someting like info:fedora/eawag:156/PDF"
			$dsPos = empty($pid) ? (strrpos($dsLink,"/")+1) : (stripos($dsLink,$pid)+strlen($pid)+1);
			$dsId = substr($dsLink,$dsPos);
			foreach( $dsPdfAry as $dsPdfIdx => $dsPdfName ) {		// $dsName may be e.g. 'PDF2' or 'PDF_PDF-A'
				if ( $dsPdfName == $dsId ) {
					// remind, a replacement happened above, so do not use 'lib4ridora-multi-embargo-document_version' or 'lib4ridora-multi-embargo-availability' here!
					$dataAry[$dsPdfName] = array( 'doc_ver' => strval($dElem->islandora_lib4ridora_multi_embargo_document_version), 'avail' => strval($dElem->islandora_lib4ridora_multi_embargo_availability) );
					unset($dsPdfAry[$dsPdfIdx]);
					break;
				}
			}
		}
	}
	return $dataAry;		// array( 'PDF' => array( 'doc_ver' => "Published", 'avail' => "intranet"), 'PDF2' => array...
}

/**
 * Retrieve PDFs.
 *
 * @param AbstractObject $object
 *   An ABstractObject representing an object within Fedora.
 *
 * @return array
 *   The listing of the PDFs on the object.
 *
 * Note: this function SHOULD BE also effective for 'Recent Additions' via lib4ridora-citation-solr-results.tpl.php / template_preprocess_lib4ridora_citation_solr_results() / template_preprocess_lib4ridora_pdf_materials(&$variables)
 */
function lib4ridora_retrieve_filtered_pdfs(AbstractObject $object) {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');			// providing lib4ridora_multi_pdf_get_options()
  $dses = iterator_to_array($object);
  $dses = array_filter($dses, 'lib4ridora_multi_embargo_pdf_filter');		// function lib4ridora_multi_embargo_pdf_filter() in embargo.form.inc, just looking for DS beginning the 'PDF'

	// 2018-Oct-17: get the desired order for PDFs (corresponding to possible 'document versions' in RELS-INT), see also Redmine-Issue #79.
	$lib4ridora_multi_pdf_options = lib4ridora_multi_pdf_get_options( TRUE /* true to get the PDF link order as required for Lib4RI */ );
	$default_list = implode( ", ", array_map( function($ver) { return strtok(ltrim($ver),' '); }, $lib4ridora_multi_pdf_options['document_version'] ) );
	$orderAry = explode(",", variable_get('lib4ridora_multi_pdf_link_order', /* Published, Accepted, Supplemental, ... */ $default_list ) );

	if ( !empty( trim( implode("",$orderAry) ) ) && @isset($object['RELS-INT']) ) {		// safety check
		$relsIntCode = $object['RELS-INT']->content;
		if ( !empty($relsIntCode) ) {
			$dsPdfAry = lib4ridora_get_embargo_data( $relsIntCode, array_keys($dses) );	// array_keys() for array( 0 => 'PDF', 1 => "PDF_PDF-A", 2 => "PDF...
			if ( sizeof($dses) == sizeof($dsPdfAry) ) {		// for safety
				$rankAry = array();	// for all index of PDFs that have a version label that is inside our order list
				while( sizeof($orderAry) ) {
					$wanted = trim( strtolower( array_shift($orderAry) ) );
					foreach( $dsPdfAry as $pIdx => $pAry ) {			// $pIdx may be 'PDF3' or 'PDF2_PDF-A', etc., $pAry may be array( 'doc_ver' => "Erratum", 'avail' => "intranet" )
						if ( gettype($pAry) == "array" && substr(strtolower($pAry['doc_ver']),0,strlen($wanted)) == $wanted ) {
							$rankAry[$pIdx] = $pIdx;
					//		break;		// better do not, there may be more than one PDF all with the same document version.
						}
					}
				}
				foreach( $dsPdfAry as $pIdx => $pAry ) {		// add PDFs possibly missed because they matched no item in the order list
					if ( @!isset($rankAry[$pIdx]) ) { $rankAry[$pIdx] = $pIdx; }
				}
				if ( sizeof($dses) == sizeof($rankAry) ) {	// for safety
					$dsesAry = array();
					foreach( $rankAry as $pIdx ) { $dsesAry[$pIdx] = $dses[$pIdx]; }
					return $dsesAry;
				}
			}
		}
	}

  ksort($dses);
  return $dses;
}

/**
 * Extract author info from an object.
 *
 * @param AbstractObject $object
 *   An object from which to extract a list of authors.
 *
 * @return array
 *   An array of associtive arrays of the form:
 *   - author: An associative array containing:
 *     - name: The author's name.
 *     - id: The author's ID.
 *   - org: An associative array containing:
 *     - name: The organization's name.
 *     - id: The organization's ID.
 */
function lib4ridora_get_author_info(AbstractObject $object) {
  $dom = new DOMDocument();
  $dom->loadXML($object['MODS']->content);
  $xpath = new DOMXPath($dom);
  $xpath->registerNamespace('mods', 'http://loc.gov/mods/v3');

  $authors = array();

  foreach ($xpath->query('//mods:mods/mods:name[@type="personal"][mods:role/mods:roleTerm[@authority="marcrelator" and @type="text"]/text() = "author"]') as $person) {
    $info = array(
      'author' => array(
        'name' => $xpath->evaluate('normalize-space(concat(mods:namePart[@type="family"], ", ", mods:namePart[@type="given"]))', $person),
        'id' => $xpath->evaluate('string('.variable_get('lib4ridora_mods_query_author_id', 'mods:nameIdentifier[@type="authorId"]').'/text())', $person),
      ),
      'org' => array(
        'name' => $xpath->evaluate('normalize-space(mods:affiliation)', $person),
        'id' => $xpath->evaluate('string(mods:nameIdentifier[@type="organizational unit id"]/text())', $person),
      ),
    );

    if (!$info['author']['name']) {
      $info['author']['name'] = $xpath->evaluate('normalize-space(mods:fullName)', $person);
    }

    $authors[] = $info;
  }

  return $authors;
}

/**
 * Determine the form to use to edit the given object.
 *
 * @param AbstractObject $object
 *   The object for which to determine the form.
 *
 * @return string|NULL
 *   The name of the form if we have one; otherwise, NULL.
 */
function lib4ridora_determine_form_name(AbstractObject $object) {
  if (!isset($object['MODS'])) {
    watchdog('lib4ridora', 'Attempted to determine the form for an object without MODS: @object_id', array(
      '@object_id' => $object->id,
    ));
    return;
  }

  module_load_include('inc', 'lib4ridora', 'includes/citation.subtypes');
  $subtypes = lib4ridora_determine_subtypes($object, FALSE);
  $first = reset($subtypes);
  return $first ? $first['form'] : NULL;
}

/**
 * Retrieve the author string from a PID.
 */
function lib4ridora_get_author_from_pid($pid) {
  $author_object = islandora_object_load($pid);
  if ($author_object) {
    return $author_object->label;
  }
  return $pid;
}

/**
 * Retrieves the PID from an author string.
 *
 * @param string $author
 *   The full name of the author to be searched for.
 *
 * @return bool|string
 *   FALSE if an author cannot be found or the PID otherwise.
 */
function lib4ridora_get_pid_from_author($author) {
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery(format_string('@query:"@author"', array(
    '@query' => variable_get('lib4ridora_author_fullname_solr_field', 'MADS_fullname_ms'),
    '@author' => $author,
  )));
  $qp->executeQuery(FALSE);
  if ($qp->islandoraSolrResult['response']['numFound'] != 0) {
    $result = reset($qp->islandoraSolrResult['response']['objects']);
    return $result['PID'];
  }
  else {
    return FALSE;
  }
}

/**
 * Helper to determine the metadata association to use for a given object.
 *
 * @param array $elements
 *   A reference to an associative array as passed to
 *   hook_islandora_solr_metadata_display_elements_alter() and
 *   hook_islandora_solr_metadata_description_elements_alter().
 *
 * @see hook_islandora_solr_metadata_display_elements_alter()
 * @see hook_islandora_solr_metadata_description_elements_alter()
 */
function lib4ridora_metadata_display_elements(&$elements) {
  if (in_array('ir:citationCModel', $elements['islandora_object']->models)) {
    module_load_include('inc', 'lib4ridora', 'includes/citation.subtypes');
    $display = NULL;
    foreach (lib4ridora_determine_subtypes($elements['islandora_object'], TRUE) as $type => $info) {
      if ($info['display'] !== NULL) {
        $display = $info['display'];
        break;
      }
    }
    if ($display === NULL) {
      return;
    }

    module_load_include('inc', 'islandora_solr_metadata', 'includes/db');
    $config_id = islandora_solr_metadata_retrieve_configuration_from_machine_name($display);
    if (!$config_id) {
      // Config doesn't exist... Features out of whack?
      watchdog('lib4ridora', 'The "@name" metadata display does not appear to exist, loading a "@type" object.', array(
        '@name' => $display,
        '@type' => $type,
      ));
      return;
    }

    $elements['associations'][] = array('configuration_id' => $config_id);
  }
}

/**
 * DOI-validation helper function
 *
 * @param string $doi
 *   The DOI to check
 *
 * @return bool|array
 *   FALSE if no existing object with DOI=$doi is found, or an associative 
 *   array containing information on the existing objects with that DOI:
 *   - count: The number of objects found
 *   - objects: An associative array for each key=PID containing:
 *     - namespace: The namespace of the object
 *     - incollections: An array of collections the object is a member of
 *     - #markup: A link to the object followed by the list of collections
 *   - collections: An associative array for each key=collection containing:
 *     - namespace: The namespace of the collection
 */

function lib4ridora_get_objects_by_doi($doi) {
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  module_load_include('inc', 'islandora', 'includes/utilities');
  $doisolrfield = variable_get('lib4ridora_solr_field_document_doi', 'mods_identifier_doi_mlt');
  $collectionsolrfield = variable_get('islandora_solr_member_of_collection_field', 'RELS_EXT_isMemberOfCollection_uri_ms');
  $query = $doisolrfield . ":\"" . islandora_solr_lesser_escape(trim($doi)) . "\"";
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery($query);
  $qp->solrParams['fl'] = "PID, $doisolrfield, $collectionsolrfield";
  $qp->solrParams['sort'] = "PID asc";
  $qp->executeQuery(FALSE);
  $result = array();
  $result['objects'] = array();
  $result['collections'] = array();
  $result['count'] = 0;
  if (!empty($qp->islandoraSolrResult) && !empty($qp->islandoraSolrResult['response']) && isset($qp->islandoraSolrResult['response']['numFound']) && $qp->islandoraSolrResult['response']['numFound'] > 0){
    foreach ($qp->islandoraSolrResult['response']['objects'] as $curobj) {
      $r = array();
      $pid = $curobj['PID'];
      $r['namespace'] = preg_replace('/:.*$/', '', $pid);
      $r['incollections'] = (array) preg_replace('/^.*\//', '', $curobj['solr_doc'][$collectionsolrfield]);
      foreach ($r['incollections'] as $c) {
        if (!in_array($c, $result['collections'])) {
          $collectionobject = islandora_object_load($c);
          $policy = new CollectionPolicy($collectionobject['COLLECTION_POLICY']->content);
          $models = $policy->getContentModels();
          $model = 'ir:citationCModel';
          $namespace = 'islandora';
          if (isset($models[$model])) {
            $model = $models[$model];
            $namespace = islandora_get_namespace($model['namespace']);
          }
          $result['collections'][$c] = array(
            'namespace' => $namespace,
          );
        }
      }
      $r['#markup'] = t('<a href="@url">@pid</a> (@collections)', array(
        '@url' => url($curobj['object_url']),
        '@pid' => $pid,
        '@collections' => implode(', ', $r['incollections']),
      ));
      $result['objects'][$pid] = $r;
      $result['count'] += 1;
    }
  }
  if ($result['count'] == 0) {
    return FALSE;
  }
  return $result;
}

/**
 * DOI-validator helper function
 *
 * @param string $doi
 *   A string containing the DOI of which to check the existence
 *
 * @return bool|string
 *   FALSE if DOI does not exist, the document url otherwise
 *
 * @TODO: make DOI lookup-url customisable
 */

function lib4ridora_doi_get_document_url($doi) {
  $url = 'https://doi.org'; // no trailing slash!

  $options = array('max_redirects' => 0);

  $response = drupal_http_request($url . '/' . $doi, $options);

  if (!in_array($response->code, array(200,302))) {
    return FALSE;
  }

  if (property_exists($response, 'redirect_url')) {
    return $response->redirect_url;
  }

  return FALSE;
}

/**
 * DOI-validator callback
 *
 * @param bool $ingesting
 *   To specify whether we are ingesting or editing
 *
 * @return bool|SimpleXMLElement|string
 *   FALSE if error or not found, otherwise a MODS document - if it can be
 *   retrieved - or a string containing the document url (but only if we
 *   are not ingesting)
 */

function lib4ridora_doi_validate($element, &$form_state, $form, $ingesting=FALSE) {
  if ((!$ingesting || $element['#id'] != "edit-doi") && $element['#value'] == "") {
    return FALSE; // do nothing if element is empty and we are not importing via doi
  }
  if ((!$ingesting || $element['#id'] != "edit-doi") && preg_match('/^!/', $element['#value'])) {
    $form_state['values']['identifiers']['doi'] = preg_replace('/^!/', '', $element['#value']); // remove leading '!'
    return FALSE; // do nothing if user wants to override with leading '!'
  }
  module_load_include('inc', 'islandora_doi', 'includes/utilities');
  // Check for duplicate DOIs whithin the same collection or namespace
  module_load_include('inc', 'islandora', 'includes/utilities');
  $samedoiobjs = lib4ridora_get_objects_by_doi($element['#value']);
  if ($samedoiobjs !== FALSE) {
    $thispid = FALSE;
    $thisnamespace = FALSE;
    if (!$ingesting && isset($form_state['pid'])) {
      $thispid = $form_state['pid'];
      $thisnamespace = preg_replace('/:.*$/', '', $thispid);
      if (array_key_exists($thispid, $samedoiobjs['objects'])) {
        unset($samedoiobjs['objects'][$thispid]);
        $samedoiobjs['count'] -= 1;
        if ($samedoiobjs['count'] == 0) {
          $samedoiobjs = FALSE;
        }
      }
    }
  }
  if ($samedoiobjs !== FALSE) {
    $targetcollections = array();
    if (!$ingesting) {
      if ($thispid !== FALSE) {
        $thisobj = islandora_object_load($thispid);
        $targetcollections = $thisobj->getParents();
      }
    }
    else {
      $storage = islandora_ingest_form_get_shared_storage($form_state);
      $targetcollections = (array) $storage['parent'];
    }
    foreach ($samedoiobjs['collections'] as $cname => $c) {
      $samedoiobjs['collections'][$cname]['issibling'] = FALSE;
      $samedoiobjs['collections'][$cname]['istarget'] = FALSE;
    }
    foreach ($targetcollections as $targetcollection) {
      $collectionobject = islandora_object_load($targetcollection);
      $policy = new CollectionPolicy($collectionobject['COLLECTION_POLICY']->content);
      $models = $policy->getContentModels();
      $model = 'ir:citationCModel';
      $namespace = 'islandora';
      if (isset($models[$model])) {
        $model = $models[$model];
        $namespace = islandora_get_namespace($model['namespace']);
      }
      foreach ($samedoiobjs['collections'] as $cname => $c) {
        if ($c['namespace'] == $namespace || $c['namespace'] == 'islandora' || ($thisnamespace && ($c['namespace'] == $thisnamespace) || $thisnamespace == 'islandora')) {
          $samedoiobjs['collections'][$cname]['issibling'] = TRUE;
        }
        if ($cname == $targetcollection) {
          $samedoiobjs['collections'][$cname]['istarget'] = TRUE;
        }
      }
    }
    if ($thisnamespace === FALSE && count($targetcollections) == 1) {
      $thisnamespace = $namespace; // if ingesting, we set the current namespace to the collection's namespace
    }
    foreach ($samedoiobjs['objects'] as $pid => $obj) {
      $samedoiobjs['objects'][$pid]['issibling'] = ($obj['namespace'] == 'islandora' ? TRUE : FALSE);
      if ($thisnamespace && $obj['namespace'] == $thisnamespace) {
        $samedoiobjs['objects'][$pid]['issibling'] = TRUE;
      }
      $samedoiobjs['objects'][$pid]['sametarget'] = FALSE;
      foreach ($obj['incollections'] as $c) {
        if ($samedoiobjs['collections'][$c]['issibling']) {
          $samedoiobjs['objects'][$pid]['issibling'] = TRUE;
        }
        if ($samedoiobjs['collections'][$c]['istarget']) {
          $samedoiobjs['objects'][$pid]['sametarget'] = TRUE;
        }
      }
    }
    foreach ($samedoiobjs['objects'] as $pid => $obj) {
      if (!$obj['issibling']) {
        unset($samedoiobjs['objects'][$pid]);
        $samedoiobjs['count'] -= 1;
      }
    }
    $many = ($samedoiobjs['count'] > 1);
    $allinothercollection = TRUE;
    $objlinklist = array();
    foreach ($samedoiobjs['objects'] as $obj) {
      array_push($objlinklist, $obj['#markup']);
      if ($obj['sametarget']) {
        $allinothercollection = FALSE;
      }
    }
    if ($samedoiobjs['count'] > 0) {
      form_error($element, t(($many ? 'Several' : 'An') . ' object' . ($many ? 's' : '') . ' with DOI "@value" already exist' . ($many ? '' : 's') . ': ' . implode(', ', $objlinklist) . '.' . ($allinothercollection ? ' Perhaps you could share ' . ($many ? 'an appropriate object' : 'it') . ' with the current collection' . (count($targetcollections) > 1 ? 's' : '') . ' "@collection"' : ''), array(
        '@value' => trim($element['#value']),
        '@collection' => implode('", "', $targetcollections),
      )));
    }
  }
  // check for valid doi
  $doc_url = lib4ridora_doi_get_document_url(trim($element['#value']));
  $mods = islandora_doi_get_mods(trim($element['#value']));
  if ($mods === FALSE) {
    if ($element['#value'] == "") {
      form_error($element, t('@title is required.', array(
        '@title' => $element['#title'],
      )));
    }
    elseif (trim($element['#value']) == "") {
      form_error($element, t('"@value" is not a DOI.', array(
        '@value' => $element['#value'],
      )));
    }
    elseif ($doc_url !== FALSE) {
      if ($element['#id'] == "edit-doi") {
        form_error($element, t('"@value" does appear to be a valid DOI, but no MODS could be retrieved, so there could be an issue with the DOI configuration.', array(
          '@value' => $element['#value'],
        )));
      }
    }
    else {
      form_error($element, t('"@value" does not appear to be a valid DOI.', array(
        '@value' => $element['#value'],
      )));
    }
    return (!$ingesting) ? $doc_url : FALSE;
  }
  return $mods;
}
