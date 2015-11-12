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

function lib4ridora_organizational_block_configuration_form() {
  $organizations = lib4ridora_get_organization_facets();
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
