<?php

/**
 * @file
 * Utiilty functions for Lib4Ri's altered advanced search form for FUNDING.
 */

module_load_include('inc', 'lib4ridora', 'includes/advanced_search');

function lib4ridora_get_solr_field_funding($funding_element = '') {  // see lib4ridora_parse_affiliation_fields() - vairables would be nice!?
	if ( stripos($funding_element,'funder_name') === 0 ) {
		return array('mods_extension_fundingReferences_fundingReference_funderName_ms');
	}
	if ( stripos($funding_element,'funding_stream') === 0 ) {
		return array('mods_extension_fundingReferences_fundingReference_fundingStream_ms');
	}
	if ( stripos($funding_element,'award_title') === 0 ) {
		return array('mods_extension_fundingReferences_fundingReference_awardTitle_ms');
	}
	if ( stripos($funding_element,'award_number') === 0 ) {
		return array('mods_extension_fundingReferences_fundingReference_awardNumber_ms');
	}
	return '';
}

function lib4ridora_get_funding_options( $funding_element, $add_wildcard = FALSE ) {	// similar to lib4ridora_get_all_organizational_units() , without  */All
	
  $facet_fields = lib4ridora_get_solr_field_funding($funding_element);
  $facets = lib4ridora_facet_query($facet_fields);

  // If there's results, construct an array where the keys are affiliations as in Solr, and the values are formatted to be labels.
  $fundAry = array();
  foreach ($facets as $facet) {
    if ( !empty($facet) ) {
      $fundAry = array_merge($fundAry, array_combine( array_keys($facet), array_keys($facet) ) );
    }
  }

  // Sort the array alphanumerically (ignoring lower/upper case):
  $sortAryTmp = array_map( 'strtolower', $fundAry );
  if ( stripos($solr_field,'number') ) {
    $sortAryTmp = array_map( function($val) { return str_pad($val,10,'0',STR_PAD_LEFT); }, $sortAryTmp );
  }
  array_multisort( $sortAryTmp, SORT_ASC, SORT_STRING, $fundAry );

  return ( $add_wildcard ? array_merge( array("*" => "All"), $fundAry ) : $fundAry );
}

/**
 * Builds a filter query based on funding data
 *
 * @param array $form_state
 *   Form state.
 *
 * @return string
 *   Solr filter query.
 */
function lib4ridora_construct_funding_element_filter(&$form_state, $funding_element = '' ) {		// see lib4ridora_construct_organization_unit_filter();

  $element_value = "*";
  if ( !empty($funding_element) ) {  // Get the organizational unit value from the dropdown.
    $element_value = $form_state['values'][$funding_element];
  }
  if ($element_value == "*") {
    return "";
  }

  $facet_fields = lib4ridora_get_solr_field_funding( $funding_element );
  $values = array_fill(0, count($facet_fields), $element_value);
  $filters = array_map("lib4ridora_construct_solr_statement", $facet_fields, $values);

  return lib4ridora_construct_filter_string_from_array($filters);
}
