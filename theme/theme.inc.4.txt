<?php
/**
 * @file
 * Theme hooks.
 */

/**
 * Implements hook_preprocess_lib4ridora_pdf_materials().
 */
function template_preprocess_lib4ridora_pdf_materials(&$variables) {
  $object = $variables['object'];
  module_load_include('inc', 'lib4ridora', 'includes/utilities');
  $pdfs = lib4ridora_retrieve_filtered_pdfs($object);
  $variables['pdf_listing'] = array();
  foreach ($pdfs as $pdf) {
    $variables['pdf_listing'][$pdf->id] = array(
      '#theme' => 'lib4ridora_pdf_link',
      '#datastream' => $pdf,
      '#statement' => $variables['statement'],
    );
  }
}

/**
 * Implements hook_process_lib4ridora_pdf_materials().
 */
function template_process_lib4ridora_pdf_materials(&$variables) {
  $variables['pdf_listing'] = drupal_render($variables['pdf_listing']);
}

/**
 * Implements hook_preprocess_lib4ridora_pdf_link().
 */
function template_preprocess_lib4ridora_pdf_link(&$variables) {
  $values = lib4ridora_get_embargo_info($variables['datastream']);
  $access = $values['availability'];

  $variables['pdf_link'] = $values;
  $variables['availability_classes'] = array('availability-text');

  if ($access == 'public') {
    $variables['span_classes_array'][] = 'fa fa-unlock-alt';
  }
  elseif ($access == 'date') {
    $embargo = gmmktime(0, 0, 0, $values['embargo_date']['month'], $values['embargo_date']['day'], $values['embargo_date']['year']);

    if (REQUEST_TIME > $embargo) {
      $variables['span_classes_array'][] = 'fa fa-unlock-alt';
    }
    else {
      // If IP is internal show the 'yellow' lock, otherwise use the standard
      // grey color.
      $variables['span_classes_array'][] = lib4ridora_check_ip() ? 'fa fa-unlock-alt yellow' : 'fa fa-lock';
    }
  }
  elseif (lib4ridora_check_ip() && $access == 'intranet') {
    $variables['span_classes_array'][] = 'fa fa-unlock-alt yellow';
  }
  else {
    $variables['span_classes_array'][] = 'fa fa-lock';
  }
}

/*
 * Implements function to proprietarily compose the download name of the link PDF file.
 * Related to Redmine issue #67 / http://lib-dora-dev1.emp-eaw.ch:3000/issues/67
 *
 * There will be a 'hidden' configuration available at/with: admin/islandora/solution_pack_config/lib4ridora?dl
 */
function lib4ridora_download_name_pdf($pid,$fileExt,$docVer,$dsId) {
	$title = "Lib4RI-" . strtr(rawurldecode($pid),":","-");		// aux
	if( ( $object = islandora_object_load($pid) ) && @isset($object['MODS']) ) {
	    $mods = $object['MODS']->content;
		$dom = new DOMDocument();
		$dom->loadXML($mods);
		$xpath = new DOMXPath($dom);
		$xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');

		$confAry = str_getcsv( variable_get('lib4ridora_download_name_tuning','+3,_') );
		$sep = ( preg_match('/[<>\\/:\*\|\?]+/',$confAry[1]) ? "_" : $confAry[1] );		// it can be a space too, so do not trim!
		$name = "";
		$year = "";
		$version = "";
		$titLimit = max( abs(intval($confAry[0])), 1 );				// use abs(), since if negative may become a meaning here
		$titCustom = 0;
		if ( intval($confAry[0]) < 0 ) { $titCustom = -1; }
		elseif ( substr(ltrim($confAry[0]),0,1) == "+" ) { $titCustom = 1; }			// to enable custum tunings for the naming.

		if ( $query = $xpath->query('//mods:mods/mods:name[@type="personal"]/mods:namePart[@type="family"]') ) {
			$name = trim( strip_tags(html_entity_decode( $query->item(0)->nodeValue )) );
		}

		if ( $query = $xpath->query('//mods:mods/mods:titleInfo/mods:title') ) {
			$title = trim( strtok( strip_tags(html_entity_decode( $query->item(0)->nodeValue )).":", ":" ) );		// hard-coded: only until the next semi-colon
			$title = trim( str_replace("\n","; ",strtr($title,"<>\\/:\*\|\?\t\r","{}_--',~  ")) );
		}

		$titAry = explode(" ",$title);
		if ( sizeof($titAry) > ( $titLimit - min($titCustom,0) + 1 ) ) {		// if any customization allowed then +1 for 1 word 'hard-coded' tolerance
			$title = implode(" ",array_slice($titAry,0,$titLimit)) . ( ( $titCustom < 1 ) ? "" : "..." );		// hard-coded '...' to indicate that we cut the title.
		}

		if ( $query = $xpath->query('//mods:mods/mods:originInfo/mods:dateIssued') ) {
			if ( $year = $query->item(0)->nodeValue ) {				// let's keep it simple, it's usually just a (year) number, rarely something like 2015-12-23
				$year = ltrim( strip_tags(html_entity_decode($year)), "/-: \t\n\r\0\x0B" );
				$year = rtrim( strtok(strtr($year,"/:","--")."-","-") );
			}
		}

		// In case there are multiple DS with identical 'document version' (assuming this is won't happen for published or accepted versions)
		// then let's add a specific number or letter to keep the download names different.
		$version = $docVer;
		/* However here a snippet when we need to examine RELS-INT for a more precise itendifying number/letter, unsure however what happends if datastream ID would change.
		    $dom->loadXml( $object['RELS-INT']->content );
		    $xpath = new DOMXPath($dom);
			$xpath->registerNamespace('rdf', "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
			$xpath->registerNamespace('islandora', "http://islandora.ca/ontology/relsint#");
			$query = $xpath->query('/rdf:RDF/rdf:Description/@rdf:about');
		*/
		if ( !stristr($docVer,"published") && !stristr($docVer,"accepted") ) {			// assuming there will be only one accepted/published version...
			// note: for Lib4ri publications the $object[$dsId]->checksum is not assigned ('none' will be returned). So let's compose an own one:
			$hash = dechex( crc32( strval( $object[$dsId]->label ) ) );
			$version .= " " . $hash;		// after a space, since there may be spaces too inside the doc.ver. term.
		}
		if ( $titCustom >= 0 ) { $version = "(" . $version . ")"; }			// hard-coded: document version inside brackets if customization allowed

		// replace space character if requested by the 2nd config parameter:
		$title = $name . $sep . ( empty($year) ? "" : $year.$sep ) . $title . $sep . $version;
		if ( @isset($confAry[2]) ) {
			if ( empty($confAry[2]) ) {			// may happen if whithing quotes
				$title = str_replace( " ", "", ucwords($title,"[{(_.,;!')}] \t\r\n\f\v") );
			}
			elseif ( @!empty(trim($confAry[2])) && !preg_match('/[<>\\/:\*\|\?]+/',trim($confAry[2])) ) {		// here safer if we trim, since it's not a space
				$title = str_replace( " ", trim($confAry[2]), $title );
			}
		}
	}
	return( $title . "." . ( empty($fileExt) ? "pdf" : $fileExt ) );		// = actually the new DL name now.
}

/**
 * Implements hook_process_lib4ridora_pdf_link().
 */
function template_process_lib4ridora_pdf_link(&$variables) {
  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
  $values = $variables['pdf_link'];
  if ($variables['statement']) {
    $availability = $values['availability'];
    if ($availability == 'intranet') {
      $availability_text = t('Intranet Only');
    }
    elseif ($availability == 'date') {
      // Need to get a human readable month.
      $date = new DateTime();
      $embargo = gmmktime(0, 0, 0, $values['embargo_date']['month'], $values['embargo_date']['day'], $values['embargo_date']['year']);
      // Check if embargo date has passed.
      if (REQUEST_TIME > $embargo) {
        $availability_text = t('Open Access');
      }
      else {
        $availability_text = t('Intranet Only until @day @month @year', array(
          '@day' => $values['embargo_date']['day'],
          '@month' => date_format($date->createFromFormat('!m', $values['embargo_date']['month']), 'F'),
          '@year' => $values['embargo_date']['year'],
        ));
      }
    }
    elseif ($availability == 'private') {
      $availability_text = t('Private');
    }
    else {
      $availability_text = t('Open Access');
    }
    $variables['availability'] = $availability_text;

  }
  $human_readable = lib4ridora_multi_pdf_get_options();
  $human_version = $human_readable['document_version'];
  
  // PDF downloand: let's assign a download name for the PDF that is as meaningful as possible.
  // Refers to Redmine issue #67 http://lib-dora-dev1.emp-eaw.ch:3000/issues/67
  $pid = $variables['datastream']->parent->id;
  if ( $ext = @trim(strchr($variables['datastream']->mimeType,"/"),"/") ) {
    $ext = strtolower($ext);
  } elseif ( $ext = @trim(strchr(substr($variables['datastream']->label,-5),"."),".") ) {
    $ext = strtolower($ext);		// assumed the last 5 signs/letters of the DS label are carrying an extension (for PDFs this is currently the case).
  } else {
    $ext = strtolower($variables['datastream']->id );		// aux, to be tuned by MIME type detection of DS
  }
  $dl_name = lib4ridora_download_name_pdf( $pid, $ext, $values['document_version'], $variables['datastream']->id );
  $dl_name = preg_replace('/[?*<\\/>:]+/', '-', strtr($dl_name,'"',"'") );		// rather for safety
  $variables['title_link'] = array(
    'title' => $human_version[$values['document_version']],
    'url' => url("islandora/object/{$variables['datastream']->parent->id}/datastream/{$variables['datastream']->id}/{$dl_name}"),
  );
}

/**
 * Implements hook_preprocess_lib4ridora_citation_solr_results().
 */
function template_preprocess_lib4ridora_citation_solr_results(&$variables) {
  foreach ($variables['citations'] as &$citation) {
    $citation['pdf_listing'] = array(
      '#theme' => 'lib4ridora_pdf_materials',
      '#object' => islandora_object_load($citation['pid']),
    );
  }
}

/**
 * Implements hook_process_lib4ridora_citation_solr_results().
 */
function template_process_lib4ridora_citation_solr_results(&$variables) {
  foreach ($variables['citations'] as &$citation) {
    $citation['pdf_listing'] = drupal_render($citation['pdf_listing']);
  }
}

/**
 * Implements hook_preprocess_HOOK().
 */
function template_preprocess_lib4ridora_islandora_solr_metadata_injected_author_info(&$variables) {
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  if ($variables['info']['author']['id']) {
    $variables['author_attributes']['href'] = array(
      'islandora/search',
      array(
        'query' => array(
          'f' => array(
            format_string('!field:"!value"', array(
              '!field' => islandora_solr_lesser_escape(variable_get('lib4ridora_author_solr_field', 'mods_name_personal_nameIdentifier_authorId_ms')),
              '!value' => $variables['info']['author']['id'],
            )),
          ),
        ),
      ),
    );
  }
  if ($variables['info']['org']['id']) {
    $variables['org_attributes']['href'] = array(
      'islandora/search',
      array(
        'query' => array(
          'f' => array(
            format_string('!field:"!value"', array(
              '!field' => islandora_solr_lesser_escape('mods_name_personal_author_authority_marcrelator_nameIdentifier_organizational unit id_ms'),
              '!value' => $variables['info']['org']['id'],
            )),
          ),
        ),
      ),
    );
  }
}

/**
 * Implements hook_process_THEME().
 */
function template_process_lib4ridora_islandora_solr_metadata_injected_author_info(&$variables) {
  if (isset($variables['author_attributes']['href'])) {
    $variables['author_attributes']['href'] = call_user_func_array('url', $variables['author_attributes']['href']);
  }
  if (isset($variables['org_attributes']['href'])) {
    $variables['org_attributes']['href'] = call_user_func_array('url', $variables['org_attributes']['href']);
  }
}
