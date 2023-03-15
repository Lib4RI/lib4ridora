<?php

/**
 * @file
 * Block functionality for lib4ridora.
 */

/**
 * Gets a form element for an organization block content.
 *
 * @return array
 *   An array representing an organization block content form element.
 */
function lib4ridora_get_organizational_block_content() {
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  $facets = lib4ridora_get_organization_facets();
  uasort($facets, 'lib4ridora_organizational_facet_sort');
  $items = array();
  foreach ($facets as $facet => $info) {
    $items[] = array(
      'data' => l($info['label'], 'islandora/search/*:*', array(
        'query' => array(
          'f' => array(
            format_string('!field:"!value"', array(
              '!field' => islandora_solr_lesser_escape($info['field']),
              '!value' => islandora_solr_facet_escape($facet),
            )),
          ),
        ),
      )) . "<span class='label label-default'>({$info['count']})</span>",
      'class' => array('list-group-item'),
    );
  }
  return array(
    '#type' => 'item',
    '#theme' => 'item_list',
    '#items' => $items,
    '#attributes' => array(
      'class' => array('list-group'),
    ),
  );
}
/**
 * Custom sort function used in lib4ridora_get_organizational_block_content()
 */
function lib4ridora_organizational_facet_sort($a, $b) {
  return strcmp(strtolower($a['label']),strtolower($b['label']));
}

/**
 * Gets a list of theme info for recently added citations.
 *
 * @return array
 *   Items to be themed into a list of recent citations, containing:
 *   - 'citation': A citation entry for the object,
 *   - 'pid': The PID of the object,
 *   - 'pdfs': an array of PDF datastreams on the object, containing a 'dsid',
 *     a 'version' and a 'class' for rendering.
 */
function lib4ridora_get_recent_additions() {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
  module_load_include('inc', 'lib4ridora', 'includes/pdf_list');
  $style = citeproc_default_style();
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery('*:*');
  $qp->solrParams['fl'] = 'PID';
  $model_field = variable_get('islandora_solr_content_model_field', 'RELS_EXT_hasModel_uri_ms');
  $qp->solrParams['fq'][] = "$model_field:info\:fedora\/ir\:citationCModel";
  $qp->solrParams['sort'] = variable_get('lib4ridora_recent_additions_block_sort_parameter', 'fgs_createdDate_dt desc');
  $qp->solrLimit = min( max( variable_get('lib4ridora_recent_additions_amount', 15), 2 ), 100 );	// = 10;
  $qp->executeQuery(FALSE);
  $items = array();
  foreach ($qp->islandoraSolrResult['response']['objects'] as $item) {
    $object = islandora_object_load($item['PID']);
    if ($object) {
      // Render the citation.
      $mods = islandora_bibliography_get_mods($object->id);
      $entry = citeproc_bibliography_from_mods($style, $mods);
      $items[] = array(
        'citation' => $entry,
        'pid' => $object->id,
      );
    }
  }
  return $items;
}

/**
 * Gets a form for configuring the organization facet block.
 *
 * @return array
 *   Form definition for items in the organization facet block configuration.
 */
function lib4ridora_organizational_block_configuration_form() {
  $form = array(
    'field' => array(
      '#type' => 'textfield',
      '#title' => t('Organization Solr Fields'),
      '#description' => t('Comma-separated names of Solr fields to use to search for organizations.'),
      '#default_value' => variable_get('lib4ridora_organization_block_organization_field', 'mods_name_personal_affiliation_ms, mods_name_corporate_affiliation_ms'),
    ),
    'results_to_omit' => array(
      '#type' => 'textarea',
      '#title' => t('Omitted Organizations'),
      '#description' => t("A list with IDs of organizational units (example: lib4ri-unit:123) to omit from facet results, separated by new lines.<br> After the ID a comment may be appended, separated by a leading '/' or '#'."),
      '#default_value' => variable_get('lib4ridora_organization_block_results_to_omit', ''),
    ),
  );
  return $form;
}

/**
 * Gets a form for configuring the recent additions block.
 *
 * @return array
 *   Form definition for the recent additions block configuration.
 */
function lib4ridora_recent_additions_block_configuration_form() {
  $form = array(
    'sort_field' => array(
      '#type' => 'textfield',
      '#title' => t('Solr Sort Parameter'),
      '#description' => t('A parameter to use for sorting the Solr results.'),
      '#default_value' => variable_get('lib4ridora_recent_additions_block_sort_parameter', 'fgs_createdDate_dt desc'),
    ),
  );
  return $form;
}

/**
 * Returns the facet fields for a Solr facet search on organizations.
 *
 * @return array
 *   An associative array with facets as keys, each containing an array of
 *   'field' as the query field, 'label' as the facet label and 'count' as the facet count.
 */
function lib4ridora_get_organization_facets() {
  // Let us build an index of search fields and see if we need to replace the pid with the label:
  $myrecords = islandora_solr_get_fields("facet_fields", FALSE, FALSE); // get all facet fields...
  $org_fields = array_map("trim", explode(',', variable_get('lib4ridora_organization_block_organization_field', 'mods_name_personal_affiliation_ms, mods_name_corporate_affiliation_ms')));
  $facets = array();
  $omitted_facets = explode(PHP_EOL, variable_get('lib4ridora_organization_block_results_to_omit', '') );
  $omitted_facets = array_map( function($uTmp) { return trim(strtok(strtr($uTmp,'|#','//').'/','/')); }, $omitted_facets );  // to allow appending '//' comments/unit-name onto each line/unit-id 
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery('*:*');
  $qp->solrParams['fl'] = "PID, " . implode(', ', $org_fields);
  $qp->solrParams['facet.limit'] = 100000;
  $qp->solrParams['facet.mincount'] = 1;
  $qp->solrParams['facet.field'] = $org_fields;
  $qp->executeQuery(FALSE);
  // Remark. Weird naming convention: '$organization' actually contains the search field, whereas '$facet' refers to the name... We keep it as is, although it probably should be the other way around...
  foreach ($qp->islandoraSolrResult['facet_counts']['facet_fields'] as $organization => $results) {
    foreach ($results as $facet => $count) {
      if (!in_array($facet, $omitted_facets)) {
        $facet_alt = $facet;
        // Check if we want to translate the pid into the label and do it
        foreach ($myrecords as $mykey => $myval) {
          if ((strpos($organization, $myval['solr_field']) !== FALSE) && islandora_is_valid_pid($facet) && isset($myval['solr_field_settings']['pid_object_label']) && $myval['solr_field_settings']['pid_object_label']) {
            $myqp = new IslandoraSolrQueryProcessor();
            $myqp->buildQuery("PID:\"$facet\"");
            $myqp->solrParams['fl'] = "PID, " . variable_get('islandora_solr_object_label_field', 'fgs_label_s');
            $myqp->executeQuery();
            if (!empty($myqp->islandoraSolrResult) && !empty($myqp->islandoraSolrResult['response']['objects']) && !empty($myqp->islandoraSolrResult['response']['object'][0]['object_label'])) {
              $facet_alt = $myqp->islandoraSolrResult['response']['objects'][0]['object_label'];
            }
            else {
              if ($myobj = islandora_object_load($facet)) {
                $facet_alt = $myobj->label;
              }
            }
          }
        }
        $facets[$facet] = array(
          'field' => $organization,
          'label' => $facet_alt,
          'count' => $count,
        );
      }
    }
  }
  return $facets;
}

/**
 * Helper function to query to get the related items.
 *
 * @param AbstractObject $object
 *   An AbstractObject representing an object within Fedora.
 *
 * @return bool|array
 *   An array containing URLs to be rendered or FALSE if MODS does not exist.
 */
function lib4ridora_get_related_research_data(AbstractObject $object) {
  if (isset($object['MODS'])) {
    $dom = new DOMDocument();
    $dom->loadXML($object['MODS']->content);
    $xpath = new DOMXPath($dom);
    $xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
//  $results = $xpath->query('//mods:relatedItem[@type="otherType"]/mods:identifier[@type="uri"]');
    $ary = array_map('trim',explode('|',variable_get('lib4ridora_solr_field_research_data', 'mods_extension_resource_identifier_mt | mods:extension/mods:resource/mods:identifier[@identifierTyp="DOI"]')));
    if ( @empty($ary[1]) /* at [1] there should be the MODS-X-Path */ ) {
      return FALSE;
    }
    $results = $xpath->query('//' . trim($ary[1],"/ \r\n\t\v\x00") );
    // Filter out empty results now.
    $filtered_results = array();
    foreach ($results as $result) {
      if (!empty($result->nodeValue)) {
        $filtered_results[] = $result->nodeValue;
      }
    }
    return $filtered_results;
  }
  return FALSE;
}

/**
 * Renders the markup for related research data.
 *
 * @param DOMNodeList $results
 *   A DOMNodeList of DOMElements to be rendered.
 *
 * @return array
 *   An array representing the markup for the block
 */
function lib4ridora_get_related_research_data_block_content($results) {
  $markup = array();
  foreach ($results as $key => $result) {
    $markup["research_$key"] = array(
      '#theme' => 'link',
      '#text' => format_plural(count($results), 'View Research Data', 'View Research Data (!count)', array('!count' => $key + 1)),
      '#path' => $result,
      '#options' => array(
        'attributes' => array('target' => '_blank'),
      ),
      "#prefix" => '<div class="libfourri-research-data"><span>',
      '#suffix' => '</span></div>',
    );
  }
  return $markup;
}
