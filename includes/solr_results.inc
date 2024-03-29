<?php

/**
 * @file
 * Contains methods to search solr and display results. Depends on
 * Apache_Solr_Php client.
 */

/**
 * Extension of IslandoraSolrResultsBookmark to create an alternative display
 * type. The purpose of overriding this for Lib4RI, is to remove the markup from
 * the returned $object_url_info, before being passed to bookmark's
 * generate_markup helper.
 */
class IslandoraSolrResultsLib4riBookmark extends IslandoraSolrResultsBookmark {
  /**
   * Constructor.
   */
  public function __construct() {
    parent::__construct();
  }

  /**
   * Build a row for inclusion in the tableselect.
   *
   * @param array $object_result
   *   A result from the query processor.
   *
   * @return array
   *   An associative array of cells, keyed according to the header provided in
   *   $this->getTableHeader().
   */
  protected function getTableRow($object_result) {
    // Define per content model and what fields should be rendered in
    // Addition to the Bookmark content. This is gross, but it is a
    // Custom display. These fields really should be configurable.
    $cmodels = array(
      'info:fedora/islandora:organizationCModel' => array(
        0 => "fgs_label_s",
        "prefix" => array(
          "Organizational Unit",
        ),
      ),
      'info:fedora/islandora:collectionCModel' => array(
        0 => "fgs_label_s",
        "prefix" => array(
          "Collection",
        ),
      ),
      'info:fedora/lib4ri:journalCModel' => array(
        0 => "fgs_label_s",
        "prefix" => array(
          "Journal",
        ),
      ),
      'info:fedora/islandora:personCModel' => array(
        0 => 'fgs_label_s',
        "prefix" => array(
          "Author",
        ),
      ),
    );

    // Initilize our array's and markup variables.
    $markup = "";
    $data = array(
      "key" => NULL,
      "fields" => NULL,
      "prefix" => "",
    );

    foreach ($cmodels as $key => $value) {
      $found = array_search($key, $object_result['content_models']);
      if (FALSE !== $found) {
        $data['key'] = $key;
        $data['fields'] = $cmodels[$key];
        $data['prefix'] = $cmodels[$key]['prefix'][0];
        $markup .= $this->getMarkupForCModel($data, $object_result);
      }
    }

    $object_url_info = array(
      'path' => $object_result['object_url'],
      'params' => $object_result['object_url_params'],
      'markup' => "",
    );
    return array(
      'markup' => $markup . islandora_bookmark_generate_markup($object_result['PID'], $object_url_info),
    );
  }

  /**
   * Provide specific fields for given content models.
   *
   * @param array $data
   *   Key value array of CModels and associated fields.
   *
   * @param array $object_results
   *   A result from the query processor.
   *
   * @return string
   *   Concatenated and space separated string of provided
   *   Fields.
   */
  protected function getMarkupForCModel($data, $object_results) {
    $markup = "";
    foreach ($data['fields'] as $key => $value) {
      if (is_string($value)) {
        $field = isset($object_results['solr_doc'][$value]) ? $object_results['solr_doc'][$value] : "";
        if (is_array($field)) {
          $markup .= implode(" ", $field) . " ";
        }
        else {
          $markup .= $field . " ";
        }
      }
    }
    return t("@prefix", array("@prefix" => $data['prefix'])) . ": " . $markup;
  }

  /*
  * function to replace the name of a Solr field with the corresponding label as found in the
  * Solr configuration https://www.dora.lib4ri.ch/admin/islandora/search/islandora_solr/settings
  *
  * Array input can be obtained from the related MySQL tables e.g. with:
  * 	- islandora_solr_get_fields("search_fields", FALSE, FALSE);
  * 	- islandora_solr_get_fields("facet_fields", FALSE, FALSE);
  */
  public function lib4ridora_replace_field_with_label( $query_string = '', $solr_fields = array(), $remove_weight = false ) {
	$labelAry = array( /* 'full' => array(  solr_field_with_mxy => Field-Label ), 'aux' => array(  solr_field_no_mxy => Field-Label ) ) */ );
    foreach ($solr_fields as $solr_field => $data) {		// $solr_field also can be just a digit depending on the data source used!
      if ( empty($data) ) { continue; }
      $label = '';
      if ( !intval($solr_field) && is_string($data) ) {
        $label = $data;	// no corresponding table available, rather for flexibility
      } else {
        $solr_field = $data['solr_field'];
        if ( !( $label = @strval($data['solr_field_settings']['label']) ) ) {  /* if table 'islandora_solr_fields' was used */
          if ( !( $label = @strval($data['data']['display_label']) ) ) {    /* if table 'islandora_solr_metadata_fields' was used */
            if ( !( $label = @strval($data['display_label']) ) || !( $label = @strval($data['label']) ) ) {    /* just for convenience */
              continue;    // eek! one of the key-path must exist!
            }
          }
        }
      }
      if ( @empty($labelAry['full'][$solr_field]) ) {
        $labelAry['full'][$solr_field] = $label;
        if ( ( $pos = strrpos($solr_field,'_') ) && in_array( strtolower(substr($solr_field,$pos+1)), array('s','ss','ms','mt','mlt','dt','mdt') ) ) {
          $solr_field = substr($solr_field,0,$pos);	// without _ms, _mt, ...
          if ( @empty($labelAry['aux'][$solr_field]) ) {
            $labelAry['aux'][$solr_field] = $label;
          }
        }
      }
    }
    if ( @isset($labelAry['full']) ) {
      foreach( $labelAry['full'] as $solr_field => $label ) {
        $query_string = str_ireplace($solr_field, $label, $query_string);
      }
    }
    if ( @isset($labelAry['aux']) ) {
      foreach( $labelAry['aux'] as $solr_field => $label ) {
		$query_string = preg_replace('/'.substr($solr_field,0,$pos).'_\w+/i', $label, $query_string);
      }
    }
    if ( $remove_weight && strpos($query_string,'^') ) {  // try to remove search-weights:
      $query_string = preg_replace('/\)\^\d\s+/', ') ', $query_string.' ');    // for: (carbon)^5
      $query_string = preg_replace('/\"\^\d\s+/', '" ', $query_string.' ');    // for: "carbon"^5
    }
    return rtrim($query_string);
  }

  /**
   * Displays elements of the current solr query.
   *
   * Displays current query and current filters. Includes a link to exclude the
   * query/filter.
   *
   * @param IslandoraSolrQueryProcessor $islandora_solr_query
   *   The IslandoraSolrQueryProcessor object which includes the current query
   *   settings and the raw Solr results.
   *
   * @return string
   *   Rendered lists of the currently active query and/or filters.
   */
  public function currentQuery($islandora_solr_query) {
    module_load_include('inc', 'islandora', 'includes/utilities');
    module_load_include('inc', 'lib4ridora', 'includes/utilities');

    $output = '';
    $path = current_path();
    $format = variable_get('islandora_solr_facet_date_format', 'Y');

    $solr_facet_fields = islandora_solr_get_fields("facet_fields", FALSE, FALSE, TRUE);
    $solr_search_fields = islandora_solr_get_fields("search_fields", FALSE, FALSE, TRUE);
    $solr_misc_fields = array_merge(
		islandora_solr_get_fields("sort_fields", FALSE, FALSE, TRUE),
		islandora_solr_get_fields("result_fields", FALSE, FALSE, TRUE),
		lib4ridora_get_metadata_fields() /* , */
	);
	$solr_all_fields = array_reverse(array_merge($solr_misc_fields,$solr_search_fields,$solr_facet_fields));

    // Get user provided filter parameters.
    $fq = isset($islandora_solr_query->internalSolrParams['f']) ? $islandora_solr_query->internalSolrParams['f'] : array();
    // Parameters set in URL.
    $params = $islandora_solr_query->internalSolrParams;
    // Get query values.
    if (!in_array($islandora_solr_query->solrQuery, $islandora_solr_query->differentKindsOfNothing)) {

      $query_value = '';
      // spacial case: if there is an explit given search string then just use this:
      if ( !( $query_value_human = @trim(strip_tags($_GET['search_string'])) ) ) { // possible html tags need to be stripped to prevent html injection
        // Get query value/string.
        $query_value = $islandora_solr_query->solrQuery;
        $query_value_human = $this->lib4ridora_replace_field_with_label( $query_value, $solr_all_fields, true );
      }

      $query_list = array();

      // Remove link keeps all parameters (query gets removed instead).
      $query_minus = $params;

      // Remove query from path.
      $path_minus = implode('/', explode('/', $path, -1));

      // Set attributes variable for remove link.
      $attributes = array(
        'minus' => array(
          'attr' => array(),
          'path' => $path_minus,
          'query' => $query_minus,
        ),
      );
      $attr_minus =& $attributes['minus']['attr'];
      $attr_minus['title'] = t('Remove') . ( empty($query_value) ? '' : ' '.strip_tags($query_value) );
      $attr_minus['class'] = array('remove-query');
      $attr_minus['rel'] = 'nofollow';
      $attr_minus['href'] = url($path_minus, array('query' => $query_minus));

      $hooks = islandora_build_hook_list(ISLANDORA_SOLR_FACET_BUCKET_CLASSES_HOOK_BASE);
      drupal_alter($hooks, $attributes, $islandora_solr_query);

      // XXX: We are not using l() because of active classes:
      // @see http://drupal.org/node/41595
      // Create link.
      $query_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a> ' . htmlentities($query_value_human);

      // Add wrap and list.
      $output .= '<div class="islandora-solr-query-wrap">';
      $output .= theme('item_list', array(
        'items' => $query_list,
        'title' => t('Query'),
        'type' => 'ul',
        'attributes' => array('class' => 'islandora-solr-query-list query-list'),
      ));
      $output .= '</div>';
    }

    // Get filter values.
    if (empty($fq)) { return $output; }
    /* $fq = Array(
		[0] => mods_originInfo_encoding_w3cdtf_keyDate_yes_dateIssued_dt:[2001-01-01T00:00:00Z TO 2021-12-31T23:59:59Z]
		[1] => mods_genre_ms:"Journal Article"
	)	*/

    // Set list variables.
    $filter_list = array();
    foreach ($fq as $key => $filter) {
      // Check for exclude filter.
      $symbol = ($filter[0] == '-') ? '&ne;' /* 'not equal' sign */ : '=';
      $filter_string = str_replace('\\','',urldecode($filter));		// ensure %5c%20 or %5c%3a will not end up with a backslash inside!
      $is_filter_with_op = false;
      if ( /* stripos($filter_string,'_dt:') || */ !preg_match('/\s+(AND|OR|NOT)\s+/',$filter_string) ) {
        $filter_string = $this->formatFilter($filter,$islandora_solr_query);
      } else {
        $is_filter_with_op = true;
        $filter_string = $this->formatFilterSimple($filter_string,$solr_all_fields);
      }
      // Pull out filter (for exclude link).
      $query_minus = array();
      $f_x['f'] = array_diff($params['f'], array($filter));
      $query_minus = array_merge($params, $f_x);
      // @todo Find a cleaner way to do this.
      // Resetting the filter keys' order.
      if ($query_minus['f']) {
        $query_minus['f'] = array_merge(array(), $query_minus['f']);
      }
      // Remove 'f' if empty.
      if (empty($query_minus['f'])) {
        unset($query_minus['f']);
      }
      // Set attributes variable for remove link.
      $attributes = array(
        'minus' => array(
          'attr' => array(),
          'path' => $path,
          'query' => $query_minus,
        ),
      );
      $filterAry = explode(':',$filter,2);
      if ( @!empty($filterAry[1]) ) {		// this is just for display reasons, so remove query-auxiliary backslashes
			$filterAry[1] = ' ' . trim(str_replace("\\",'',$filterAry[1]));
		}
      $attr_minus =& $attributes['minus']['attr'];
      $attr_minus['title'] = t('Remove') . ' ' . strip_tags(implode(':',$filterAry));
      $attr_minus['class'] = array('remove-filter');
      $attr_minus['rel'] = 'nofollow';
      $attr_minus['href'] = url($path, array('query' => $query_minus));

      $hooks = islandora_build_hook_list(ISLANDORA_SOLR_FACET_BUCKET_CLASSES_HOOK_BASE);
      drupal_alter($hooks, $attributes, $islandora_solr_query);

      // Append the human readable labels to the front of
      // the facet removal link, in the 'current_query_facet_name' block.
      // @XXX: We are not using l() because of active classes:
      // @see http://drupal.org/node/41595
      // The following differs from the default
      // islandora_query block currentQuery().
      $filterPlain = preg_replace( '/\\\\ /', ' ', strtok($filter.':',':') );
      $keySolrField = -1;

      foreach( $solr_all_fields as $key => $value) {
        $solr_field = $value['solr_field'];
        if ( stripos($filterPlain,$solr_field) !== FALSE ) {
          $keySolrField = $key;
          break;	// perfect match, so stop
        }
        // cut off '_mt', '_ms', ... - remind there are dc.title, dc.creator, ... or psuedo-solr-fields:
        if ( $keySolrField < 0 && !strpos($solr_field,'.') && ( $pos = strrpos($solr_field,'_') ) ) {
          if ( stripos($filterPlain,substr($solr_field,0,$pos)) !== FALSE ) {
            $keySolrField = $key;		// since we have nothing better so far
          }
        }
      }
      if ( $keySolrField >= 0 && ( $value = $solr_all_fields[$keySolrField] ) ) {
        // Re-write certain PIDs to name strings.
        if (isset($value['solr_field_settings']['pid_object_label']) && $value['solr_field_settings']['pid_object_label']) {
          $pidAry = array();
          foreach( array('eawag','empa','psi','wsl') as $inst ) {
            foreach( array('units','authors') as $objType ) {
              $regEx = '/' . $inst . '\-' . $objType . ':\d+/';
              $matchAry = array();
              if ( preg_match_all( $regEx, $filter_string, $matchAry ) ) {
                foreach( $matchAry as $mAry ) {
                  foreach( $mAry as $pid ) { $pidAry[$pid] = ''; }
                }
              }
            }
          }
          if ( empty($pidAry) ) {
            $pidAry[$filter_string] = '';  // assumed $filter_string could be any type of PID we don't know
          }
          foreach( $pidAry as $mypid => $myany ) {
            if ( @!islandora_is_valid_pid($mypid) ) { continue; }
            $myqp = new IslandoraSolrQueryProcessor();
            $mylbl = '';
            $myqp->buildQuery("PID:\"$mypid\"");
            $myqp->solrParams['fl'] = 'PID, ' . variable_get( 'islandora_solr_object_label_field', 'fgs_label_s' );
            $myqp->executeQuery();
            if ( !empty($myqp->islandoraSolrResult) && !empty($myqp->islandoraSolrResult['response']['objects']) ) {
              if ( !( $mylbl = @trim( $myqp->islandoraSolrResult['response']['objects'][0]['object_label'] ) ) ) { $mylbl = "..."; }
            } else {
              if ( $myobj = islandora_object_load($mypid) ) { $mylbl = strval($myobj->label); }
            }
            if ( !empty($mylbl) ) {
              $filter_string = str_replace( $mypid, $mylbl, $filter_string );
            }
          }
        }
        if ( !( $label = @strval($value['solr_field_settings']['label']) ) ) {  /* if table 'islandora_solr_fields' was used */
          if ( !( $label = @strval($value['data']['display_label']) ) ) {    /* if table 'islandora_solr_metadata_fields' was used */
            if ( !( $label = @strval($$value['display_label']) ) || !( $label = @strval($value['label']) ) ) {    /* just for convenience */
              $label = '&hellip;';
            }
          }
        }
        $tmp = check_plain(ltrim($filter_string,'-!'));
        if ( $is_filter_with_op && stripos($tmp,$label) !== false ) {	// they got a special treatment, so do not add the label (=Solr field name) once again
			$filter_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a> ' . $tmp;
		} else {
			$filter_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a> ' . $label . ' ' . $symbol . ' ' . $tmp;
		}
      }
      else { // Default to old behavior.
        $filter_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a> &hellip; ' . $symbol . ' ' . check_plain(ltrim($filter_string,'-!'));
      }
    }

    // Return filter list.
    $output .= '<div class="islandora-solr-filter-wrap">';
    $output .= theme('item_list', array(
      'items' => $filter_list,
      'title' => t("Enabled Filters"),
      'type' => 'ul',
      'attributes' => array('class' => 'islandora-solr-filter-list filter-list'),
    ));
    $output .= '</div>';
    return $output;
  }

  /**
   * Formats the passed in filter into a human readable form. Based on formatFilter()
   *
   * Cannot (yet) handle Date-Solr-fields currenty, so check $filter about such a field
   * e.g. with stripos($filter,'_dt:') and prefer formatFilter() then.
   * 
   */
  public function formatFilterSimple($filter, $solr_fields = array(), $date_format = '') {		// date_format: e.g. "Y-m-d"
    if ( empty($solr_all_fields) ) {
      module_load_include('inc', 'islandora', 'includes/utilities');
      module_load_include('inc', 'lib4ridora', 'includes/utilities');
      module_load_include('inc', 'lib4ridora', 'includes/advanced_search.funding.inc');
      // get all Solr fields incl. labels:
      $solr_facet_fields = islandora_solr_get_fields("facet_fields", FALSE, FALSE, TRUE);
      $solr_search_fields = islandora_solr_get_fields("search_fields", FALSE, FALSE, TRUE);
      $solr_misc_fields = array_merge(
          islandora_solr_get_fields("sort_fields", FALSE, FALSE, TRUE),
          islandora_solr_get_fields("result_fields", FALSE, FALSE, TRUE),
          lib4ridora_get_metadata_fields() /* , */
      );
      $solr_fields = array_reverse(array_merge($solr_misc_fields,$solr_search_fields,$solr_facet_fields));
    }
    if ( empty($date_format) ) { $date_format = "Y"; }
    // replace date-spans with year-spans, expect $filter to contain "...sued_dt:[2014-01-01T00:00:00Z TO 2021-12-31T23:59:59Z]&..." or even URL-encoded.
    while( @!isset($yearRange) || @!empty($yearRange) ) {			// loop, as support for multiple dates
      $yearRange = array(); // intentionally initiate here/later!
      if ( ( ( $pos0 = stripos($filter,'_dt:',@intval($pos)) ) && ( $len0 = 4 ) ) || ( ( $pos0 = stripos($filter,'_dt%3A',@intval($pos)) ) && ($len0 = 6 ) ) ) {
		if ( ( substr($filter.'&',$pos0+$len0,2) == '*&' && ( $len1 = 1 ) ) || ( strtoupper(substr($filter.'&',$pos0+$len0,4)) == '%2A&' && ( $len1 = 3 ) ) ) {
          // so we just have an '*' - replace it with 'any...':
          $yTmp = ( variable_get('islandora_solr_facets_year_only',true) ? 'Any Year' : 'Any Date' );
          if ( !stripos(strrchr(substr('&'.substr($filter,$pos,$pos0-$pos),'&')),'date') ) { $yTmp = 'Anytime'; /* for _dt Solr fields without 'date' inside */ }
          $filter = substr($filter,0,$pos0+$len0) . $yTmp . substr($filter,$pos0+$len0+$len1);
          $pos = $pos0 + 8;	// to step forward a bit in $filter
        }
        elseif ( ( ( $pos1 = strpos($filter,'[',$pos0) ) && ( $len1 = 1 ) ) || ( ( $pos1 = stripos($filter,'%5B',$pos0) ) && ( $len1 = 3 ) ) ) {
          if ( ( ( $pos2 = stripos($filter,' TO ',$pos1) ) && ( $len2 = 4 ) ) || ( ( $pos2 = stripos($filter,'%20TO%20',$pos1) ) && ( $len2 = 8 ) ) ) {
            if ( ( ( $pos3 = strpos($filter,']',$pos2) ) && ( $len3 = 1 ) ) || ( ( $pos3 = stripos($filter,'%5D',$pos2) ) && ( $len3 = 3 ) ) ) {
              $yearRange[0] = trim(substr($filter,$pos1+$len1,$pos2-$pos1-$len1));		// will be e.g. 2014-01-01T00:00:00Z
              $yearRange[1] = trim(substr($filter,$pos2+$len2,$pos3-$pos2-$len2));		// will be e.g. 2021-12-31T23:59:59Z
              $yearRange = array(	/* we do not want the timezone to be affected, so better use date_create()+date_format() than strtotime()+date() */
                0 => ( ( !empty($yearRange[0]) && substr($yearRange[0],0,1) != '*' ) ? date_format(date_create($yearRange[0]),$date_format) : 'since ever' ),
                1 => ( ( !empty($yearRange[1]) && substr($yearRange[1],0,1) != '*' ) ? date_format(date_create($yearRange[1]),$date_format) : 'now' ),
              );
              $yTmp = ( strcmp($yearRange[0],$yearRange[1]) ? implode( ( stripos($filter,'%') ? '%20-%20' : ' - ' ), $yearRange ) : $yearRange[0] );
              $filter = substr($filter,0,$pos1) . $yTmp . substr($filter,$pos3+$len3);
              $pos = $pos0 + 16;	// to step forward a bit in $filter
            }
          }
        }
      }
    }
    // Try to teplace names of Solr fields with their (configured) labels:
    $filter = $this->lib4ridora_replace_field_with_label( $filter, $solr_fields );

    // Enprettyments:
    $filter = html_entity_decode($filter,ENT_HTML401,'UTF-8'); // there may be special characters as HTML-encoding inside that may become double-encoded.
    $filter = str_replace('*:* NOT','NOT',$filter);		// the *:* before NOT may be Solr-logically needed, for human readability it's not
    return preg_replace('/\s+/',' ',str_replace(':',': ',$filter));         // should not harm, just looks better with 1 space after semicolon
  }

  /**
   * Formats the passed in filter into a human readable form.
   *
   * @param string $filter
   *   The passed in filter like:
   *    mods_extension_fundingReferences_fundingReference_fundingStream_mt:"Horizon 2020 Framework Programme" OR mods_extension_fundingReferences_fundingReference_awardTitle_mt:(Horizon AND Framework AND Prog...
   * @param object $islandora_solr_query
   *   The current Solr Query as big array with all the results inside and other query/facet related data
   *
   * @return string
   *   The formatted filter string for breadcrumbs and active query.
   */
  public function formatFilter($filter, $islandora_solr_query, $date_format = '') {		// date_format: e.g. "Y-m-d"
    // @todo See how this interacts with multiple date filters.
    // Check if there are operators in the filter.
    $fq_split = preg_split('/ (OR|AND|NOT) /', $filter);
    $fq_split_full = preg_split('/( OR | AND | NOT |(?<!\\\)\ )/', $filter);
    if (count($fq_split) > 1) {
      $operator_split = preg_split(ISLANDORA_SOLR_QUERY_SPLIT_REGEX, $filter);
      $operator_split = array_diff($operator_split, $fq_split_full);
      $out_array = array();
      foreach ($fq_split as $fil) {
        $fil_split = preg_split(ISLANDORA_SOLR_QUERY_FIELD_VALUE_SPLIT_REGEX, $fil, 2);
        $out_str = str_replace(array('"', 'info:fedora/'), '', $fil_split[1]);
        $out_array[] = $out_str;
      }
      $filter_string = '';
      foreach ($out_array as $out) {
        $filter_string .= $out;
        if (count($operator_split)) {
          $filter_string .= ' ' . array_shift($operator_split) . ' ';
        }
      }
      $filter_string = trim($filter_string);
    }
    else {
      if ( empty($date_format) ) { $date_format = "Y"; }
      // Split the filter into field and value, for e.g. array( 'mods_originInfo_encoding_w3cdtf_keyDate_yes_dateIssued_dt', '[2007-01-01T00:00:00Z TO 2012-01-01T00:00:00Z]' )
      $filter_split = preg_split(ISLANDORA_SOLR_QUERY_FIELD_VALUE_SPLIT_REGEX, $filter, 2);
      // Trim brackets.
      $filter_split[1] = trim($filter_split[1], "\"");
      // If value is date.
      if (isset($islandora_solr_query->solrParams['facet.date']) && in_array(ltrim($filter_split[0], '-'), $islandora_solr_query->solrParams['facet.date'])) {
        // Check date format setting.
        foreach ($this->rangeFacets as $value) {
          if ($value['solr_field'] == $filter_split[0] && isset($value['solr_field_settings']['date_facet_format']) && !empty($value['solr_field_settings']['date_facet_format'])) {
            $format = $value['solr_field_settings']['date_facet_format'];
          }
        }
        // Split range filter string to return formatted date values.
        if ( empty($filter_split[1]) || empty(rtrim($filter_split[1],'*')) ) {
          $filter_split[1] = ( variable_get('islandora_solr_facets_year_only',true) ? 'Any Year' : 'Any Date' );  // if only a '*', rather special case...
        } else {
          $yearRange = array_map( function($y) { return trim($y,"[] \n\r\t\v\0"); }, explode(' TO ',$filter_split[1]) );
          $yearRange = array(	/* we do not want the timezone to be affected, so better use date_create()+date_format() than strtotime()+date() */
            0 => ( ( !empty($yearRange[0]) && substr($yearRange[0],0,1) != '*' ) ? date_format(date_create($yearRange[0]),$date_format) : 'since ever' ),
            1 => ( ( !empty($yearRange[1]) && substr($yearRange[1],0,1) != '*' ) ? date_format(date_create($yearRange[1]),$date_format) : 'now' ),
          );
          $filter_split[1] = ( strcmp($yearRange[0],$yearRange[1]) ? implode(' - ',$yearRange) : $yearRange[0] );
        }
      }
      $filter_string = $filter_split[1];
    }
    $filter_string = str_replace('*:* NOT','NOT',$filter_string);	// the *:* before NOT may be Solr-logically needed, for human readability it's not
    return stripslashes($filter_string);
  }
}
