<?php

/**
 * @file
 * Block functionality for lib4ridora.
 */

function lib4ridora_get_organizational_block_content() {
  $facets = lib4ridora_get_organization_facets();
  $field = variable_get('lib4ridora_organization_block_organization_field', 'mods_name_personal_affiliation_ms');
  $items = array();
  foreach ($facets as $facet => $count) {
    $items[] = array(
      'data' => l($facet, "islandora/search/$field:($facet)") . "<span class='label label-default'>($count)</span>",
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
  $filter = variable_get('lib4ridora_recent_additions_block_namespace_filter', 'islandora');
  $style = citeproc_default_style();
  $query = $filter == '' ? "*\:*" : "PID:$filter\:*";
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery($query);
  $qp->solrParams['fl'] = 'PID';
  $qp->solrParams['fq'] = array("RELS_EXT_hasModel_uri_ms:info\:fedora\/ir\:citationCModel");
  $qp->solrParams['sort'] = 'fgs_createdDate_dt desc';
  $qp->solrLimit = 5;
  $qp->executeQuery(FALSE);
  $items = array();
  foreach ($qp->islandoraSolrResult['response']['objects'] as $item) {
    $object = islandora_object_load($item['PID']);
    if ($object) {
      // Render the citation.
      $mods = islandora_bibliography_get_mods($object->id);
      $entry = citeproc_bibliography_from_mods($style, $mods);
      // Get the info for the PDFs.
      $datastreams = iterator_to_array($object);
      $pdf_datastreams = array_filter($datastreams, 'lib4ridora_multi_embargo_pdf_filter');
      $pdfs = array();
      foreach ($pdf_datastreams as $pdf_datastream) {
        if (!islandora_datastream_access(ISLANDORA_VIEW_OBJECTS, $pdf_datastream)) {
          continue;
        }
        $version = $pdf_datastream->relationships->get(ISLANDORA_RELS_INT_URI, 'lib4ridora-multi-embargo-document_version');
        $version = array_shift($version);
        if (!is_null($version)) {
          $pdfs[] = array(
            'dsid' => $pdf_datastream->id,
            'version' => $version['object']['value'],
            'id' => 'lib4ri-citation-' . str_replace(' ', '-', $version['object']['value']),
          );
        }
      }
      $items[] = array(
        'citation' => $entry,
        'pid' => $object->id,
        'pdfs' => $pdfs,
      );
    }
  }
  return $items;
}

function lib4ridora_organizational_block_configuration_form() {
  $form = array(
    'namespace' => array(
      '#type' => 'textfield',
      '#title' => t('Namespace Filter'),
      '#description' => t('Enter the namespace to use when filtering PIDs for this site.'),
      '#default_value' => variable_get('lib4ridora_organization_block_namespace_filter', 'islandora'),
    ),
    'field' => array(
      '#type' => 'textfield',
      '#title' => t('Organization Solr Field'),
      '#description' => t('Enter the Solr field to use to search for organizations.'),
      '#default_value' => variable_get('lib4ridora_organization_block_organization_field', 'mods_name_personal_affiliation_ms'),
    ),
  );
  return $form;
}

function lib4ridora_recent_additions_block_configuration_form() {
  return array(
    'namespace' => array(
      '#type' => 'textfield',
      '#title' => t('Namespace Filter'),
      '#description' => t('Enter the namespace to use when filtering PIDs for this site.'),
      '#default_value' => variable_get('lib4ridora_recent_additions_block_namespace_filter', 'islandora'),
    ),
  );
}

function lib4ridora_get_organization_facets() {
  $namespace = variable_get('lib4ridora_organization_block_namespace_filter', 'islandora');
  $query = $namespace == '' ? "*\:*" : "PID:$namespace\:*";
  $org_field = variable_get('lib4ridora_organization_block_organization_field', 'mods_name_personal_affiliation_ms');
  $qp = new IslandoraSolrQueryProcessor();
  $qp->buildQuery($query);
  $qp->solrParams['fl'] = $org_field;
  $qp->solrParams['facet.limit'] = 100000;
  $qp->solrParams['facet.mincount'] = 1;
  $qp->solrParams['facet.field'] = array();
  $qp->solrParams['facet.query'] = array('*');
  $qp->solrParams['facet.field'] = array($org_field);
  $qp->executeQuery(FALSE);
  return $qp->islandoraSolrResult['facet_counts']['facet_fields'][$org_field];
}