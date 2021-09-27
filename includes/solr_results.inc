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

    // Get user provided filter parameters.
    $fq = isset($islandora_solr_query->internalSolrParams['f']) ? $islandora_solr_query->internalSolrParams['f'] : array();
    // Parameters set in URL.
    $params = $islandora_solr_query->internalSolrParams;
    // Get query values.
    if (!in_array($islandora_solr_query->solrQuery, $islandora_solr_query->differentKindsOfNothing)) {
      // Get query value.
      $query_value = $islandora_solr_query->solrQuery;
      $query_value_human = $query_value;
      $search_fields = islandora_solr_get_fields("search_fields", FALSE, FALSE);
      foreach ($search_fields as $key => $value){
        $query_value_human = preg_replace("/" . $value['solr_field'] . "/", $value['solr_field_settings']['label'], $query_value_human);
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
      $attr_minus['title'] = t('Remove') . ' ' . $query_value;
      $attr_minus['class'] = array('remove-query');
      $attr_minus['rel'] = 'nofollow';
      $attr_minus['href'] = url($path_minus, array('query' => $query_minus));

      $hooks = islandora_build_hook_list(ISLANDORA_SOLR_FACET_BUCKET_CLASSES_HOOK_BASE);
      drupal_alter($hooks, $attributes, $islandora_solr_query);

      // XXX: We are not using l() because of active classes:
      // @see http://drupal.org/node/41595
      // Create link.
      $query_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a> ' . check_plain($query_value_human);

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
    if (!empty($fq)) {
      // Gather the human readable labels of the available facets.
      $records = islandora_solr_get_fields("facet_fields", FALSE, FALSE);

      // Set list variables.
      $filter_list = array();
      foreach ($fq as $key => $filter) {
        // Check for exclude filter.
        if ($filter[0] == '-') {
          // Not equal sign.
          $symbol = '&ne;';
        }
        else {
          $symbol = '=';
        }
        $filter_string = $this->formatFilter($filter, $islandora_solr_query);
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
        $attr_minus =& $attributes['minus']['attr'];
        $attr_minus['title'] = t('Remove') . ' ' . $filter;
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
        $updated_filter = FALSE;
		// spacial case: if there is an explit given search string then just use this:
        if ( $str = @trim(strip_tags($_GET['search_string'])) ) {  //html tags are stripped again to prevent html injection
          $filter_string = htmlentities($str);
        } else {
          foreach ($records as $key => $value) {
            // Re-write certain PIDs to name strings.
            if (strpos(preg_replace('/\\\\ /', ' ', $filter), $value['solr_field']) !== FALSE) {
              $updated_filter = TRUE;
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
              $filter_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a>' . " {$value['solr_field_settings']['label']} " . $symbol . " " . check_plain($filter_string);
            }
          }
        }
        // Default to old behavior.
        if (!$updated_filter) {
          $filter_list[] = '<a' . drupal_attributes($attributes['minus']['attr']) . '>(-)</a> &hellip; ' . $symbol . ' ' . check_plain($filter_string);
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
    }
    return $output;
  }

  /**
   * Formats the passed in filter into a human readable form.
   *
   * @param string $filter
   *   The passed in filter.
   * @param object $islandora_solr_query
   *   The current Solr Query
   *
   * @return string
   *   The formatted filter string for breadcrumbs and active query.
   */
  public function formatFilter($filter, $islandora_solr_query) {
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
      // Split the filter into field and value.
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
        $filter_str = $filter_split[1];
        $filter_str = trim($filter_str, '[');
        $filter_str = trim($filter_str, ']');
        $filter_array = explode(' TO ', $filter_str);
        $filter_split[1] = format_date(strtotime(trim($filter_array[0])) + (60 * 60 * 24), 'custom', $format) . ' - ' . format_date(strtotime(trim($filter_array[1])) - (60 * 60 * 24), 'custom', $format);
      }
      $filter_string = $filter_split[1];
    }
    return stripslashes($filter_string);
  }
}
