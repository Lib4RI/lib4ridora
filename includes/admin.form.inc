<?php
/**
 * @file
 * Admin form.
 */

/**
 * Admin for for lib4ridora.
 */
function lib4ridora_admin(array $form, array &$form_state) {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
  $textfield_size = 128;		// Default 60, but sometimes too short even for some text field values.

  $lib4ridora_multi_pdf_options = lib4ridora_multi_pdf_get_options( TRUE /* true to get the PDF link order as required for Lib4RI */ );
  $lib4ridora_multi_pdf_options_availability = $lib4ridora_multi_pdf_options['availability'];
  $lib4ridora_multi_pdf_options_availability_exclude = array('date' => $lib4ridora_multi_pdf_options_availability['date']);
  $lib4ridora_multi_pdf_options_availability_effective = array_diff_key($lib4ridora_multi_pdf_options_availability, $lib4ridora_multi_pdf_options_availability_exclude);
  $form['lib4ridora_multi_pdf_defaults_property_availability'] = array(
    '#type' => 'select',
    '#title' => t('Default PDF availability at ingest'),
    '#options' => $lib4ridora_multi_pdf_options_availability_effective,
    '#default_value' => variable_get('lib4ridora_multi_pdf_defaults_property_availability', 'intranet'),
    '#description' => t("Choose the default availability of ingested PDFs. Default is \"" . $lib4ridora_multi_pdf_options_availability['intranet'] . "\"."),
    '#multiple' => FALSE,
    '#required' => TRUE,
  );
  $form['lib4ridora_multi_pdf_link_order'] = array(	/* according to http://lib-dora-dev1.emp-eaw.ch:3000/issues/79 : Updated, Published, Accepted, Supplemental, Erratum, Unspecified */
    '#type' => 'textfield',
    '#title' => t('Link order of PDF versions'),
    '#default_value' => variable_get('lib4ridora_multi_pdf_link_order', implode( ", ", array_map( function($ver) { return strtok(ltrim($ver),' '); }, $lib4ridora_multi_pdf_options['document_version'] ) ) ),
    '#description' => t("Comma-separated list of PDF versions to define the order when displayed (case-insensitive, leading part is sufficient):<br>- " . implode( "<br>- ", $lib4ridora_multi_pdf_options['document_version'] ) . "<br>The 'Updated Version' has been removed to match the belongs of Lib4RI." ),
    '#required' => FALSE,
    '#size' => $textfield_size,
  );
  if ( @isset($_GET['all']) || @isset($_GET['add']) ) {
	  $form['lib4ridora_recent_additions_amount'] = array(
		'#type' => 'textfield',
		'#title' => t('Amount of Recent Additions'),
		'#default_value' => min( max( variable_get('lib4ridora_recent_additions_amount', 15), 2 ), 100 ),
		'#description' => t("Number to define how many of the recently added publications will be listed when you click on the 'Browse' link."),
		'#required' => FALSE,
	    '#size' => 160,
	  );
  }
  if ( @isset($_GET['all']) || @isset($_GET['dl']) ) {			// related to Redmine issue #67 / http://lib-dora-dev1.emp-eaw.ch:3000/issues/67
	  $form['lib4ridora_pdf_download_name_tuning'] = array(
		'#type' => 'textfield',
		'#title' => t('Option list for PDF download name'),
		'#default_value' => variable_get('lib4ridora_pdf_download_name_tuning','+3,_'),
		'#description' => t("Comma-separated list: [1] how many words of the title to use ('+' for cropping dots), [2] optional string to separate the filename parts, [3] optional space replacement. Quotes are OK for easier space/comma/empty-string handling."),
		'#required' => FALSE,
	    '#size' => 160,
	  );
  }
  $form['lib4ridora_restricted_datastreams'] = array(
    '#type' => 'textfield',
    '#title' => t('Restricted DSIDs'),
    '#default_value' => variable_get('lib4ridora_restricted_datastreams', 'RELS-INT,RELS-EXT,POLICY'),
    '#description' => t("Comma-separated list of datastream-IDs that should not be viewable for anonymous users. Default is: \"RELS-INT,RELS-EXT,POLICY\"."),
    '#required' => FALSE,
    '#size' => $textfield_size,
  );
/* if ( @isset($_GET['all']) || @isset($_GET['ip']) ) {		// IPv*4* assignments for intranet, see lib4ridora.module, function lib4ridora_check_ip(). The origin of the IPs is unknown - up-to-date!?
	  $form['lib4ridora_intranet_ip_range_list'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Intranet IPv4 restriction'),
	    '#description' => t('If the publication is available on the intranet only, user\'s IP must be in one of the specified ranges (a comma-seperated list, slash allowed on the last IP part to indicate min/max).'),
	    '#default_value' => variable_get( 'lib4ridora_intranet_ip_range_list', "129.129.0.0/16, 152.88.0.0/16, 192.33.118.0/24, 193.134.200.0/21, 195.176.244.0/23" ),
	    '#required' => TRUE,
	    '#size' => 160,
	  );
  } */
  $form['lib4ridora_author_fullname_solr_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Author Full Name Solr Field'),
    '#description' => t("A field used that stores the author's full name in MADS."),
    '#default_value' => variable_get('lib4ridora_author_fullname_solr_field', 'MADS_fullname_ms'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_author_solr_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Author PID Solr Field'),
    '#default_value' => variable_get('lib4ridora_author_solr_field', 'mods_name_personal_nameIdentifier_authorId_ms'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_mods_query_author_id'] = array(
      '#type' => 'textfield',
      '#title' => t('Author MODS xpath query'),
      '#description' => t(''),
      '#default_value' => variable_get('lib4ridora_mods_query_author_id', 'mods:nameIdentifier[@type="authorId"]'),
      '#required' => TRUE,
      '#size' => $textfield_size,
  );
  $form['lib4ridora_issn_solr_field'] = array(
    '#type' => 'textfield',
    '#title' => t('ISSN Solr Field'),
    '#default_value' => variable_get('lib4ridora_issn_solr_field', 'mods_identifier_issn_s'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_e_issn_solr_field'] = array(
    '#type' => 'textfield',
    '#title' => t('E-ISSN Solr Field'),
    '#default_value' => variable_get('lib4ridora_e_issn_solr_field', 'mods_identifier_e-issn_s'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_solr_field_article_host_journal'] = array(
    '#type' => 'textfield',
    '#title' => t('Article Journal Solr Field'),
    '#description' => t('A Solr field on articles identifying to which journal it belongs. Should be the journal PID. (used in CSV export)'),
    '#default_value' => variable_get('lib4ridora_solr_field_article_host_journal', 'mods_relatedItem_host_identifier_ms'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_solr_field_article_date_issued'] = array(
    '#type' => 'textfield',
    '#title' => t('Article Date Issued Solr Field'),
    '#description' => t('A Solr field on articles containing (at least) the year in which the article was issued. (used in CSV export)'),
    '#default_value' => variable_get('lib4ridora_solr_field_article_date_issued', 'mods_originInfo_dateIssued_ms'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_pseudo_solr_field_factor'] = array(
    '#type' => 'textfield',
    '#title' => t('Journal Impact Factor Pseudo-Solr Field'),
    '#description' => t('A field we will inject into Solr documents during CSV exports. If restricting fields, ensure it is available.'),
    '#default_value' => variable_get('lib4ridora_pseudo_solr_field_factor', 'journal_impact_factor'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_pseudo_solr_field_year'] = array(
    '#type' => 'textfield',
    '#title' => t('Journal Impact Factor Year Pseudo-Solr Field'),
    '#description' => t('A field we will inject into Solr documents during CSV exports. If restricting fields, ensure it is available.'),
    '#default_value' => variable_get('lib4ridora_pseudo_solr_field_year', 'journal_impact_factor_year'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  if ( @isset($_GET['all']) || @isset($_GET['impact']) ) {			// Even if we skip this configuration, the variable defaults will do what we need - until reconfiguring it here.
	  $form['lib4ridora_pseudo_solr_field_factor_failed'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Journal Impact Factor failure term for export'),
	    '#description' => t('Terms in CSV export if Journal Impact Factor evaluation failed (originally \'N/A\'). May be comma-separated to have different terms for factor and year field (quotes are allowed).'),
	    '#default_value' => variable_get('lib4ridora_pseudo_solr_field_factor_failed', ''),
	    '#required' => FALSE,
	    '#size' => 160,
	  );
	  $form['lib4ridora_pseudo_solr_field_factor_none_yet'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Journal Impact Factor term replacement for export'),
	    '#description' => t('You may replace given terms from the corresponding datastream resp. from the imported data file by a custom one (syntax: old1 = new1, old2 = new2, ...).'),
	    '#default_value' => variable_get('lib4ridora_pseudo_solr_field_factor_none_yet', 'Not available = not available'),
	    '#required' => FALSE,
	    '#size' => 160,
	  );
  }
  $form['lib4ri_citation_export_form_placeholder'] = array(		/*  for citations in Excel/CSV exports */
    '#type' => 'textfield',
    '#title' => t("Citation Pseudo-Solr Field"),
    '#description' => t("Placeholder where to put citations into CSV/Ecxel exports. To be added as 'display field' at: /admin/islandora/search/islandora_solr/settings"),
    '#default_value' => variable_get('lib4ri_citation_export_form_placeholder','mods_citation_export_placeholder_lib4ri'),
    '#required' => FALSE,
    '#size' => ( ( @isset($_GET['all']) || @isset($_GET['cit']) ) ? 128 : $textfield_size ),
  );
  $form['lib4ridora_solr_field_publication_year'] = array(
    '#type' => 'textfield',
    '#title' => t('Publication Year Solr Field'),
    '#description' => t('A field we use to determine publication year in Solr.  Is used by the advanced search block.'),
    '#default_value' => variable_get('lib4ridora_solr_field_publication_year', 'mods_originInfo_encoding_w3cdtf_keyDate_yes_dateIssued_dt'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_solr_field_reporting_year'] = array(
    '#type' => 'textfield',
    '#title' => t('Reporting Year Solr Field'),
    '#description' => t('A field we use to determine reporting year in Solr.  Is used by the advanced search block.'),
    '#default_value' => variable_get('lib4ridora_solr_field_reporting_year', 'mods_originInfo_encoding_w3cdtf_type_reporting year_dateOther_dt'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_solr_field_publication_type'] = array(
    '#type' => 'textfield',
    '#title' => t('Publication Type Solr Field'),
    '#description' => t('A field we use to determine publication type in Solr.  Is used by the advanced search block.'),
    '#default_value' => variable_get('lib4ridora_solr_field_publication_type', 'mods_genre_ms'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_solr_field_affiliations'] = array(
    '#type' => 'textfield',
    '#title' => t('Affiliations Solr Fields'),
    '#description' => t('A comma seperated list of fields we use to determine affilitations in Solr.  Is used by the advanced search block.'),
    '#default_value' => variable_get('lib4ridora_solr_field_affiliations', 'mods_name_personal_affiliation_ms,mods_name_corporate_affiliation_ms,mods_name_conference_affiliation_ms'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ri_affiliation_author1_form_export_placeholder'] = array(		/*  for affiliation of the first author, see also http://lib-dora-dev1.emp-eaw.ch:3000/issues/14 */
    '#type' => 'textfield',
    '#title' => t("First Author's Affiliation Pseudo-Solr Field"),
    '#description' => t("Placeholder where to put first author's affiliation into CSV/Ecxel exports. To be added as 'display field' at: /admin/islandora/search/islandora_solr/settings <br>Instead of one value only, this field here can also have two values (comma-separated) to put author name and author's unit into two columns."),
    '#default_value' => variable_get('lib4ri_affiliation_author1_form_export_placeholder','mods_personal_affiliation_author1_placeholder_lib4ri'),
    '#required' => FALSE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_solr_field_peer_review'] = array(
    '#type' => 'textfield',
    '#title' => t('Peer Review Solr Field'),
    '#description' => t('A field we use to determine peer review status in Solr.  Is used by the advanced search block.'),
    '#default_value' => variable_get('lib4ridora_solr_field_peer_review', 'mods_note_peer review_ms'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_solr_field_document_version'] = array(
    '#type' => 'textfield',
    '#title' => t('Document Version Solr Field'),
    '#description' => t('A field we use to determine peer review status in Solr.  Is used by the advanced search block.'),
    '#default_value' => variable_get('lib4ridora_solr_field_document_version', 'RELS_INT_lib4ridora-multi-embargo-document_version_literal_ms'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_solr_field_document_availability'] = array(
    '#type' => 'textfield',
    '#title' => t('Document Availability Solr Field'),
    '#description' => t('A field we use to determine document availability in Solr.  Is used by the advanced search block.'),
    '#default_value' => variable_get('lib4ridora_solr_field_document_availability', 'RELS_EXT_fullText_literal_ms'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_solr_field_document_doi'] = array(
    '#type' => 'textfield',
    '#title' => t('DOI Solr Field'),
    '#description' => t('A field we use to determine DOI in Solr.  Is used for duplicity check during ingest via DOI.'),
    '#default_value' => variable_get('lib4ridora_solr_field_document_doi', 'mods_identifier_doi_mlt'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  if ( @isset($_GET['all']) || @isset($_GET['search']) ) {
    $form['lib4ridora_solr_search_extension_field_data'] = array(
      '#type' => 'textfield',
      '#title' => t('Expanded search: Solr Field Data'),
  //  '#description' => t("Accepts a special list, example: 'field1|field2 + fieldA' means that the search in 'field1' or in 'field2' both will be expanded by searching in 'fieldA' too."),
      '#description' => t("Intension/Example: There is SolrField1 and SolrField2, and for both of them also the values in SolrFieldX should be checked. <br>Resulting syntax: SolrField1|SolrField2 + SolrFieldX"),
      '#default_value' => variable_get('lib4ridora_solr_search_extension_field_data','dc.creator|lib4ri_all_authors_mt + mods_extension_originalAuthorList_mt'),
      '#required' => FALSE,
      '#size' => 160,
    );
  }
  if ( @isset($_GET['all']) || @isset($_GET['prepo']) ) {			// 'prepo' ~= label prepositions, see also http://lib-dora-dev1.emp-eaw.ch:3000/issues/57
    $form['lib4ridora_solr_field_org_terms_1on1'] = array(
      '#type' => 'textfield',
      '#title' => t('Organizational unit name parts untouched'),
      '#description' => t('A comma seperated list of name parts where the first letter shall not be upper-cased (affects ingest form and adv. search).'),
      '#default_value' => variable_get('lib4ridora_solr_field_org_terms_1on1', 'or, and, for, of, in, at'),
      '#required' => FALSE,
      '#size' => 160,
    );
  }
  if ( @isset($_GET['all']) || @isset($_GET['css']) ) {			// 'prepo' ~= label prepositions, see also http://lib-dora-dev1.emp-eaw.ch:3000/issues/57
    $form['lib4ridora_css_block_inert_iconless'] = array(
      '#type' => 'textfield',
      '#title' => t('CSS class for block text'),
      '#description' => t('A space or comma seperated list of CSS classes to use for text labels in 1st/2nd sidebar without leading icons.'),
      '#default_value' => variable_get('lib4ridora_css_block_inert_iconless', 'availability-text'),
      '#required' => FALSE,
      '#size' => 160,
    );
  }
  // related to Redmine issue #189 / http://lib-dora-dev1.emp-eaw.ch:3000/issues/189 (about HTML tags in page title)
  $form['lib4ridora_page_title_tag_fix'] = array(
    '#type' => 'checkbox',
    '#title' => t("Enable HTML tag tuning in web pages?"),
    '#description' => t("Activate work-around to decode accidentially encoded HTML tags (related to HTML-editing of publications)."),
    '#default_value' => variable_get('lib4ridora_page_title_tag_fix', TRUE),
    '#required' => FALSE,
    '#prefix' => "<a name='html'><!-- to jump here --></a><span style='font-size:10pt; position:relative; top:4px'><b>Layout</b>",
    '#suffix' => "</span><br>",
  );
  $form['lib4ridora_config_dummy_line'] = array(
    '#type' => 'radios',
    '#title' => t(''),
    '#options' => array(),
    '#required' => FALSE,
  );
  return system_settings_form($form);
}
