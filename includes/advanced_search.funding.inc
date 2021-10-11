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
function lib4ridora_get_solr_field_funding($funding_element = '',$field_type = 'ms') {
	if ( strtolower($funding_element) == 'fundersearch' ) {
		$ary = explode(',', variable_get('lib4ridora_funding_solr_field_default','mods_extension_fundingReferences_fundingReference_awardTitle_mt'));
		foreach( $ary as $idx => $val ) { $ary[$idx] = ( empty($field_type) ? trim($val) : preg_replace('/_m\w{1,3}$/','_'.$field_type,trim($val)) ); }
		return $ary;
	}
	if ( strtolower($funding_element) == 'funderselect' ) {
		$ary = explode(',', variable_get('lib4ridora_funding_solr_field_funder','mods_extension_fundingReferences_fundingReference_fundingStream_mt'));
		foreach( $ary as $idx => $val ) { $ary[$idx] = ( empty($field_type) ? trim($val) : preg_replace('/_m\w{1,3}$/','_'.$field_type,trim($val)) ); }
		return $ary;
	}
	if ( stripos($funding_element,'funder_name') === 0 ) {
		return array('mods_extension_fundingReferences_fundingReference_funderName_'.$field_type);
	}
	if ( stripos($funding_element,'funding_stream') === 0 ) {
		return array('mods_extension_fundingReferences_fundingReference_fundingStream_'.$field_type);
	}
	if ( stripos($funding_element,'award_title') === 0 ) {
		return array('mods_extension_fundingReferences_fundingReference_awardTitle_'.$field_type);
	}
	if ( stripos($funding_element,'award_number') === 0 ) {
		return array('mods_extension_fundingReferences_fundingReference_awardNumber_'.$field_type);
	}
	return '';
}

/**
 * Will create an funding related array to use with 'select' form field
 * 
 * See also lib4ridora_get_all_organizational_units()
 */
function lib4ridora_get_funding_options( $funding_element, $add_wildcard = FALSE ) {
	
  $facet_fields = lib4ridora_get_solr_field_funding($funding_element,'ms');
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
 * 
 * Issue:
 * With the field as we have it - offering auto-completion and user-typped input at the same time - we cannot be sure that
 * the auto-completion and attached JS-Code is triggered.
 * So the approach of delivering the value-corresponding Solr field (one of many) as semi-colon separated suggestion-perfix may work well
 * (but only) as long as user is selecting/clicking (on) one of the suggenstions. We better should recheck typped input on/after submit.
 * The re-evaluation code below should be logcally in sync with the suggestions resp. lib4ridora_adv_search_autocomplete_callback() function.
 */
function lib4ridora_construct_funding_element_filter(&$form_state, $funding_element = '' ) {                // see lib4rido$
  if ( empty($funding_element) ) { return ""; }

  if ( in_array($funding_element,array('fund1')) ) { return ""; }

  if ( !in_array($funding_element,array('fund1','fund2','fund3')) ) {

    $element_value = $form_state['values'][$funding_element];
    if ( empty($element_value) || $element_value == "*" ) { return ""; }

    $facet_fields = lib4ridora_get_solr_field_funding( $funding_element );
    $values = array_fill(0, count($facet_fields), $element_value);
    $filters = array_map("lib4ridora_construct_solr_statement", $facet_fields, $values);

    return lib4ridora_construct_filter_string_from_array($filters);
  }

// MI-STEP 2
  // vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
global $user;

  $facet_fields = lib4ridora_get_solr_field_funding('fundersearch','mt');		// default, if we cannot find something better
  $solr_field_default = reset($facet_fields);
  $solr_field_funder = array_shift( lib4ridora_get_solr_field_funding('funding_stream','mt') );	// = Solr field to use, currently support for 1 field only.
  $solr_field_anumber = array_shift( lib4ridora_get_solr_field_funding('award_number','mt') );	// = Solr field to use, currently support for 1 field only.
  $solr_field_atitle = array_shift( lib4ridora_get_solr_field_funding('award_title','mt') );	// = Solr field to use, currently support for 1 field only.

  $valueAry = ( @is_array($form_state['values'][$funding_element]) ? $form_state['values'][$funding_element] : array( $form_state['values'][$funding_element] ) );


	if ( $user->name == "Frank" ) {
			dpm( "ValueAry: " . print_r( $valueAry , 1 ) );
	}

  $searchAry = array();
  foreach( $valueAry as /* $element_value */ $fieldAry ) {
    $filters = array();
	/* $valueAry : Array ( 
		[0] => Array ( 
				[field] => mods_extension_fundingReferences_fundingReference_awardTitle_mt
				[search] => GEANARAFDEV - Genomic Analyses of Arabidopsis Flower Development: sORFs, miRNAs, and transcription factor-coding genes
				[hidden_submit] => Search
				[add] => +f2
				[remove] => -f2
				[boolean] => AND
			)
		[1] => Array (
				[field] => mods_extension_fundingReferences_fundingReference_awardTitle_mt
				[search] => HORIZON - Research into effects on cognitive performance of maritime watch-keepers under different watch patterns, workloads & conditions, with reality usage of ships bridge, engine & cargo control simulators
				[hidden_submit] => Search
				[add] => +f2
				[remove] => -f2
			)
		)
	Note: [field] => mods_extension_fundingReferences_fundingReference_awardTitle_mt is the default - this may not be appropriate resp. not fit to [search] !
	*/
    if ( !( $search_string = trim($fieldAry['search']) ) ) { continue; }

	$solr_field = $solr_field_funder;
    $fundAry = lib4ridora_funding_data_solr_autocomplete( $search_string, $solr_field );
    foreach( $fundAry as $value ) {
	  if ( stripos($value,$search_string) !== false ) {		// re-check needed!?
		$filters[] = lib4ridora_construct_solr_statement($solr_field, $value);
	  }
    }
    $dataAry = array();
    if ( !( $award_number = intval(ltrim($search_string,"[({-})] \n\r\t\v\0")) ) ) {
      // no number found at the beginning, so let's see if there is a number at the end (as eg. on Detailed Record Page):
      $tmpAry = array(); 
      preg_match('/\s+\d+$/',rtrim(strtr($search_string,'[({-})]','       ')),$tmpAry);
      $award_number = @intval(ltrim($tmpAry[0]));
    }
    if ( !$award_number ) { // if a (award) number is found then search for both:
      $solr_field = $solr_field_atitle;
      $dataAry = lib4ridora_funding_data_solr_autocomplete( $search_string, $solr_field_atitle );		// we can/need/can re-use this function here
    } else {
      $solr_field = $solr_field_anumber;
      $dataAry = lib4ridora_funding_data_solr_autocomplete( $search_string, $solr_field_anumber );
    }
    foreach( $dataAry as $value ) {
	  if ( stripos($value,$search_string) !== false ) {		// re-check needed!?
        $filters[] = lib4ridora_construct_solr_statement($solr_field, $value);
      }
    }
    $searchAry[] = array( 'find' => lib4ridora_construct_filter_string_from_array($filters), 'op' => @strval($fieldAry['boolean']) );
  }
  // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  if ( sizeof($searchAry) < 2 ) {
    return @strval($searchAry[0]['find']);
  }
  $searchAll = '';
  foreach( $searchAry as $sIdx => $sAry ) {
    $searchAll .= ( strpos($sAry['find'],' OR ') ? '(' . $searchAll . ')' : $sAry['find'] ) . ( empty($sAry['op']) ? ' ' : ' '.$sAry['op'].' ' );
  }
  return rtrim($searchAll);
}

function lib4ridora_construct_funding_autocomplete_filters(&$form_state, $funding_type = 'award_title', $funding_element = 'fund2' ) {		// see lib4ridora_construct_organization_unit_filter();

  if ( @empty($funding_element) || @empty($funding_type) || @!isset($form_state['values'][$funding_element]) ) { return array(); }

global $user;

  $facet_fields = lib4ridora_get_solr_field_funding('fundersearch','mt');		// default, if we cannot find something better
  $solr_field = reset($facet_fields);
  $solr_field_funder = array_shift( lib4ridora_get_solr_field_funding('funding_stream','mt') );	// = Solr field to use, currently support for 1 field only.
  $solr_field_anumber = array_shift( lib4ridora_get_solr_field_funding('award_number','mt') );	// = Solr field to use, currently support for 1 field only.
  $solr_field_atitle = array_shift( lib4ridora_get_solr_field_funding('award_title','mt') );	// = Solr field to use, currently support for 1 field only.
  if ( empty($solr_field_atitle) ) { return ''; }

  $valueAry = ( @is_array($form_state['values'][$funding_element]) ? $form_state['values'][$funding_element] : array( $form_state['values'][$funding_element] ) );

if ( $user->name == "Frank" ) {
	dpm( $funding_type . " / ValueAry/AC: " . print_r( $valueAry , 1 ) );
}

  $searchAry = array();
  $searchAll = '';
  foreach( $valueAry as /* $element_value */ $fieldAry ) {
	/* $valueAry : Array ( 
		[0] => Array ( 
				[field] => mods_extension_fundingReferences_fundingReference_awardTitle_mt
				[search] => GEANARAFDEV - Genomic Analyses of Arabidopsis Flower Development: sORFs, miRNAs, and transcription factor-coding genes
				[hidden_submit] => Search
				[add] => +f2
				[remove] => -f2
				[boolean] => AND
			)
		[1] => Array (
				[field] => mods_extension_fundingReferences_fundingReference_awardTitle_mt
				[search] => HORIZON - Research into effects on cognitive performance of maritime watch-keepers under different watch patterns, workloads & conditions, with reality usage of ships bridge, engine & cargo control simulators
				[hidden_submit] => Search
				[add] => +f2
				[remove] => -f2
			)
		)
	Note: [field] => mods_extension_fundingReferences_fundingReference_awardTitle_mt is the default - this may not be appropriate resp. not fit to [search] !
	*/
    if ( !( $search_string = trim($fieldAry['search']) ) ) { continue; }

    $solr_field = $solr_field_funder;
    $filters = array();
    $resultAry = lib4ridora_funding_data_solr_autocomplete( $search_string, $solr_field_funder );
    foreach( $resultAry as $value ) {
      if ( stripos($value,$search_string) !== false ) {		// re-check needed!?
        $filters[] = lib4ridora_construct_solr_statement($solr_field, $value);				// to get: mods_example_mt:"Book"
	  }
	}
	if ( sizeof($filters) ) {
	  // lib4ridora_construct_filter_string_from_array( $filters) to get: mods_example_mt:"Book" OR mods_example_mt:"Journal" */ ;
      $searchAry[] = array( 'find' => lib4ridora_construct_filter_string_from_array( $filters), 'op' => @strval($fieldAry['boolean']) );
    } 
    else {
      if ( !( $award_number = intval(ltrim($search_string,"[({-})] \n\r\t\v\0")) ) ) {
        // no number found at the beginning, so let's see if there is a number at the end (as eg. on Detailed Record Page):
        $tmpAry = array(); 
        preg_match('/\s+\d+$/',rtrim(strtr($search_string,'[({-})]','       ')),$tmpAry);
        $award_number = @intval(ltrim($tmpAry[0]));
      }
      if ( !$award_number ) {
        $solr_field = $solr_field_atitle;
        $resultAry = lib4ridora_funding_data_solr_autocomplete( $search_string, $solr_field_atitle );		// we can/need/can re-use this function here
      } else {
        $solr_field = $solr_field_anumber;
        $resultAry = lib4ridora_funding_data_solr_autocomplete( $search_string, $solr_field_anumber );
      }
      foreach( $resultAry as $value ) {
        if ( stripos($value,$search_string) !== false ) {		// re-check needed!?
          $filters[] = lib4ridora_construct_solr_statement($solr_field, $value);		// to get: mods_pubtype_mt:"Book"
        }
      }
      if ( sizeof($filters) ) {
        $searchAry[] = array( 'find' => lib4ridora_construct_filter_string_from_array($filters), 'op' => @strval($fieldAry['boolean']) );
      }
    }
  }

  if ( sizeof($searchAry) < 2 ) {
	return @strval($searchAry[0]['find']);
  }
  foreach( $searchAry as $sIdx => $sAry ) {
	$op = ( @isset($searchAry[$sIdx+1]) ? ( empty($sAry['op']) ? ' ' : ' '.$sAry['op'].' ' ) : '' );
    $searchAll .= ( strpos($sAry['find'],' OR ') ? '(' . $sAry['find'] . ')' : $sAry['find'] ) . $op;
  }
  return rtrim($searchAll);
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
    $facet_fields = lib4ridora_get_solr_field_funding('award_number','mt');		// = returns an array with solr fiels to check for the input in *one* form field
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
	$facet_fields = lib4ridora_get_solr_field_funding('award_title','mt');		// = returns an array with solr fiels to check for the input in *one* form field
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

/*
 * Solr search for 'all words for one Solr field'
 * 
 * For debugging reasons via web API currently to see/test the Solr URL, later perhaps something
 * similar to lib4ridora_facet_query() should Islandora objects be all we need to find.
 */
function lib4ridora_solr_search_words( $words = array(), $solr_field = '', $field_type = 'mt', $word_wild_card = 1 ) {
	if ( !sizeof($words) || empty($solr_field) ) {
		return FALSE;
	}
	if ( !empty($field_type) ) {
		$solr_field = preg_replace('/_m\w{1,3}$/','_'.ltrim($field_type,'_'),$solr_field);
	}

	$valueList = '';	// for Solr query
	$opWord = '';	// for possible boolean operator that may come as word
	$mustHaveAry = array();		// recheck work-round
	foreach ( $words as $word ) {
		$word = str_replace('.','',strtr($word,
			';:,><()"[]?{}&',
			'..............')
		);
		$word = $opWord . ltrim( rtrim($word,"-! \n\r\t\v\0"), "* \n\r\t\v\0");
		$opWord = '';
		if ( $word == 'AND' || $word == 'OR' ) {
			continue;
		}
		if ( $word == 'NOT' ) {
			$opWord = '-';
			continue;
		}
		if ( substr($word,0,1) == '!' || substr($word,0,1) == '-' ) {
			if ( $word = ltrim($word,"-!* \n\r\t\v\0") ) {
				$valueList .= ( ( empty($valueList) ? 'NOT+' : '+NOT+' ) . '*' . rawurlencode($word) . '+NOT+' . rawurlencode($word) . '*' );
			}
		} elseif( ( $wLen = strlen($word) ) && $wLen > 1 ) {
			if ( $chk = rtrim( $word, "* \n\r\t\v\0" ) ) {
				$mustHaveAry[] = $chk;		// so it cannot be empty of just a *
			}
			$varAry = array( rawurlencode($word) );
			if ( $word_wild_card > 0 ) {
				$varAry[] = '*'.rawurlencode($word).'*';
				if ( $wLen > 2 ) { 
					$varAry[] = '*'.rawurlencode(substr($word,1));
					$varAry[] = rawurlencode(substr($word,0,-1)).'*';
				}
				if ( $wLen > 3 ) {
					$varAry[] = '*'.rawurlencode(substr($word,1,-1)).'*';
				}
			}
			$valueList .= ( ( empty($valueList) ? '' : '+AND+' ) . '(' . implode('+OR+',$varAry) . ')' );
		}
	}
	if ( empty($valueList) ) {
		return FALSE;
	}

	// work-round: cut off 1 letter (to be rechecked later with MustHaveAry) for '_mt' fields and wildcard searches:
	$solr_term = '*' . rawurlencode( substr($search_string,0,-1) ) . '*';

	$solr_sep = '|';
	$solr_url = 'http://' . trim(exec('hostname')) .  '.emp-eaw.ch:8080/solr/collection1/select';
	$solr_url .= '?wt=json&csv.separator=' . $solr_sep . '&indent=true&rows=987654321&sort=PID%20asc';
	$solr_url .= '&fl=PID%2c+' . $solr_field;
	$solr_url .= '&q=PID:*%5c%3a*+AND+' . $solr_field . ':(' . $valueList . ')';

	$resultAry = array();
	$numFound = -1;
	if ( $jsonAry = @json_decode(file_get_contents($solr_url)) ) {
		if ( @isset($jsonAry->response->numFound) ) {
			$numFound = intval($jsonAry->response->numFound);
		}
		if ( $numFound && @isset($jsonAry->response->docs) ) {
			foreach( $jsonAry->response->docs as $obj ) {
				foreach( $obj->$solr_field as $term ) {
					$found = ( sizeof($mustHaveAry) ? false : true );
					foreach( $mustHaveAry as $chk ) {
						if ( stripos($term,$chk) === false ) { continue; }
						$found = true;
						break;
					}
					if ( $found ) {		
						$idx = strtolower($term);
						if ( @isset($resultAry[$idx]) ) { continue; }
						$resultAry[$idx] = $term;
					}
				}
			}
			ksort($resultAry);
		}
	}
	if ( $word_wild_card != 0 || $numFound !== 0 ) {		// $word_wild_card:-1 to skip wildcard-retry, $word_wild_card:1 to prevent looping!
		return $resultAry;
	}
	return lib4ridora_solr_search_words( $words, $solr_field, $field_type, 1 );	// retry with wildcard'ed words
}

/*
 * Function to search+find award titles in publications existing in DORA
 *
 * Currently the Solr's web API is used, also https://github.com/Lib4RI/lib4ridora/blob/7.x/includes/utilities.inc#L22 may work.
 */
function lib4ridora_funding_award_title_autocomplete( $search_string ) {
	if ( empty($search_string) ) {
		return FALSE;
	}
	$search_string = preg_replace('/\s+/',' ',urldecode($search_string));
	$solr_field = array_shift( lib4ridora_get_solr_field_funding('award_title','mt') );	// = Solr field to use, currently support for 1 field only.

	return lib4ridora_solr_search_words( explode(' ',$search_string), $solr_field );
}

function lib4ridora_funding_data_solr_autocomplete( $search_string, $solr_field = '' ) {
	if ( empty($search_string) || empty($solr_field) ) {
 		return FALSE;
	}
	$search_string = preg_replace('/\s+/',' ',urldecode($search_string));

	return lib4ridora_solr_search_words( explode(' ',$search_string), $solr_field );
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
  if ( strlen($search_string) < 2 ) {
    drupal_json_output( array('*' => '--- please type at least 3 characters') );
    return;
  }
  if ( strlen($search_string) < 3 ) {
    drupal_json_output( array('*' => '--- please type one more') );
    return;
  }

  // order of string-examination: award number, funding stream, award title.
  // generally, let's see if this could be a funding stream (or an award title):
  $solr_field = array_shift( lib4ridora_get_solr_field_funding('funding_stream','mt') );	// = Solr field to use, currently support for 1 field only.
  $fundAry = lib4ridora_funding_data_solr_autocomplete( $search_string, $solr_field );
  if ( @!empty(reset($fundAry)) ) {
    $fundAry = array_combine( array_values($fundAry), array_values($fundAry) );
    $fundAry = array_map( function($val) { return($val.' (=Funder)'); }, $fundAry );
  }

  $dataAry = array();
  if ( !( $award_number = intval(ltrim($search_string,"[({-})] \n\r\t\v\0")) ) ) {
    // no number found at the beginning, so let's see if there is a number at the end (as eg. on Detailed Record Page):
    $tmpAry = array(); 
    preg_match('/\s+\d+$/',rtrim(strtr($search_string,'[({-})]','       ')),$tmpAry);
    $award_number = @intval(ltrim($tmpAry[0]));
  }
  if ( !$award_number ) { // if a (award) number is found then search for both:
//  $dataAry = lib4ri_funding_atitle_autocomplete( 0 /* known funding stream */, $search_string );
    $dataAry = lib4ridora_funding_award_title_autocomplete( $search_string );
  } elseif( module_exists('lib4ri_funding') ) {
    module_load_include('inc', 'lib4ri_funding', 'includes/autocomplete' );
    $nAry = lib4ri_funding_atitle_autocomplete(0,strval($award_number));
    $sTmp = str_replace(strval($award_number),'',$search_string);  // remove the number, so we just have text
    $sTmp = trim( preg_replace('/\s*\(\s*\)\s*/',' ',strtr($sTmp,'[{}]','(())')), "[({-})] \n\r\t\v\0" );	// clean up
    $tAry = ( empty($sTmp) ? array() : lib4ridora_funding_award_title_autocomplete( $sTmp ) );
    if ( sizeof($tAry) && sizeof($nAry) && implode('|',$tAry) != implode('|',$nAry) ) {
      $dataAry = array_merge( $tAry, /* array(' '.strval($award_number)), */ $nAry );
    } else {
      $dataAry = ( sizeof($tAry) ? $tAry : $nAry );
    }
  } elseif ( @empty(reset($fundAry)) ) {
    drupal_json_output( array('*' => '--- search suggestions are not available currently') );		
    return;
  }
  
  if ( @!empty(reset($dataAry)) ) {
    $dataAry = array_combine( array_values($dataAry), array_values($dataAry) );
    if ( @!empty(reset($fundAry)) ) {
	  $sTmp = ( strlen($search_string) > 20 ? 'the user input' : '"'.strval($search_string).'"' );
	  $sTmp .= ' appears in its title' . ( intval($search_string) ? ' or number:' : ':' );
      $fundAry['*'] = '--- award titles where ' . $sTmp;
      $dataAry = array_merge( $fundAry, $dataAry );
    }
  } else {
    $dataAry = ( @empty(reset($fundAry)) ? array('*' => '--- sorry, nothing alike available') : $fundAry );
  }

  drupal_json_output($dataAry);
}
