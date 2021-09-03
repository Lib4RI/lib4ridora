<?php

/**
 * @file
 * Utiilty functions for Lib4Ri's altered advanced search form for FUNDING.
 */

module_load_include('inc', 'lib4ridora', 'includes/advanced_search');	// for safety, perhaps later to merge.

/**
 * Look-up function to find the right solr field for the form fields related to funding.
 * 
 * see lib4ridora_parse_affiliation_fields() - So, vairables would be nice!?
 */
function lib4ridora_get_solr_field_funding($funding_element = '') {
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

/**
 * Will create an funding related array to use with 'select' form field
 * 
 * See also lib4ridora_get_all_organizational_units()
 */
function lib4ridora_get_funding_options( $funding_element, $add_wildcard = FALSE ) {
	
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
  if ( stripos($funding_element,'number') ) {
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

/**
 * Test callback function to check cooperation/inclusion with/of 'lib4ri_funding' module
 *
 * Test link: https://www.dora-dev.lib4ri.ch/psi/adv-search/funding/Calipso
 */
function lib4ridora_adv_search_funding_callback( $search_string /* should be passed in automatically, otherwise use $_GET['q'] minus length of AC-path */ ) {
  $dataAry = array();
  if ( module_exists('lib4ri_funding') ) {
    module_load_include('inc', 'lib4ri_funding', 'includes/autocomplete' );
    $dataAry = lib4ri_funding_atitle_autocomplete( 0 /* known funding stream */, $search_string );
    $dataAry = array_combine( array_values($dataAry), array_values($dataAry) );
  }
  drupal_json_output($dataAry);
}

/**
 * Callback function to find funding data for auto-complete fieled in adv. search.
 * 
 * Called via hook_menu()
 */
function lib4ridora_funding_autocomplete( $search_string ) {
  $output = NULL;
  if ( module_exists('lib4ri_funding') ) {
    module_load_include('inc', 'lib4ri_funding', 'includes/autocomplete' );
    $output = lib4ri_funding_atitle_autocomplete( 0 /* known funding stream */, $search_string );
  }
  drupal_json_output($output);
}
