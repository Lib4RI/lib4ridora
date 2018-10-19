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
  $lib4ridora_multi_pdf_options = lib4ridora_multi_pdf_get_options();
  $lib4ridora_multi_pdf_options_availability = $lib4ridora_multi_pdf_options['availability'];
  $lib4ridora_multi_pdf_options_availability_exclude = array('date' => $lib4ridora_multi_pdf_options_availability['date']);
  $lib4ridora_multi_pdf_options_availability_effective = array_diff_key($lib4ridora_multi_pdf_options_availability, $lib4ridora_multi_pdf_options_availability_exclude);
  $textfield_size = 72;		// Default 60, but sometimes too short even for some Solr field names.
  $form['lib4ridora_multi_pdf_defaults_property_availability'] = array(
    '#type' => 'select',
    '#title' => t('Default PDF availability at ingest'),
    '#options' => $lib4ridora_multi_pdf_options_availability_effective,
    '#default_value' => variable_get('lib4ridora_multi_pdf_defaults_property_availability', 'intranet'),
    '#description' => t("Choose the default availability of ingested PDFs. Default is \"" . $lib4ridora_multi_pdf_options_availability['intranet'] . "\"."),
    '#multiple' => FALSE,
    '#required' => TRUE,
  );
  $form['lib4ridora_restricted_datastreams'] = array(
    '#type' => 'textfield',
    '#title' => t('Restricted DSIDs'),
    '#default_value' => variable_get('lib4ridora_restricted_datastreams', 'RELS-INT,RELS-EXT,POLICY'),
    '#description' => t("Comma separated list of datastream-IDs that should not be viewable for anonymous users. Default is: \"RELS-INT,RELS-EXT,POLICY\"."),
    '#required' => FALSE,
    '#size' => $textfield_size,
  );
  if ( @isset($_GET['all']) || @isset($_GET['ip']) ) {		// IP assignments for intranet configurable now, see lib4ridora.module, function lib4ridora_check_ip(), ...but no idea who originally assigned this IP ranges.
	  $form['lib4ridora_ip_range_list'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Intranet IP restriction'),
	    '#description' => t('If the publication is available on the intranet only, user\'s IP must be in one of the specified ranges (a comma-seperated list, slash allowed on the last IP part to indicate min/max).'),
	    '#default_value' => variable_get( 'lib4ridora_ip_range_list', "129.129.0.0/16, 152.88.0.0/16, 192.33.118.0/24, 193.134.200.0/21, 195.176.244.0/23" ),
	    '#required' => TRUE,
	    '#size' => 128,
	  );
  }
  $form['lib4ridora_author_solr_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Author PID Solr Field'),
    '#default_value' => variable_get('lib4ridora_author_solr_field', 'mods_name_personal_nameIdentifier_authorId_ms'),
    '#required' => TRUE,
    '#size' => $textfield_size,
  );
  $form['lib4ridora_author_fullname_solr_field'] = array(
    '#type' => 'textfield',
    '#title' => t('Author Full Name Solr Field'),
    '#description' => t("A field used that stores the author's full name in MADS."),
    '#default_value' => variable_get('lib4ridora_author_fullname_solr_field', 'MADS_fullname_ms'),
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
  if ( @isset($_GET['all']) || @isset($_GET['impact']) ) {		// Even if we skip this configuration, the variable defaults will do what we need - until we reconfigure it.
	  $form['lib4ridora_pseudo_solr_field_factor_failed'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Journal Impact Factor failure term for export'),
	    '#description' => t('Terms in CSV export if Journal Impact Factor evaluation failed (originally \'N/A\'). May be comma-separated to have different terms for factor and year field.'),
	    '#default_value' => variable_get('lib4ridora_pseudo_solr_field_factor_failed', ''),
	    '#required' => FALSE,
	    '#size' => 128,
	  );
	  $form['lib4ridora_pseudo_solr_field_factor_none_yet'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Journal Impact Factor term replacement for export'),
	    '#description' => t('You may replace given terms from the corresponding datastream resp. from the imported data file by a custom one (syntax: old1 = new1, old2 = new2, ...).'),
	    '#default_value' => variable_get('lib4ridora_pseudo_solr_field_factor_none_yet', 'Not available = not available'),
	    '#required' => FALSE,
	    '#size' => 128,
	  );
  }
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
  if ( @isset($_GET['all']) || @isset($_GET['aa1']) ) {		// aa1: affiliation of author #1, see also http://lib-dora-dev1.emp-eaw.ch:3000/issues/14
	  $form['lib4ri_affiliation_author1_form_export_placeholder'] = array(
	    '#type' => 'textfield',
	    '#title' => t("First Author's Affiliation Pseudo-Solr Field"),
	    '#description' => t("Placeholder where to put first author's affiliation in CSV/Ecxel exports. To be added as 'display field' at: /admin/islandora/search/islandora_solr/settings"),
	    '#default_value' => variable_get('lib4ri_affiliation_author1_form_export_placeholder','mods_name_personal_author_affiliation_1_placeholder_lib4ri'),
	    '#required' => FALSE,
	    '#size' => ( ( @isset($_GET['all']) || @isset($_GET['aa1']) ) ? 128 : $textfield_size ),
	  );
  }
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
  if ( @isset($_GET['all']) || @isset($_GET['prepo']) ) {		// prepo ~= label prepositions, see also http://lib-dora-dev1.emp-eaw.ch:3000/issues/57
	  $form['lib4ridora_solr_field_org_terms_1on1'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Organizational unit name parts untouched'),
	    '#description' => t('A comma seperated list of name parts where the first letter shall not be upper-cased (affects ingest form and adv. search).'),
	    '#default_value' => variable_get('lib4ridora_solr_field_org_terms_1on1', 'or, and, for, of, in, at'),
	    '#required' => FALSE,
	    '#size' => ( ( @isset($_GET['all']) || @isset($_GET['prepo']) ) ? 128 : $textfield_size ),
	  );
  }
  $form['lib4ridora_mods_query_author_id'] = array(
      '#type' => 'textfield',
      '#title' => t('Author MODS xpath query'),
      '#description' => t(''),
      '#default_value' => variable_get('lib4ridora_mods_query_author_id', 'mods:nameIdentifier[@type="authorId"]'),
      '#required' => TRUE,
      '#size' => $textfield_size,
  );
  return system_settings_form($form);
}
