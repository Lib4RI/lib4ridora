<?php

/**
 * @file
 * Contains the admin form and callback functions for datastream manipulations supporting/tuning exclusively the islandora_usage_stats module.
 */


/**
 * Helper function for lib4ridora_provide_datastream
 *
 */
function lib4ridora_helper_datastream(AbstractDatastream $datastream, $download = FALSE, $version = NULL, $info = array() ) {
  if ( stripos($datastream->mimetype,"/pdf") ) {
    return lib4ridora_view_datastream($datastream, $download, $version, $info);		// for Lib4RI specific PDF file names
  }
  module_load_include('inc', 'islandora', 'includes/datastream');
  return islandora_view_datastream($datastream, $download, $version);
}

/**
 * Callback to download the given datastream to the users computer.
 *
 * @param AbstractDatastream $datastream
 *   The datastream to view/download.
 * 
 * Note: This function is the same as islandora_download_datastream(), however expanded to support the counting of downloads.
 */
function lib4ridora_provide_datastream(AbstractDatastream $datastream, $download = FALSE, $version = NULL) {
  if ( user_is_logged_in() ) {
    $urlNow = @trim( strtok( " ".$_SERVER['HTTP_REFERER']."?" , "?" ) );
    $ignPages = trim( variable_get('lib4ridora_download_count_ignore_page','/manage/, /datastream/, /lib4ridora_pdf_list') );
    if ( !empty($urlNow) && !empty($ignPages) && substr($ignPages,0,1) != "#" ) {
      // Generally do not do anything special if we are an editor/admin managing PDFs/datastreams:
      foreach( str_getcsv($ignPages) as $page ) {
        if ( stripos($urlNow,trim($page)) !== false ) {
          module_load_include('inc', 'islandora', 'includes/datastream');
          return islandora_view_datastream($datastream, $download, $version);		// so we get the Islandora's original file names.
        }
      }
    }
  }

  if ( !module_exists("islandora_usage_stats") ) {
    return lib4ridora_helper_datastream($datastream, $download, $version);
  }

  $fileTypeOk = trim(variable_get('lib4ridora_download_count_file_type','application/pdf'));
  if ( !empty($fileTypeOk) && substr($fileTypeOk,0,1) != "#" ) {
    foreach( str_getcsv($fileTypeOk) as $type ) {
      if ( stripos($datastream->mimetype,trim($type)) !== false ) { $fileTypeOk = ""; break; }
    }
    if ( !empty($fileTypeOk) ) {
      return lib4ridora_helper_datastream($datastream, $download, $version);
    }
  }

  $ignUserRoles = trim( variable_get('lib4ridora_download_count_ignore_role','editor, repo manager, administrator') );
  if ( !empty($ignUserRoles) && substr($ignUserRoles,0,1) != "#" ) {
    global $user;
    foreach(str_getcsv($ignUserRoles) as $role) {
      if ( in_array(trim(strtolower($role)),array_values($user->roles)) ) {
        return lib4ridora_helper_datastream($datastream, $download, $version);
      }
    }
  }

  // Get a proprietary list of code keywords to know/influence what to do:
  $filterList = trim( strtolower( variable_get('lib4ridora_download_count_skip_list', /* 'pdf/a,private' */ "" ) ) );

  // Try to count the download now. Assuming here the mime type is PDF (although no real need to restrict it explicitly):
  $dsInfo = array();
  if ( !variable_get('lib4ridora_download_count_req_language',false) || @!empty(trim($_SERVER['HTTP_ACCEPT_LANGUAGE'])) ) {
    $docVerOk = trim(variable_get('lib4ridora_download_count_doc_version','published, accepted'));
    if ( !stripos($datastream->mimetype,"/pdf") || empty($docVerOk) || substr($docVerOk,0,1) == "#" ) {
      module_load_include('inc', 'islandora_usage_stats', 'includes/utilities'); 
      if ( function_exists('islandora_usage_stats_log_datastream_download') ) {
        islandora_usage_stats_log_datastream_download($datastream->parent->id, $datastream->id);
      }
    } else {  // for PDFs only
      $pdfa_suffix = variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A');		// option not to count PDF/A files
      if ( strpos($filterList,"pdf/a") === false || substr(strval($datastream->id),0-strlen($pdfa_suffix)) != $pdfa_suffix ) {
        module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
        $dsInfo = lib4ridora_get_embargo_info($datastream);
        if ( strpos($filterList,"private") === false || stripos($dsInfo['availability'],"private") === false ) {
          if ( !empty($docVerOk) && substr($docVerOk,0,1) != "#" ) {
            foreach( str_getcsv($docVerOk) as $docVer ) {
              if ( stripos($dsInfo['document_version'],trim($docVer)) !== false ) { $docVerOk = ""; break; }
            }
          }
          if ( empty($docVerOk) || substr($docVerOk,0,1) == "#" ) {
            module_load_include('inc', 'islandora_usage_stats', 'includes/utilities'); 
            if ( function_exists('islandora_usage_stats_log_datastream_download') ) {
              islandora_usage_stats_log_datastream_download($datastream->parent->id, $datastream->id);
            }
          }
        }
      }
    }
  }
  return lib4ridora_helper_datastream($datastream, $download, ( stripos($datastream->mimetype,"/pdf") ? NULL : $version ), $dsInfo);
}

/**
 * Callback function to view or download a datastream.
 *
 * @note
 *  This function calls exit().
 *
 * @param AbstractDatastream $datastream
 *   The datastream to view/download.
 * @param bool $download
 *   If TRUE the file is download to the user computer for viewing otherwise it
 *   will attempt to display in the browser natively.
 * @param int $version
 *   The version of the datastream to display
 */
function lib4ridora_view_datastream(AbstractDatastream $datastream, $download = FALSE, $version = NULL, $dsInfo = array() ) {
  module_load_include('inc', 'islandora', 'includes/mimetype.utils');
  module_load_include('inc', 'islandora', 'includes/datastream');
  // XXX: Certain features of the Devel module rely on the use of "shutdown
  // handlers", such as query logging... The problem is that they might blindly
  // add additional output which will break things if what is actually being
  // output is anything but a webpage... like an image or video or audio or
  // whatever the datastream is here.
  $GLOBALS['devel_shutdown'] = FALSE;

  if ($version !== NULL) {
    if (isset($datastream[$version])) {
      $datastream = $datastream[$version];
    }
    else {
      return drupal_not_found();
    }
  }
  header('Content-Type: ' . $datastream->mimetype);
  if ($datastream->controlGroup == 'M' || $datastream->controlGroup == 'X') {
    header('Content-Length: ' . $datastream->size);
  }
  if ( $download ) {
    // Browsers will not append all extensions.
    $label = $datastream->label;
    $extension = '.' . islandora_get_extension_for_mimetype($datastream->mimetype);
    // Prevent adding on a duplicate extension.
    $filename = $label . ( ( strtolower(strrchr($label,'.')) == strtolower($extension) ) ? "" : $extension );

    if ( ( $pid = strval($datastream->parent->id) ) && strpos($pid,':') ) {		// trivial check is OK
	  if ( empty($dsInfo) ) {
        module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
		$dsInfo = lib4ridora_get_embargo_info($datastream);
	  }
	  module_load_include('inc', 'lib4ridora', 'theme/theme');
	  $filename = lib4ridora_download_name_pdf( $pid, ltrim($extension,'.'), $dsInfo['document_version'], $datastream->id );
	}
    header("Content-Disposition: attachment; filename=\"" . basename($filename) . "\"");
  }

  $cache_check = islandora_view_datastream_cache_check($datastream);
  if ($cache_check !== 200) {
    if ($cache_check === 304) {
      header('HTTP/1.1 304 Not Modified');
    }
    elseif ($cache_check === 412) {
      header('HTTP/1.0 412 Precondition Failed');
    }
  }
  islandora_view_datastream_set_cache_headers($datastream);

  drupal_page_is_cacheable(FALSE);

  // New content needed.
  if ($cache_check === 200) {
    // We need to see if the chunking is being requested. This will mainly
    // happen with iOS video requests as they do not support any other way
    // to receive content for playback.
    $chunk_headers = FALSE;
    if (isset($_SERVER['HTTP_RANGE'])) {
      // XXX: Can't make assertions on byte ranging of redirect datastreams.
      // @see https://jira.duraspace.org/browse/ISLANDORA-2084.
      if (!$download && $datastream->controlGroup == 'R') {
        drupal_goto($datastream->url);
      }
      // Set headers specific to chunking.
      $chunk_headers = islandora_view_datastream_set_chunk_headers($datastream);
    }
    // Try not to load the file into PHP memory!
    // Close and flush ALL the output buffers!
    while (@ob_end_flush()) {
    };

    if ( !isset($_SERVER['HTTP_RANGE']) ) {
      $datastream->getContent('php://output');
    }
    elseif ($chunk_headers) {
      islandora_view_datastream_deliver_chunks($datastream, $chunk_headers);
    }
  }
  exit();
}
