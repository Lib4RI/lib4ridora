<?php

/**
 * @file
 * Utility functions for the module.
 */

/**
 * Retrieve the *first* journal found, identified via an ISSN or EISSN.
 *
 * @param string $identifier
 *   An ISSN or EISSN.
 *
 * @return mixed
 *   The object that is the journal or FALSE if none found.
 */
function lib4ridora_get_journal($identifier) {
  $issn_field = variable_get('lib4ridora_issn_solr_field', 'mods_identifier_issn_s');
  $e_issn_field = variable_get('lib4ridora_e_issn_solr_field', 'mods_identifier_e-issn_s');
  if ( !strpos($identifier,'-') ) {
    $identifier = substr(ltrim($identifier),0,4) . '-' . substr(rtrim($identifier),-4);  // long-dash work-around (possibly in CSV, DORA-value assumed to be OK)
  }
  $solr = new IslandoraSolrQueryProcessor();
  $solr->buildQuery("PID:journals%5c%3a* AND ({$issn_field}:\"$identifier\" OR {$e_issn_field}:\"$identifier\")");
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
	return islandora_object_load($journal);
  }
  return FALSE;
}

/**
 * Returns an array with PIDs of journals identified via an ISSN or EISSN.
 *
 * @param string $identifier
 *   An ISSN or EISSN.
 *
 * @return array
 *   Array of Journal PIDs, optionally as object.
 */
function lib4ridora_get_journals($identifier, $asObject = FALSE) {
  $issn_field = variable_get('lib4ridora_issn_solr_field', 'mods_identifier_issn_s');
  $e_issn_field = variable_get('lib4ridora_e_issn_solr_field', 'mods_identifier_e-issn_s');
  if ( !strpos($identifier,'-') ) {
    $identifier = substr(ltrim($identifier),0,4) . '-' . substr(rtrim($identifier),-4);  // long-dash work-around (possibly in CSV, DORA-value assumed to be OK)
  }
  $solr = new IslandoraSolrQueryProcessor();
  $solr->buildQuery("PID:journals%5c%3a* AND ({$issn_field}:\"$identifier\" OR {$e_issn_field}:\"$identifier\")");
  $solr->solrLimit = 987654321;
  $solr->solrParams['fl'] = implode(
    ',',
    array(
      'PID',
      $issn_field,
      $e_issn_field,
    )
  );
  $solr->executeQuery(FALSE);
  $journalAry = array();
  if ( isset($solr->islandoraSolrResult['response']['objects']) ) {
    foreach( $solr->islandoraSolrResult['response']['objects'] as $solrItem ) {
      $journal = $solrItem['solr_doc']['PID'];
      $journalAry[] = ( !$asObject ? $journal : islandora_object_load($journal) );
    }
  }
  return $journalAry;
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
    $factors[$year] = $factor;
    $factor_datastream->content = json_encode($factors);
  } else {
    $factor_datastream = $journal->constructDatastream(LIB4RIDORA_FACTOR_DSID, 'M');
    $factor_datastream->label = 'Journal Impact Factors';
    $factor_datastream->mimetype = 'application/json';
    $factors = array( $year => $factor );
    $factor_datastream->content = json_encode($factors);
    $journal->ingestDatastream($factor_datastream);
  }
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
					$dataAry[$dsPdfName] = array(
						'document_version' => strval($dElem->islandora_lib4ridora_multi_embargo_document_version),
						'availability' => strval($dElem->islandora_lib4ridora_multi_embargo_availability),
					);
					unset($dsPdfAry[$dsPdfIdx]);
					break;
				}
			}
		}
	}
	return $dataAry;		// array( 'PDF' => array( 'document_version' => "Published", 'availability' => "intranet"), 'PDF2' => array...
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
	$orderAry = array_map('trim',explode(',', variable_get('lib4ridora_multi_pdf_link_order', /* Published, Accepted, Supplemental, ... */ $default_list ) ));

	if ( !empty(implode('',$orderAry)) && @isset($object['RELS-INT']) ) {		// safety check
		$relsIntCode = $object['RELS-INT']->content;
		if ( !empty($relsIntCode) ) {
			$dsPdfAry = lib4ridora_get_embargo_data( $relsIntCode, array_keys($dses) );	// array_keys() for array( 0 => 'PDF', 1 => "PDF_PDF-A", 2 => "PDF...
			if ( sizeof($dses) == sizeof($dsPdfAry) ) {		// for safety
				$rankAry = array();	// for all index of PDFs that have a version label that is inside our order list
				while( sizeof($orderAry) ) {
					$wanted = array_shift($orderAry);
					foreach( $dsPdfAry as $pIdx => $pAry ) {			
						// $pIdx may be 'PDF3' or 'PDF2_PDF-A', etc.
						// $pAry may be array( 'document_version' => "Erratum", 'availability' => "intranet" )
						if ( gettype($pAry) == "array" && @stripos($pAry['document_version'],$wanted) === 0 ) {
							$rankAry[$pIdx] = $pAry;
					//		break;		// better do not, there may be more than one PDF all with the same document version.
						}
					}
				}
				foreach( $dsPdfAry as $pIdx => $pAry ) {		// add PDFs possibly missed because they matched no item in the order list
					if ( @!isset($rankAry[$pIdx]) ) { $rankAry[$pIdx] = $pAry; }
				}

				if ( sizeof($dses) == sizeof($rankAry) ) {	// for safety
					$dsesAry = array();
					foreach( $rankAry as $pIdx => $pAry ) { $dsesAry[$pIdx] = $dses[$pIdx]; }
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
 * Extract funder info from an object.
 *
 * @param AbstractObject $object
 *   An object from which to extract a list of authors.
 *
 * @return array
 *   Similar to lib4ridora_get_author_info()
 *
 * Not checked/parsed currently:
 *   <funderIdentifier funderIdentifierType="Crossref_Funder"/>
 */
function lib4ridora_get_funder_info(AbstractObject $object) {
  $dom = new DOMDocument();
  $dom->loadXML($object['MODS']->content);
  $xpath = new DOMXPath($dom);
  $xpath->registerNamespace('mods', 'http://loc.gov/mods/v3');

  $funders = array();
  foreach ($xpath->query('//mods:mods/mods:extension/mods:fundingReferences/mods:fundingReference') as $funder) {
    $info = array(
      'fundingstream' => array(
        'stream' => $xpath->evaluate('normalize-space(mods:fundingStream)', $funder),
      ),
      'fundername' => array(
        'name' => $xpath->evaluate('normalize-space(mods:funderName)', $funder),
      ),
      'awardtitle' => array(
        'title' => $xpath->evaluate('normalize-space(mods:awardTitle)', $funder),
      ),
      'awardnumber' => array(
        'number' => $xpath->evaluate('string(mods:awardNumber/text())', $funder),
      ),
    );
    if ( !@empty($info['fundingstream']['stream']) || !@empty($info['fundername']['name']) ) {
      $funders[] = $info;
    }
  }
  return $funders;
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

/*
 * function to get information of metadata fields as configured Solr
 *
 * based on islandora_solr_get_fields
 */
function lib4ridora_get_metadata_fields($field_as_key = true) {
  // Fields query.
  $query = db_select('islandora_solr_metadata_fields', 'ismf');
  $query->fields('ismf');
  $query->orderBy('ismf.weight', 'ASC');
  try {
    $result = $query->execute();
    $records = $result->fetchAll(PDO::FETCH_ASSOC);
  }
  catch (Exception $e) {
    if ($e->errorInfo[0] == '42S02') {
      drupal_set_message(t('Islandora Solr metadata fields table not found. Try running <a href="@update_url">update.php</a>.', array('@update_url' => url('update.php'))), 'error');
    }
    else {
      drupal_set_message($e->getMessage(), 'error');
    }
    return array();
  }
  // Unserialize Solr field settings blob.
  foreach ($records as &$value) {
    $value['data'] = unserialize($value['data']);
  }
  // Fields as keys.
  if ( $field_as_key ) {
    $original = $records;
    $records = array();
    foreach ($original as $field) {
      $records[$field['solr_field']] = $field;
    }
  }
  return $records;
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

/*
 * function to return an array with data of the PDF datastream that is Open Access.
 * By default the array-*key* with the ID of this datastream is called 'dsid'.
 *
 * If available the (first) published version will be favored if available, otherwise the 
 * (first) accepted version is taken - supposed it is public or the embargo date expired.
 */
function lib4ridora_get_open_access_pdf(AbstractObject $object, $keyDsIdPdf = 'dsid') {
	module_load_include('inc', 'lib4ridora', 'includes/pdf-upload.form');
	$pdfInfoAry = @lib4ridora_assemble_pdf_info($object);
	$pdfOA = array();
	foreach( $pdfInfoAry as $pdfDsId => $pInfoAry ) {
		/*
			"PDF2" => array(4) {
				["availability"]=>
				string(4) "date"
				["embargo_date"]=>
				array(3) {
				  ["year"]=>
				  int(2019)
				  ["month"]=>
				  int(7)
				  ["day"]=>
				  int(26)
				}
				["document_version"]=>
				string(16) "accepted version"
				["use_permission"]=>
				string(11) "CC BY-NC-ND"
			}
		*/
		$docVer = $pInfoAry['document_version'];
		if ( stripos($docVer,'published') !== 0 && stripos($docVer,'accepted') !== 0 ) { continue; }

		if ( !preg_match('/^PDF\d*$/i',$pdfDsId) ) { /* for safety, if 'assemble filter' fails */ continue; }

		$isOA = false;
		if ( stripos($pInfoAry['availability'],'public') === 0 ) {
			$isOA = true;
		} elseif ( stripos($pInfoAry['availability'],'date') === 0 ) {
			date_default_timezone_set('GMT');
			$edAry = $pInfoAry['embargo_date'];
			$edEnd = @strtotime($edAry['year'].'-'.$edAry['month'].'-'.$edAry['day']);
			$isOA = ( $edEnd && $edEnd < time() );
		}
		if ( !$isOA ) { continue; }
		
		if ( stripos($docVer,'published') === 0 ) {
			$pdfOA = $pInfoAry;
			$pdfOA['pid'] = $pdfDsId;
			$pdfOA[$keyDsIdPdf] = $pdfDsId;
			return $pdfOA;	// can't get any better
		}
		if ( empty($pdfOA) ) {
			$pdfOA = $pInfoAry;
			$pdfOA['pid'] = $pdfDsId;
			$pdfOA[$keyDsIdPdf] = $pdfDsId;
		}
	}
	return $pdfOA;
}

/*
 * Try to detect if the user is a bot. Partially uses functions from from other modules if availalbe.
 * 
 * Hard-code list is based on CitCount_IsWebRobot() + lib4ri_publication_feedback_form_is_bot
 */
function lib4ridora_user_is_bot($user_agent = '', $demand_language = FALSE, $use_session = FALSE) {
	
	if ( $use_session && !empty($_SESSION['lib4ridora_user_is_bot']) ) {
		return ( intval($_SESSION['lib4ridora_user_is_bot']) > 0 );			// +1 for bot, -1 if not
	}
	if ( $demand_language && @empty(trim($_SERVER['HTTP_ACCEPT_LANGUAGE'])) ) {
		if ( $use_session ) { $_SESSION['lib4ridora_user_is_bot'] = 1; }
		return TRUE;	// considered to be a bot
	}
	if ( empty($user_agent) ) {
		if ( !( $user_agent = @trim($_SERVER['HTTP_USER_AGENT']) ) ) {
			if ( $use_session ) { $_SESSION['lib4ridora_user_is_bot'] = 1; }
			return TRUE;	// with an empty user agent string we will consider the user as a bot
		}
	}
	// If available use the check in islandora_usage_stats, see islandora_usage_stats/includes/utilities.inc
	// see for example: https://www.dora.lib4ri.ch/eawag/admin/islandora/tools/islandora_usage_stats
	$bot_regex = trim(variable_get('islandora_usage_stats_exclude_bots_regex', '/bot|rambler|spider|crawl|slurp|curl|^$/i'));
	if ( !( module_exists('islandora_usage_stats') && variable_get('islandora_usage_stats_exclude_bots', '1') ) || strlen($bot_regex) < 10 ) {
		// let's use a hard-coded list then, patially however expandable
		$botAry = array(
			'Bot,Search,Crawl,Spider,Scan,Scout,Archiv,Index,Ads,Job,SEO', 			/* generic */
			variable_get('lib4ridora_bot_list_addon',''),
			'curl,wget,typo3,java,python,rambler,zoom', 		/* generic/tools */
			'Google',		/* assumed it's a fake. Note that Google's Chrome Browser doesn't have 'Google' inside its user-agent string */
			'Semrush,BingPreview',
		/*	'BingBot,MSNBot',	*/
		/*	'Gigabot,VoilaBot',	*/
		/*	'AdIdxBot',		*/
			'Baidu,Yahoo,Slurp,Yandex,YaDirect',
			'DuckDuck,AOLbuild,Teoma,Jeeves',		/* misc custom commercial bots/search-engines */
			'HTTrack',			/* kwnow archivers */
		);
		$bot_regex = '/' . implode('|',array_map('trim',explode(',',implode(',',$botAry)))) . '|^$/i';
	}
	if ( preg_match($bot_regex,	$user_agent) === 1 ) {
		if ( $use_session ) { $_SESSION['lib4ridora_user_is_bot'] = 1; }
		return TRUE;
	}
	if ( $use_session ) { $_SESSION['lib4ridora_user_is_bot'] = -1; }
	return FALSE;
}

/*
 * function to prevent bots from running searches.
 *
 * Goal: Task1: Stop browsing DORA with a unwanted user-agent string (Eawag firewall cannot handle this - Jour Fixe 2021-08-27)
 *       Task2: Do not allow (known) bots to run batches (specially CSV/Excel exports).
 * 
 * Caution: Take care that out own/Lib4RI's batch processes are not afffected!
 *          Note too that within Drupal we have no $_SERVER are, global like $base_path or $user are there however.
 * Partially based on: /var/www/html/includes/batch.inc
 * Future: Should be removed again as soon we have a better/external possibilty to filter out unwanted visitors.
 *
 * This covers batch-triggering links like:
 * https://www.dora.lib4ri.ch/empa/islandora/search/dc.title%3Acarbon?islandora_solr_search_navigation=1&solr_profile=lib4ri_csv
 * https://www.dora.lib4ri.ch/eawag/batch?op=start&id=78228
 *
 * Variable assignments:
 * sudo drush -r /var/www/html -y @sites php-eval 'variable_set("lib4ridora_omit_user_agent","seekport")'
 * sudo drush -r /var/www/html -y @sites php-eval 'variable_set("lib4ridora_omit_user_agent",array("Aeekport","SEOcrawl","sciSearch"))'
 *
 * However (as tested/experienced):
 * - For https://www.dora.lib4ri.ch/eawag/batch?op=start&id=78228 batch_alter is applyed to late!(?) Because in the returning
 *   web page (as you may get it with Wget) it's full HTML, with recent additions, and the 'No active batch' error message
 *   as issued by /var/www/html/includes/batch.inc file - lucky hook_init can interrupt it if needed.
 * - For https://www.dora.lib4ri.ch/empa/islandora/search/dc.title%3Acarbon?islandora_solr_search_navigation=1&solr_profile=lib4ri_csv
 *   as found 'underneath' the CSV/Ecxel icon the hook_init does not seem to be triggered (always) but hook_bach_alter() can interrupt it.
 */
function lib4ridora_access_rejection( $batch = NULL ) {

  $ipAry = array(
    @strval( $_SERVER['HTTP_CLIENT_IP'] ),
    @strval( $_SERVER['HTTP_X_REAL_IP'] ),
    @strval( ip_address() /* by Drupal */ ),
  );
  $ipAry = array_filter($ipAry);	// separate row to avoid a 'strict' warning
  $ipClient = array_shift( /* array_filter($ipAry) */ $ipAry );

  if ( lib4ridora_is_white_listed($ipClient) ) {
	return;		// never reject with an explicit IP
  }
  if ( variable_get('lib4ridora_bot_intranet_ok',true) && lib4ridora_check_ip($ipClient) ) {
	return;		// never reject a bot from inside Lib4RI network
  }

  $isBot = 0;    // bot detection via islandora_usage_stats, needs a user agent string (fails if empty)
  $uaOk = 0;    // for the result of user-agent string check, 1 for ok, -1 for rejectable.

  $uaList = array_filter( array_map('trim', explode(',',strtr(variable_get('lib4ridora_bot_user_agent_list'),"|;\n",",,,"))) );
  $uaString = @trim($_SERVER['HTTP_USER_AGENT']);
  if ( $uaString && $uaList ) {
	// uaList: expected to have a browser's user agent string, or an array with multiple of them.
    foreach( ( is_array($uaList) ? array_map('trim',$uaList) : array(trim($uaList)) ) as $uaTerm ) {
      $uaTmp = ( substr($uaTerm,0,1) == '+' ) ? 1 : -1;
      if ( stripos($uaString,ltrim($uaTern,"+- \n\r\t\v\0")) !== false ) {
        $uaOk = $uaTmp;
        break;
      }
    }
    $isBot = lib4ridora_user_is_bot($uaString,variable_get('lib4ridora_bot_needs_language',false),true);
  }

  if ( $uaOk < 0 ) { // Task 1: user-agent check:
    global $base_path;
    if ( !drupal_is_cli() || @!empty($_SERVER['DOCUMENT_ROOT']) ) {	// see: https://api.drupal.org/api/drupal/includes%21bootstrap.inc/function/drupal_is_cli/7.x
      if ( empty($uaString) || $uaOk < 0 ) {
        $tmpPath = '/tmp/browser-user-agent.denied' . strtr($base_path,'/','.') . 'txt';
        $ipAry = array( date("Y-m-d.H-i-s"), $_SERVER['REMOTE_ADDR'], $_SERVER['HTTP_X_REAL_IP'], $_SERVER['HTTP_X_FORWARDED_FOR'], $_SERVER['REQUEST_URI'], $_SERVER['HTTP_USER_AGENT'] );
        file_put_contents( $tmpPath, @strval(file_get_contents($tmpPath)) . implode(' | ',$ipAry) . "\r\n" );
        header("HTTP/1.0 503 Service Unavailable");
        echo("503 Service Unavailable");
        exit;
      }
    }
  }
  // Task 2: bot rejection:
  if ( variable_get('lib4ridora_no_batch_for_bot',true) ) {	// TRUE by default - Jour Fixe 2021-10-21
    // Special case, catch obsolete links like: https://www.dora.lib4ri.ch/eawag/batch?op=start&id=78228
    if ( isset($_REQUEST['id']) && ( @empty($_SERVER['REQUEST_URI']) || strpos(strtok($_SERVER['REQUEST_URI'].'?','?'),'/batch') ) ) {
      if ( !function_exists('batch_load') ) {
        include_once('/var/www/html/includes/batch.inc');
      }
      if ( !$batch ) {
        $batch = &batch_get();
      }
      // Retrieve the current state of the batch. However this is for explicit batch links, so bascially act always.
      if ( !$batch || ( function_exists('batch_load') && !( $batch = batch_load($_REQUEST['id']) ) ) ) {
        if ( $isBot < 0 ) {
          header("HTTP/1.0 404 Not Found");		// 404 similar as in Drupal 9: https://www.drupal.org/project/drupal/issues/1986330
          echo("404 Not Found");
          exit;
        }
      }
    }
    // handle general case via name check of interface, for drush command and 'rather normal' usage links like:
    // https://www.dora.lib4ri.ch/empa/islandora/search/dc.title%3Acarbon?islandora_solr_search_navigation=1&solr_profile=lib4ri_csv
    // It's a matter of opinion/purpose if a batch check is useful here, but if not there must be a location check!
    // About drupal_is_cli() see https://api.drupal.org/api/drupal/includes%21bootstrap.inc/function/drupal_is_cli/7.x
    if ( !drupal_is_cli() && ( @empty($_SERVER['REQUEST_URI']) || strpos(strtok($_SERVER['REQUEST_URI'].'?','?'),'/islandora/search') ) ) {
      if ( !$batch ) {
        $batch = &batch_get();
      }
      if ( $batch && $isBot < 0 ) {
        header("HTTP/1.0 404 Not Found");		// 404 similar as in Drupal 9: https://www.drupal.org/project/drupal/issues/1986330
        echo("404 Not Found");
        exit;
      }
    }
  }
}

function lib4ridora_is_white_listed($ip) {
    $white_list = array_filter( array_map('trim', explode(',',strtr(variable_get('lib4ridora_bot_white_list'),"|;\n",",,,"))) );
    
    $ips = [];
    $ranges = [];
    
    foreach ($white_list as $entry){
        $e = explode(';', $entry);
        
        if (sizeof($e) == 1) {
            array_push($ips, $e[0]);
        }
        elseif (sizeof($e) == 2){
            $e[0] = trim($e[0]," \n\r\t\v\0[]");
            $e[1] = trim($e[1]," \n\r\t\v\0[]");
            
            array_push($ranges, $e);
        }        
    }
    
    if (in_array($ip, $ips)){
        return TRUE;
    }
    
    foreach ($ranges as $range){
        $ipl = ip2long($ip);
        $low = ip2long($range[0]);
        $high = ip2long($range[1]);
        
        if($low <= $ipl && $ipl <= $high){
            return TRUE;
        }
    }
    
    return FALSE;
}
