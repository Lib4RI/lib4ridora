<?php

/**
 * @file
 * Default views hook.
 */

/**
 * Implements hook_views_default_views().
 */
function lib4ridora_views_default_views() {
  $views = array();

  $view = new view();
  $view->name = 'lib4ridora_document_management';
  $view->description = '';
  $view->tag = 'default';
  $view->base_table = 'islandora_solr';
  $view->human_name = 'lib4ridora_document_management';
  $view->core = 7;
  $view->api_version = '3.0';
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

  /* Display: Master */
  $handler = $view->new_display('default', 'Master', 'default');
  $handler->display->display_options['use_more_always'] = FALSE;
  $handler->display->display_options['access']['type'] = 'none';
  $handler->display->display_options['cache']['type'] = 'none';
  $handler->display->display_options['query']['type'] = 'views_query';
  $handler->display->display_options['exposed_form']['type'] = 'basic';
  $handler->display->display_options['pager']['type'] = 'some';
  $handler->display->display_options['pager']['options']['offset'] = '0';
  $handler->display->display_options['style_plugin'] = 'default';
  $handler->display->display_options['row_plugin'] = 'fields';
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing']['id'] = 'nothing';
  $handler->display->display_options['fields']['nothing']['table'] = 'views';
  $handler->display->display_options['fields']['nothing']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing']['label'] = '';
  $handler->display->display_options['fields']['nothing']['alter']['text'] = 'Edit MODS';
  $handler->display->display_options['fields']['nothing']['alter']['make_link'] = TRUE;
  $handler->display->display_options['fields']['nothing']['alter']['path'] = 'islandora/object/!1/datastream/MODS/edit';
  $handler->display->display_options['fields']['nothing']['element_label_colon'] = FALSE;
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing_1']['id'] = 'nothing_1';
  $handler->display->display_options['fields']['nothing_1']['table'] = 'views';
  $handler->display->display_options['fields']['nothing_1']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing_1']['label'] = '';
  $handler->display->display_options['fields']['nothing_1']['alter']['text'] = 'Deposit License';
  $handler->display->display_options['fields']['nothing_1']['alter']['make_link'] = TRUE;
  $handler->display->display_options['fields']['nothing_1']['alter']['path'] = 'deposit-license';
  $handler->display->display_options['fields']['nothing_1']['element_label_colon'] = FALSE;
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing_2']['id'] = 'nothing_2';
  $handler->display->display_options['fields']['nothing_2']['table'] = 'views';
  $handler->display->display_options['fields']['nothing_2']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing_2']['label'] = '';
  $handler->display->display_options['fields']['nothing_2']['alter']['text'] = 'PDF Upload';
  $handler->display->display_options['fields']['nothing_2']['alter']['make_link'] = TRUE;
  $handler->display->display_options['fields']['nothing_2']['alter']['path'] = 'islandora/object/!1/lib4ridora_pdf_upload';
  $handler->display->display_options['fields']['nothing_2']['element_label_colon'] = FALSE;
  /* Field: Global: Custom text */
  $handler->display->display_options['fields']['nothing_3']['id'] = 'nothing_3';
  $handler->display->display_options['fields']['nothing_3']['table'] = 'views';
  $handler->display->display_options['fields']['nothing_3']['field'] = 'nothing';
  $handler->display->display_options['fields']['nothing_3']['label'] = '';
  $handler->display->display_options['fields']['nothing_3']['alter']['text'] = 'PDF Management';
  $handler->display->display_options['fields']['nothing_3']['alter']['make_link'] = TRUE;
  $handler->display->display_options['fields']['nothing_3']['alter']['path'] = 'islandora/object/!1/lib4ridora_pdf_management';
  $handler->display->display_options['fields']['nothing_3']['element_label_colon'] = FALSE;
  /* Contextual filter: Islandora Solr: PID */
  $handler->display->display_options['arguments']['PID']['id'] = 'PID';
  $handler->display->display_options['arguments']['PID']['table'] = 'islandora_solr';
  $handler->display->display_options['arguments']['PID']['field'] = 'PID';
  $handler->display->display_options['arguments']['PID']['default_action'] = 'default';
  $handler->display->display_options['arguments']['PID']['default_argument_type'] = 'raw';
  $handler->display->display_options['arguments']['PID']['default_argument_options']['index'] = '2';
  $handler->display->display_options['arguments']['PID']['summary']['number_of_records'] = '0';
  $handler->display->display_options['arguments']['PID']['summary']['format'] = 'default_summary';
  $handler->display->display_options['arguments']['PID']['summary_options']['items_per_page'] = '25';

  /* Display: Block */
  $handler = $view->new_display('block', 'Block', 'block');
  $translatables['lib4ridora_document_management'] = array(
    t('Master'),
    t('more'),
    t('Apply'),
    t('Reset'),
    t('Sort by'),
    t('Asc'),
    t('Desc'),
    t('Edit MODS'),
    t('Deposit License'),
    t('PDF Upload'),
    t('PDF Management'),
    t('All'),
    t('Block'),
  );

  $views['lib4ridora_document_management'] = $view;

  $view = new view();
  $view->name = 'publication_links';
  $view->description = '';
  $view->tag = 'default';
  $view->base_table = 'islandora_solr';
  $view->human_name = 'Publication Links';
  $view->core = 7;
  $view->api_version = '3.0';
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

  /* Display: Master */
  $handler = $view->new_display('default', 'Master', 'default');
  $handler->display->display_options['title'] = 'Links';
  $handler->display->display_options['css_class'] = 'libfourri-pub-links';
  $handler->display->display_options['use_more_always'] = FALSE;
  $handler->display->display_options['access']['type'] = 'none';
  $handler->display->display_options['cache']['type'] = 'none';
  $handler->display->display_options['query']['type'] = 'views_query';
  $handler->display->display_options['exposed_form']['type'] = 'basic';
  $handler->display->display_options['pager']['type'] = 'some';
  $handler->display->display_options['pager']['options']['items_per_page'] = '5';
  $handler->display->display_options['style_plugin'] = 'default';
  $handler->display->display_options['row_plugin'] = 'fields';
  /* Field: Islandora Solr: mods_identifier_doi_s */
  $handler->display->display_options['fields']['mods_identifier_doi_s']['id'] = 'mods_identifier_doi_s';
  $handler->display->display_options['fields']['mods_identifier_doi_s']['table'] = 'islandora_solr';
  $handler->display->display_options['fields']['mods_identifier_doi_s']['field'] = 'mods_identifier_doi_s';
  $handler->display->display_options['fields']['mods_identifier_doi_s']['label'] = '';
  $handler->display->display_options['fields']['mods_identifier_doi_s']['alter']['alter_text'] = TRUE;
  $handler->display->display_options['fields']['mods_identifier_doi_s']['alter']['text'] = '<a href="http://doi.org/[mods_identifier_doi_s]" target="_blank">View at Publisher (DOI)</a>';
  $handler->display->display_options['fields']['mods_identifier_doi_s']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['mods_identifier_doi_s']['hide_empty'] = TRUE;
  $handler->display->display_options['fields']['mods_identifier_doi_s']['link_to_object'] = 0;
  /* Field: Islandora Solr: mods_identifier_uri_s */
  $handler->display->display_options['fields']['mods_identifier_uri_s']['id'] = 'mods_identifier_uri_s';
  $handler->display->display_options['fields']['mods_identifier_uri_s']['table'] = 'islandora_solr';
  $handler->display->display_options['fields']['mods_identifier_uri_s']['field'] = 'mods_identifier_uri_s';
  $handler->display->display_options['fields']['mods_identifier_uri_s']['label'] = '';
  $handler->display->display_options['fields']['mods_identifier_uri_s']['alter']['alter_text'] = TRUE;
  $handler->display->display_options['fields']['mods_identifier_uri_s']['alter']['text'] = '<a href="[mods_identifier_uri_s]" target="_blank">View at Publisher (URL)</a>';
  $handler->display->display_options['fields']['mods_identifier_uri_s']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['mods_identifier_uri_s']['hide_empty'] = TRUE;
  $handler->display->display_options['fields']['mods_identifier_uri_s']['link_to_object'] = 0;
  /* Field: Islandora Solr: mods_identifier_ut_s */
  $handler->display->display_options['fields']['mods_identifier_ut_s']['id'] = 'mods_identifier_ut_s';
  $handler->display->display_options['fields']['mods_identifier_ut_s']['table'] = 'islandora_solr';
  $handler->display->display_options['fields']['mods_identifier_ut_s']['field'] = 'mods_identifier_ut_s';
  $handler->display->display_options['fields']['mods_identifier_ut_s']['label'] = '';
  $handler->display->display_options['fields']['mods_identifier_ut_s']['alter']['alter_text'] = TRUE;
  $handler->display->display_options['fields']['mods_identifier_ut_s']['alter']['text'] = '<a href="http://ws.isiknowledge.com/cps/openurl/service?url_ver=Z39.88-2004&rft_id=info:ut/[mods_identifier_ut_s]" target="_blank">View at Web of Science</a>';
  $handler->display->display_options['fields']['mods_identifier_ut_s']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['mods_identifier_ut_s']['hide_empty'] = TRUE;
  $handler->display->display_options['fields']['mods_identifier_ut_s']['link_to_object'] = 0;
  /* Field: Islandora Solr: mods_identifier_scopus_s */
  $handler->display->display_options['fields']['mods_identifier_scopus_s']['id'] = 'mods_identifier_scopus_s';
  $handler->display->display_options['fields']['mods_identifier_scopus_s']['table'] = 'islandora_solr';
  $handler->display->display_options['fields']['mods_identifier_scopus_s']['field'] = 'mods_identifier_scopus_s';
  $handler->display->display_options['fields']['mods_identifier_scopus_s']['label'] = '';
  $handler->display->display_options['fields']['mods_identifier_scopus_s']['alter']['alter_text'] = TRUE;
  $handler->display->display_options['fields']['mods_identifier_scopus_s']['alter']['text'] = '<a href="http://www.scopus.com/scopus/openurl/link.url?ctx_ver=Z39.88-2004&rft_id=info:eid/[mods_identifier_scopus_s]" target="_blank">View at Scopus</a>';
  $handler->display->display_options['fields']['mods_identifier_scopus_s']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['mods_identifier_scopus_s']['hide_empty'] = TRUE;
  $handler->display->display_options['fields']['mods_identifier_scopus_s']['link_to_object'] = 0;
  /* Contextual filter: Islandora Solr: PID */
  $handler->display->display_options['arguments']['PID']['id'] = 'PID';
  $handler->display->display_options['arguments']['PID']['table'] = 'islandora_solr';
  $handler->display->display_options['arguments']['PID']['field'] = 'PID';
  $handler->display->display_options['arguments']['PID']['default_action'] = 'default';
  $handler->display->display_options['arguments']['PID']['default_argument_type'] = 'raw';
  $handler->display->display_options['arguments']['PID']['default_argument_options']['index'] = '2';
  $handler->display->display_options['arguments']['PID']['summary']['number_of_records'] = '0';
  $handler->display->display_options['arguments']['PID']['summary']['format'] = 'default_summary';
  $handler->display->display_options['arguments']['PID']['summary_options']['items_per_page'] = '25';
  /* Filter criterion: Islandora Solr: RELS_EXT_hasModel_uri_s */
  $handler->display->display_options['filters']['RELS_EXT_hasModel_uri_s']['id'] = 'RELS_EXT_hasModel_uri_s';
  $handler->display->display_options['filters']['RELS_EXT_hasModel_uri_s']['table'] = 'islandora_solr';
  $handler->display->display_options['filters']['RELS_EXT_hasModel_uri_s']['field'] = 'RELS_EXT_hasModel_uri_s';
  $handler->display->display_options['filters']['RELS_EXT_hasModel_uri_s']['value'] = 'info:fedora/ir:citationCModel';
  /* Filter criterion: Islandora Solr: Query Parser (Lucene) */
  $handler->display->display_options['filters']['query_lucene']['id'] = 'query_lucene';
  $handler->display->display_options['filters']['query_lucene']['table'] = 'islandora_solr';
  $handler->display->display_options['filters']['query_lucene']['field'] = 'query_lucene';
  $handler->display->display_options['filters']['query_lucene']['value'] = 'mods_identifier_doi_s:* OR mods_identifier_uri_s:* OR mods_identifier_ut_s:* OR mods_identifier_scopus_s:*';

  /* Display: Block */
  $handler = $view->new_display('block', 'Block', 'block');
  $translatables['publication_links'] = array(
    t('Master'),
    t('Links'),
    t('more'),
    t('Apply'),
    t('Reset'),
    t('Sort by'),
    t('Asc'),
    t('Desc'),
    t('<a href="http://doi.org/[mods_identifier_doi_s]" target="_blank">View at Publisher (DOI)</a>'),
    t('<a href="[mods_identifier_uri_s]" target="_blank">View at Publisher (URL)</a>'),
    t('<a href="http://ws.isiknowledge.com/cps/openurl/service?url_ver=Z39.88-2004&rft_id=info:ut/[mods_identifier_ut_s]" target="_blank">View at Web of Science</a>'),
    t('<a href="http://www.scopus.com/scopus/openurl/link.url?ctx_ver=Z39.88-2004&rft_id=info:eid/[mods_identifier_scopus_s]" target="_blank">View at Scopus</a>'),
    t('All'),
    t('Block'),
  );
  $views['lib4ridora_publication_links'] = $view;
  return $views;
}
