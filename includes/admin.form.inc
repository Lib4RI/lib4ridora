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

  $lib4ridora_multi_pdf_options = lib4ridora_multi_pdf_get_options();
  unset($lib4ridora_multi_pdf_options['availability']['date']);
  $default = variable_get('lib4ridora_multi_pdf_defaults_property_availability', 'intranet');
  global $user;
  $roles = $user->roles;
  
  if (in_array('administrator', $roles)){
    $sessionName = 'lib4ridora_advanced_conf_options' . '_' . strval($user->name);
    $form['lib4ridora_advanced_conf_options'] = array(
        '#type' => 'checkbox',
        '#default_value' => @boolval( $_SESSION[$sessionName] ),
        '#title' => t('Show developer\'s configuration options on this page'),
  		'#prefix' => "<a name='html'><!-- to jump here --></a><span style='font-size:10pt; position:relative; left:-0.25em; background-color:#ffefdc;'>&nbsp;<b>Advanced Configuration</b>&nbsp;</span>",
		'#suffix' => "<br></br>",
    );
  }
  $form['lib4ridora_multi_pdf_defaults_property_availability'] = array(
    '#type' => 'select',
    '#title' => t('Default PDF availability at ingest'),
    '#options' => $lib4ridora_multi_pdf_options['availability'],
    '#default_value' => $default,
    '#description' => t("Choose the default availability of PDFs to ingest. Default is \"" . $lib4ridora_multi_pdf_options['availability'][$default] . "\"."),
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
  
	  $form['lib4ridora_recent_additions_amount'] = array(
		'#type' => 'textfield',
		'#title' => t('Amount of Recent Additions'),
		'#default_value' => min( max( variable_get('lib4ridora_recent_additions_amount', 15), 2 ), 100 ),
		'#description' => t("Number to define how many of the recently added publications will be listed when you click on the 'Browse' link."),
		'#required' => FALSE,
        '#states' => array(
          'visible' => array(
              ':input[name="lib4ridora_advanced_conf_options"]' => array('checked' => TRUE),
          ),
        ),
		'#size' => $textfield_size,
	    '#attributes' => array('style' => 'background-color:#ffefdc;'),
	  );
  
  // related to Redmine issue #67 / http://lib-dora-dev1.emp-eaw.ch:3000/issues/67
	  $form['lib4ridora_pdf_download_name_tuning'] = array(
		'#type' => 'textfield',
		'#title' => t('Option list for PDF download name'),
		'#default_value' => variable_get('lib4ridora_pdf_download_name_tuning','+3,_'),
		'#description' => t("Comma-separated list: [1] how many words of the title to use ('+' for cropping dots), [2] optional string to separate the filename parts, [3] optional space replacement. Quotes are OK for easier space/comma/empty-string handling."),
		'#required' => FALSE,
        '#states' => array(
          'visible' => array(
              ':input[name="lib4ridora_advanced_conf_options"]' => array('checked' => TRUE),
          ),
        ),
		'#size' => $textfield_size,
	    '#attributes' => array('style' => 'background-color:#ffefdc;'),
	  );
  
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
  	// Even if we skip this configuration, the variable defaults will do what we need - until reconfiguring it here.
	  $form['lib4ridora_pseudo_solr_field_factor_failed'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Journal Impact Factor failure term for export'),
	    '#description' => t('Terms in CSV export if Journal Impact Factor evaluation failed (originally \'N/A\'). May be comma-separated to have different terms for factor and year field (quotes are allowed).'),
	    '#default_value' => variable_get('lib4ridora_pseudo_solr_field_factor_failed', ''),
	    '#required' => FALSE,
	      '#states' => array(
	          'visible' => array(
	              ':input[name="lib4ridora_advanced_conf_options"]' => array('checked' => TRUE, 'style' => 'background-color:red;'),
	          ),
	      ),
		'#size' => $textfield_size,
	    '#attributes' => array('style' => 'background-color:#ffefdc;'),
	  );
	  $form['lib4ridora_pseudo_solr_field_factor_none_yet'] = array(
	    '#type' => 'textfield',
	    '#title' => t('Journal Impact Factor term replacement for export'),
	    '#description' => t('You may replace given terms from the corresponding datastream resp. from the imported data file by a custom one (syntax: old1 = new1, old2 = new2, ...).'),
	    '#default_value' => variable_get('lib4ridora_pseudo_solr_field_factor_none_yet', 'Not available = not available'),
	    '#required' => FALSE,
	      '#states' => array(
	          'visible' => array(
	              ':input[name="lib4ridora_advanced_conf_options"]' => array('checked' => TRUE),
	          ),
	      ),
		'#size' => $textfield_size,
	    '#attributes' => array('style' => 'background-color:#ffefdc;'),
	  );
  
  $form['lib4ri_citation_export_form_placeholder'] = array(		/*  for citations in Excel/CSV exports */
    '#type' => 'textfield',
    '#title' => t("Citation Pseudo-Solr Field"),
    '#description' => t("Placeholder where to put citations into CSV/Ecxel exports. To be added as 'display field' at: /admin/islandora/search/islandora_solr/settings"),
    '#default_value' => variable_get('lib4ri_citation_export_form_placeholder','mods_citation_export_placeholder_lib4ri'),
    '#required' => FALSE,
//    '#size' => ( ( @isset($_GET['all']) || @isset($_GET['cit']) ) ? 128 : $textfield_size ),
    '#size' => $textfield_size,
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
  
    $form['lib4ridora_solr_search_extension_field_data'] = array(
      '#type' => 'textfield',
      '#title' => t('Expanded search: Solr Field Data'),
      '#description' => t("Intension/Example: There is SolrField1 and SolrField2, and for both of them also the values in SolrFieldX should be checked. <br>Resulting syntax: SolrField1|SolrField2 + SolrFieldX <br>Hint: A leading '#' will make this setting to be ignored."),
      '#default_value' => variable_get('lib4ridora_solr_search_extension_field_data','dc.creator|lib4ri_all_authors_mt + mods_extension_originalAuthorList_mt'),
      '#required' => FALSE,
        '#states' => array(
            'visible' => array(
                ':input[name="lib4ridora_advanced_conf_options"]' => array('checked' => TRUE),
            ),
        ),
		'#size' => $textfield_size,
	    '#attributes' => array('style' => 'background-color:#ffefdc;'),
    );
  
  // 'prepo' ~= label prepositions, see also http://lib-dora-dev1.emp-eaw.ch:3000/issues/57
    $form['lib4ridora_solr_field_org_terms_1on1'] = array(
      '#type' => 'textfield',
      '#title' => t('Organizational unit name parts untouched'),
      '#description' => t('A comma seperated list of name parts where the first letter shall not be upper-cased (affects ingest form and adv. search).'),
      '#default_value' => variable_get('lib4ridora_solr_field_org_terms_1on1', 'or, and, for, of, in, at'),
      '#required' => FALSE,
      '#size' => $textfield_size,
    );
  
  // 'prepo' ~= label prepositions, see also http://lib-dora-dev1.emp-eaw.ch:3000/issues/57
    $form['lib4ridora_css_block_inert_iconless'] = array(
      '#type' => 'textfield',
      '#title' => t('CSS class for block text'),
      '#description' => t('A space or comma seperated list of CSS classes to use for text labels in 1st/2nd sidebar without leading icons.'),
      '#default_value' => variable_get('lib4ridora_css_block_inert_iconless', 'availability-text'),
      '#required' => FALSE,
        '#states' => array(
            'visible' => array(
                ':input[name="lib4ridora_advanced_conf_options"]' => array('checked' => TRUE),
            ),
        ),
		'#size' => $textfield_size,
	    '#attributes' => array('style' => 'background-color:#ffefdc;'),
    );
  
  
    $white_list = array_filter( array_map('trim', explode(',',strtr(variable_get('lib4ridora_bot_white_list'),"|;\n",",,,"))) );
	$form['lib4ridora_bot_white_list'] = array(
		'#type' => 'textarea',
		'#title' => t("Bots: IP white-list to ignore any restrictions"),
		'#description' => t('Comma- or linebreak delimted list of full IPs.<br>Clients with one of these IPs do not need to stand the bot check.'),
		'#default_value' => implode("\n",$white_list),	/* so we finally can enter it with pipes or line-breaks */
		'#required' => FALSE,
		'#cols' => max( $textfield_size >> 1 , 72 ),
		'#rows' => max( sizeof($white_list) + 1, 3 ),
		'#resizable' => TRUE,
	      '#states' => array(
	          'visible' => array(
	              ':input[name="lib4ridora_advanced_conf_options"]' => array('checked' => TRUE),
	          ),
	      ),
	//	'#size' => $textfield_size,
	    '#attributes' => array('style' => 'background-color:#ffefdc;'),
	);

	  $form['lib4ridora_bot_intranet_ok'] = array(
		'#type' => 'checkbox',
		'#title' => t("Bots: Allow any requests from inside Lib4RI intranet?"),
		'#description' => t("If checked/enabled there will be no bot check/restriction of the client's request from from inside Lib4RI's intranet!"),
		'#default_value' => variable_get('lib4ridora_intranet_bot_approved',true),
		'#required' => FALSE,
		'#prefix' => "<a name='html'><!-- to jump here --></a><span style='font-size:10pt; position:relative; top:0.5em; background-color:#ffefdc;'>&nbsp;<b>Bots: Lib4RI's own bots and requests</b>&nbsp;",
		'#suffix' => "</span><br>",
	      '#states' => array(
	          'visible' => array(
	              ':input[name="lib4ridora_advanced_conf_options"]' => array('checked' => TRUE),
	          ),
	      ),
	//	'#size' => $textfield_size,
	    '#attributes' => array('style' => 'background-color:#ffefdc;'),
	  );

      $ua_list = array_filter( array_map('trim', explode(',',strtr(variable_get('lib4ridora_bot_user_agent_list'),"|;\n",",,,"))) );
	  $hint = 'Comma- or linebreak delimted list of terms (case insensitive), if one of them exists in a user-agent string the client/bot is (not) tolerated.';
	  $hint .= "<br>Add a plus sign '+' in front of each term to tolerate the bot, or a minus sign '-' to reject it.";
	  if ( module_exists('islandora_usage_stats') && variable_get('islandora_usage_stats_exclude_bots', '1') ) {
		$hint .= '<br>Please consider updating the <a href="' . url('/admin/islandora/tools/islandora_usage_stats',array('absolute' => true)) . '" target="_blank">Bot Exclusion Regex</a> list';
		$hint .= "<br>from the 'Islandora Usage Stats' module which currently used to provide a bot detection reference.";
	  }
	  $form['lib4ridora_bot_user_agent_list'] = array(
		'#type' => 'textarea',
		'#title' => t("Bots: list for 'user agent' strings to ban or allow"),
		'#description' => t($hint),
		'#default_value' => implode("\n",$ua_list),
		'#required' => FALSE,
		'#size' => $textfield_size,
		'#cols' => max( $textfield_size >> 1 , 72 ),
		'#rows' => max( sizeof($white_list) + 1, 3 ),
		'#resizable' => TRUE,
	      '#states' => array(
	          'visible' => array(
	              ':input[name="lib4ridora_advanced_conf_options"]' => array('checked' => TRUE),
	          ),
	      ),
	//	'#size' => $textfield_size,
	    '#attributes' => array('style' => 'background-color:#ffefdc;'),
	);
  
  
	  // related to Redmine issue #189 / http://lib-dora-dev1.emp-eaw.ch:3000/issues/189 (about HTML tags in page title)
	  $form['lib4ridora_page_title_tag_fix'] = array(
		'#type' => 'checkbox',
		'#title' => t("Enable HTML tag tuning in web pages?"),
		'#description' => t("Activate work-around to decode accidentially encoded HTML tags (related to HTML-editing of publications)."),
		'#default_value' => variable_get('lib4ridora_page_title_tag_fix', TRUE),
		'#required' => FALSE,
		'#prefix' => "<a name='html'><!-- to jump here --></a><span style='font-size:10pt; position:relative; top:0.5em;'><b>Layout</b>",
		'#suffix' => "</span><br>",
        '#states' => array(
            'visible' => array(
                ':input[name="lib4ridora_advanced_conf_options"]' => array('checked' => TRUE),
            ),
        ),
		'#size' => $textfield_size,
	    '#attributes' => array('style' => 'background-color:#ffefdc;'),
	);
  
  $form['lib4ridora_config_dummy_line'] = array(
    '#type' => 'radios',
    '#title' => t(''),
    '#options' => array(),
    '#required' => FALSE,
  );

  return system_settings_form(lib4ridora_admin_form_link_wiki($form));
}

/*
 * Function to attach onto the description text of the form field a link to the Lib4RI-Wiki (if available).
 * Keep/call this at the end of the form setup right before system_settings_form() !
 * 
 * It's intended that the form-field/variable name is expanded by '_wiki' to store the link alias of the Wiki.
 * For example if the wiki-link is https://www.wiki.lib4ri.ch/x/kAH6Ag it could be assinged then with Drush:
 * sudo drush -r /var/www/html -y @sites php-eval 'variable_set("lib4ridora_solr_field_org_terms_1on1_wiki","kAH6Ag")'
*/
function lib4ridora_admin_form_link_wiki($form) {
  foreach( $form as $formField => $fromData ) {
    if ( $wikiAlias = @trim(variable_get($formField.'_wiki','')) ) {
      $wikiLink = ( stripos($wikiAlias,'http') === 0 ) ? $wikiAlias : 'https://www.wiki.lib4ri.ch/x/'.$wikiAlias;
      $form[$formField]['#description'] .= ' <br>Please see <a href="' . $wikiLink . '" target="_blank">further details</a>.';
    }
  }
  return $form;
}

/*
 * Function on submit to to the reset adv. dev options, it will however put it into a session variable/cookie at the user.
 */
function lib4ridora_admin_form_value_check($form, &$form_state) {
  global $user;
  if ( $form['lib4ridora_advanced_conf_options']['#value'] ) {
	$sessionName = 'lib4ridora_advanced_conf_options' . '_' . strval($user->name);
    $_SESSION[$sessionName] = TRUE;
    $form['lib4ridora_advanced_conf_options']['#value'] = FALSE;
  }
}
