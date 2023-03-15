<?php

/**
 * @file
 * Utiilty functions for Lib4Ri's altered advanced search form.
 */

/**
 * Replacement for ucwords to manage exceptions
 *
 * @return string
 *   string with the first character of each word capitalized if not 
 *   in the array $exceptions
 */
function lib4ridora_org_unit_ucwordss($str) {
    $exceptList = variable_get('lib4ridora_solr_field_org_terms_1on1', 'or, and, for, of, in, at');
    $exceptions = explode( ",", $exceptList );
    foreach ($exceptions as $idx => $term) {
        $exceptions[$idx] = trim($term);
    }

    $words = explode(" ", $str);
    $out = "";
    foreach ($words as $word) {
        (in_array($word, $exceptions)) ? $out .= $word." " : $out .= ucfirst($word)." ";
    }
    return rtrim($out);
}

/**
 * Queries solr to get all publication types.
 *
 * @return array
 *   Associative array of publication types formatted for #options
 */
function lib4ridora_get_all_publication_types() {
  $field = variable_get('lib4ridora_solr_field_publication_type', 'mods_genre_ms');
  $facet_fields = array($field);
  $facets = lib4ridora_facet_query($facet_fields);

  // If there's results, construct an array where the keys are the genres
  // as in Solr, and the values are formatted to be labels.
  if (isset($facets[$field]) && $facets[$field]) {
    return drupal_map_assoc(array_keys($facets[$field]), 'lib4ridora_org_unit_ucwordss');
  }

  // Default to returning an empty array.
  return array();
}

/**
 * Queries solr to get all organizational units.
 *
 * @return array
 *   Associative array of organizational units formatted for #options
 */
function lib4ridora_get_all_organizational_units() {
  $facet_fields = lib4ridora_parse_affiliation_fields();
  $facets = lib4ridora_facet_query($facet_fields);

  $org_units = array();

  // If there's results, construct an array where the keys are affiliations
  // as in Solr, and the values are formatted to be labels.
  foreach ($facets as $facet) {
    if (empty($facet)) {
      continue;
    }
    $keys = array_keys($facet);
    $org_units = array_merge($org_units, drupal_map_assoc($keys, 'lib4ridora_org_unit_ucwordss'));
  }

  // Sort the array alphanumerically (ignoring lower/upper case):
  $sortAryTmp = array_map( 'strtolower', $org_units );
  array_multisort( $sortAryTmp, SORT_ASC, SORT_STRING, $org_units );

  // Return 'all' alongside whatever else.
  return array("*" => t("All")) + $org_units;
}

/**
 * Pulls solr affiliation field names out of configuration.
 *
 * @return array
 *   Array of affilation field names.
 */
function lib4ridora_parse_affiliation_fields() {
  $comma_separated_list = variable_get('lib4ridora_solr_field_affiliations', 'mods_name_personal_affiliation_ms,mods_name_corporate_affiliation_ms,mods_name_conference_affiliation_ms');
  $exploded = explode(",", $comma_separated_list);
  return array_map("lib4ridora_trim_and_solr_escape", $exploded);
}

/**
 * Trims and solr escapes a string.
 *
 * @param string $str
 *   String to trim and escape.
 *
 * @return string
 *   Trimmed and escaped string, suitable for a solr query.
 */
function lib4ridora_trim_and_solr_escape($str) {
  module_load_include("inc", "islandora_solr", "includes/utilities");
  return islandora_solr_lesser_escape(trim($str,"'\" \n\r\t\v\0"));
}

/**
 * Issues a facet query to solr for the provided fields.
 *
 * @param array $facet_fields
 *   Array of fields to generate facets.
 *
 * @return array
 *   Associative array where key is the facet field and the value is the count.
 */
function lib4ridora_facet_query(array $facet_fields, $query = '') {
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery( ( empty($query) ? '*:*' : $query ) );
  $qp->solrParams['fl'] = "PID";
  $qp->solrParams['facet.mincount'] = 1;
  $qp->solrParams['facet.field'] = $facet_fields;
  $qp->solrParams['facet'] = 'true';
  $qp->solrParams['facet.limit'] = 98765431;
  $qp->solrLimit = 0;
  $qp->executeQuery();
  return $qp->islandoraSolrResult['facet_counts']['facet_fields'];
}

/**
 * Builds a solr filter query based on publication or reporting year.
 *
 * @param array $form_state
 *   Form state
 *
 * @return string
 *   Solr filter query
 */
function lib4ridora_construct_year_filter($form_state) {
  // Get the values out of the form.
  $year_type = $form_state['values']['year']['select'];
  $to = empty($form_state['values']['year']['to']) ? "*" : $form_state['values']['year']['to'];
  $from = empty($form_state['values']['year']['from']) ? "*" : $form_state['values']['year']['from'];
  // If nothing was entered in the date fields, exit early.
  if ($to == "*" && $from == "*") {
    return "";
  }

  // Convert to proper format for solr.
  if ($to != '*') {
    $to_date = new DateTime();
    $to = date_format($to_date->createFromFormat('Y/m/d/G:i:s', "$to/12/31/23:59:59"), 'Y-m-d\TH:i:s\Z');
  }
  if ($from != '*') {
    $from_date = new DateTime();
    $from = date_format($from_date->createFromFormat('Y/m/d/G:i:s', "$from/01/01/0:00:00"), 'Y-m-d\TH:i:s\Z');
  }

  // Return fq string.
  module_load_include("inc", "islandora_solr", "includes/utilities");
  switch ($year_type) {
    case "publication year":
      $publication_year_field = islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_publication_year', 'mods_originInfo_encoding_w3cdtf_keyDate_yes_dateIssued_dt'));
      return "$publication_year_field:[$from TO $to]";

    case "reporting year":
      $reporting_year_field = islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_reporting_year', 'mods_originInfo_encoding_w3cdtf_type_reporting year_dateOther_dt'));
      return "$reporting_year_field:[$from TO $to]";

    default:
      return "";
  }
}

/**
 * Builds a solr filter query based on publication type.
 *
 * @param array $form_state
 *   Form state
 *
 * @return string
 *   Solr filter query
 */
function lib4ridora_construct_publication_type_filter($form_state) {
  // Collect the selected values.
  $genres = array();
  foreach ($form_state['values']['pub_type'] as $genre) {
    if ($genre) {
      $genres[] = $genre;
    }
  }

  // Exit early if no check box was selected.
  if (empty($genres)) {
    return "";
  }

  module_load_include("inc", "islandora_solr", "includes/utilities");
  $publication_type_field = islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_publication_type', 'mods_genre_ms'));

  $field_names = array_fill(0, count($genres), $publication_type_field);

  $filters = array_map("lib4ridora_construct_solr_statement", $field_names, $genres);

  return lib4ridora_construct_filter_string_from_array($filters);
}

/**
 * Combines a field name / value pair into a solr query statement.
 *
 * @param string $field_name
 *   Solr field name
 * @param string $field_value
 *   Solr field value
 *
 * @return string
 *   Solr query statement
 */
function lib4ridora_construct_solr_statement($field_name, $field_value) {
  $field_value = trim($field_value,"\"' \n\r\t\v\x00");
  $quote = ( empty(rtrim($field_value,'*')) || !preg_match('/\s+/',urldecode($field_value)) ) ? '' : '"';
  return ( $field_name . ':' . $quote . $field_value . $quote );
}

/**
 * Constructs a sequence of solr statements combined with OR.
 *
 * @param array $filters
 *   Array of solr query statements to combine into one big filter.
 *
 * @return string
 *   A single filter string that is the union of all provided filters.
 */
function lib4ridora_construct_filter_string_from_array($filters) {
  $total = count($filters);

  switch ($total) {
    case 0:
      return "";

    case 1:
      return reset($filters);

    default:
      return implode($filters, " OR ");
  }
}

/**
 * Builds a filter query based on organizational unit (e.g., affiliations).
 *
 * @param array $form_state
 *   Form state.
 *
 * @return string
 *   Solr filter query.
 */
function lib4ridora_construct_organization_unit_filter(&$form_state) {
  // Get the organizational unit value from the dropdown.
  $org_unit = $form_state['values']['org_unit'];

  if ($org_unit == "*") {
    return "";
  }

  // List of all possible affiliation fields.
  // Parsed from configurations as a comma separated list.
  $affiliation_fields = lib4ridora_parse_affiliation_fields();

  $org_units = array_fill(0, count($affiliation_fields), $org_unit);

  $filters = array_map("lib4ridora_construct_solr_statement", $affiliation_fields, $org_units);

  return lib4ridora_construct_filter_string_from_array($filters);
}

/**
 * Builds a filter query based on peer review status.
 *
 * @param array $form_state
 *   Form state.
 *
 * @return string
 *   Solr filter query.
 */
function lib4ridora_construct_peer_reviewed_filter(&$form_state) {
  module_load_include("inc", "islandora_solr", "includes/utilities");

  // Get field name out of configuration.
  $peer_review_field = islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_peer_review', 'mods_note_peer review_ms'));

  $filters = array();
  if ($form_state['values']['peer_review']['yes']) {
    $filters[] = "$peer_review_field:Yes";
  }
  if ($form_state['values']['peer_review']['no']) {
    $filters[] = "$peer_review_field:No";
  }
  if ($form_state['values']['peer_review']['not specified']) {
	if ( sizeof($filters) ) {
      $filters[] = "(*:* NOT $peer_review_field:*)"; 	// safer in conjunction with other filters
	} else {
      $filters[] = '-' . $peer_review_field . ':*';	// easier to interpret+display for the 'Active Filters' function
    }
  }

  return lib4ridora_construct_filter_string_from_array($filters);
}

/**
 * Builds a filter query based on a document's full text.
 *
 * @param array $form_state
 *   Form state.
 *
 * @return string
 *   Solr filter query.
 */
function lib4ridora_construct_full_text_filter(&$form_state) {
  module_load_include("inc", "islandora_solr", "includes/utilities");

  // Deal with the advanced search full text same as facet functionality.
  $availability_field = islandora_solr_lesser_escape(variable_get('lib4ridora_solr_field_document_availability', 'RELS_EXT_fullText_literal_ms'));

  $open = islandora_solr_lesser_escape('Open Access');
  $restricted = islandora_solr_lesser_escape('Restricted');
  $no_full_text = islandora_solr_lesser_escape('No Full Text');
  $embargo_restricted = islandora_solr_lesser_escape( variable_get('lib4ridora_term_embargo_restricted', 'Restricted (Embargo)') );

  $filters = array();

  if ($form_state['values']['full_text']['restricted']) {
    $filters[] = $availability_field . " : " . $restricted;
  }

  if ($form_state['values']['full_text']['open access']) {
    $filters[] = $availability_field . " : " . $open;
  }

  if ($form_state['values']['full_text']['no full text']) {
    $filters[] = $availability_field . " : " . $no_full_text;
  }

  if ($form_state['values']['full_text']['embargo-restricted']) {
    $filters[] = $availability_field . " : " . $embargo_restricted;
  }

  return lib4ridora_construct_filter_string_from_array($filters);
}

/**
 * Builds an optional filter query based on research data link.
 *
 * @param array $form_state
 *   Form state.
 *
 * @return string
 *   Solr filter query.
 */
function lib4ridora_construct_research_data_filter(&$form_state) {
  if ( @!isset($form_state['values']['research_data']['yes']) ) {
	return;
  }
  $ary = array_map('trim',explode('|',variable_get('lib4ridora_solr_field_research_data', 'mods_extension_resource_identifier_mt | mods:extension/mods:resource/mods:identifier[@identifierTyp="DOI"]')));
  if ( @empty($ary[0]) /* at [1] there should be the MODS-X-Path */ ) {
	return;
  }
  module_load_include("inc", "islandora_solr", "includes/utilities");

  // Get field name out of configuration.
  $solr_field = islandora_solr_lesser_escape( $ary[0] );

  $filters = array();
  if ( $form_state['values']['research_data']['yes'] ) {
    $filters[] = $solr_field . ':*';
  }
  if ( $form_state['values']['research_data']['no'] ) {
    $filters[] = '-' . $solr_field . ':*';
  }
  return lib4ridora_construct_filter_string_from_array($filters);
}
