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

/*
 * Function to find funder(s) by the award number
 * 
 */
function get_awards_by_number($award_number) {
  $awards = array();
  if ( module_exists('lib4ri_funding') ) {
    module_load_include('inc', 'lib4ri_funding', 'includes/queries' );

    $res = db_select(LIB4RI_FUNDING_AWARDS_TABLE, 'f')
      ->fields('f', array('stream_id','number','uri','title') )
      ->condition('number', $award_number)
      ->execute();

    foreach( $res as $award ) {
      $awards[intval($award->stream_id)] = array( 'sid' => $award->stream_id, 'number' => $award->number, 'uri' => $award->uri, 'title' => $award->title );
    }
  }
  return $awards;
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

/*
 * Work in progress
 * 
 */
function lib4ridora_construct_funding_element_filters(&$form_state) {		// see lib4ridora_construct_organization_unit_filter();

  $filterAry = array();
  $element_value = trim($form_state['values']['fundersearch']);
  if ( empty($element_value) || substr($element_value,0,1) == '*' || substr($element_value,0,2) == '--' ) {
    return $filterAry;
  }
/*
  $tmpAry = array();
  preg_match('/\d{4,}/', $element_value, $tmpAry );
  if ( $award_number = @intval($tmpAry[0]) ) { // seems user entered an award number - skipped, may happen it's just 2 digits
*/
  if ( !( $award_number = intval(ltrim($element_value,"[({-})] \n\r\t\v\0")) ) ) {
    // no number found at the beginning, so let's see if there is a number at the end (as eg. on Detailed Record Page):
    $tmpAry = array(); 
    preg_match('/\s+\d+$/',rtrim(strtr($element_value,'[({-})]','       ')),$tmpAry);
    $award_number = @intval(ltrim($tmpAry[0]));
  }
  if ( $award_number ) {
    $facet_fields = lib4ridora_get_solr_field_funding('award_number');		// = returns an array with solr fiels to check for the input in *one* form field
    $values = array_fill(0, count($facet_fields), strval($award_number) );
    $filters = array_map("lib4ridora_construct_solr_statement", $facet_fields, $values);
    $filterAry[] = lib4ridora_construct_filter_string_from_array($filters);
    
    $element_value = str_replace(strval($award_number),'',$element_value);  // remove the number, so we just have text
    $element_value = trim( preg_replace('/\s*\(\s*\)\s*/',' ',strtr($element_value,'[{}]','(())')), "[({-})] \n\r\t\v\0" );
  }
  if ( empty($element_value) || !module_exists('lib4ri_funding') ) {
    return $filterAry;
  }
  module_load_include('inc', 'lib4ri_funding', 'includes/queries' );
  if ( sizeof( get_awards_by_stream(0,$element_value) ) ) {
	$facet_fields = lib4ridora_get_solr_field_funding('award_title');		// = returns an array with solr fiels to check for the input in *one* form field
    $values = array_fill(0, count($facet_fields), $element_value );
    $filters = array_map("lib4ridora_construct_solr_statement", $facet_fields, $values);
    $filterAry[] = lib4ridora_construct_filter_string_from_array($filters);
  }
  return $filterAry;
}

/**
 * Callback function to find funding data for auto-complete fieled in adv. search.
 * 
 * Called via hook_menu()
 */
function lib4ridora_fundersearch_autocomplete( $search_string ) {
  $output = NULL;
  if ( module_exists('lib4ri_funding') ) {
    module_load_include('inc', 'lib4ri_funding', 'includes/autocomplete' );
    $output = lib4ri_funding_atitle_autocomplete( 0 /* known funding stream */, $search_string );
  }
  drupal_json_output($output);
}

/**
 * Customized callback function to check cooperation/inclusion with/of 'lib4ri_funding' module
 *
 * Called via hook_menu()
 *
 * Test link: https://www.dora-dev.lib4ri.ch/psi/lib4ridora/fundersearch/autocomplete/Calipso
 */
function lib4ridora_adv_search_funding_callback( $search_string ) {
  $search_string = trim(substr(strchr(strip_tags($_GET['q']),'/autocomplete/'),14));  // work-around, due to experienced differences between sub-sites
  $dataAry = array();
  if ( !module_exists('lib4ri_funding') ) {
    $dataAry['*'] = '--- search suggestions are not available currently';
  } elseif ( strlen($search_string) < 3 ) {
    $dataAry['*'] = ( strlen($search_string) < 2 ? '--- please type at least 3 characters' : '--- please type one more' );
  } else {
    module_load_include('inc', 'lib4ri_funding', 'includes/autocomplete' );
    if ( !( $award_number = intval(ltrim($search_string,"[({-})] \n\r\t\v\0")) ) ) {
      // no number found at the beginning, so let's see if there is a number at the end (as eg. on Detailed Record Page):
      $tmpAry = array(); 
      preg_match('/\s+\d+$/',rtrim(strtr($search_string,'[({-})]','       ')),$tmpAry);
      $award_number = @intval(ltrim($tmpAry[0]));
    }
    if ( !$award_number ) {
      $dataAry = lib4ri_funding_atitle_autocomplete( 0 /* known funding stream */, $search_string );
    } else { // if a (award) number is found then search for both:
      $nAry = lib4ri_funding_atitle_autocomplete(0,strval($award_number));
      $search_string = str_replace(strval($award_number),'',$search_string);  // remove the number, so we just have text
      $search_string = trim( preg_replace('/\s*\(\s*\)\s*/',' ',strtr($search_string,'[{}]','(())')), "[({-})] \n\r\t\v\0" );
      $tAry = lib4ri_funding_atitle_autocomplete(0,$search_string);
      if ( sizeof($tAry) && sizeof($nAry) && implode('|',$tAry) != implode('|',$nAry) ) {
		$dataAry = array_merge( $tAry, /* array(' '.strval($award_number)), */ $nAry );
	  } else {
		$dataAry = ( sizeof($tAry) ? $tAry : $nAry );
	  }
    }
    if ( !sizeof($dataAry) ) {
      $dataAry['*'] = '--- sorry, nothing alike available';
    } else { /* if the user is selection an auto-complete-suggestions, then the array key is actually copied into the form field!(?) */
      $dataAry = array_combine( array_values($dataAry), array_values($dataAry) );
    }
  }
  drupal_json_output($dataAry);
}
