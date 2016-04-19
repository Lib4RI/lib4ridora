<?php

/**
 * @file
 * Utility functions for the module.
 */

/**
 * Retrieve the journal identified via an ISSSN or EISSN.
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
  $e_issn_field = variable_get('lib4ridora_issn_solr_field', 'mods_identifier_e-issn_s');
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

  return $factor !== FALSE ? array($factor, $factor_year) : FALSE;
}

/**
 * Retrieve PDFs.
 *
 * @param AbstractObject $object
 *   An ABstractObject representing an object within Fedora.
 *
 * @return array
 *   The listing of the PDFs on the object.
 */
function lib4ridora_retrieve_filtered_pdfs(AbstractObject $object) {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
  $dses = iterator_to_array($object);
  $dses = array_filter($dses, 'lib4ridora_multi_embargo_pdf_filter');
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
        'id' => $xpath->evaluate('string(mods:nameIdentifier[@type="authorId"]/text())', $person),
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
  $subtypes = lib4ridora_determine_subtypes($object);
  $first = reset($subtypes);
  foreach (lib4ridora_citation_form_subtypes() as $type => $info) {
    if (in_array($first['name'], $info['genres'])) {
      $first['form'] = $info['name'];
    }
  }
  return $first ? $first['form'] : NULL;
}

/**
 * Retrieve the author string from a PID.
 */
function lib4ridora_get_author_from_pid($pid) {
  $author_object = islandora_object_load($pid);
  if ($author_object && isset($author_object['MADS'])) {
    $mads_dom = new DOMDocument();
    $mads_dom->loadXML($author_object['MADS']->content);
    $mads_xpath = new DOMXpath($mads_dom);
    $mads_xpath->registerNamespace('mads', 'http://www.loc.gov/mads/v2');

    $elements = $mads_xpath->query('/mads:mads/mads:authority/mads:name[@type="personal"]');
    if ($elements->length) {
      $author_node = $elements->item(0);
      $author_array = array();
      foreach ($author_node->childNodes as $node) {
        $author_array[] = $node->nodeValue;
      }
      return implode(' ', $author_array);
    }
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
