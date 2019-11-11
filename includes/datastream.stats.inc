<?php

/**
 * @file
 * Contains the admin form and callback functions for datastream manipulations supporting/tuning exclusively the islandora_usage_stats module.
 */

/**
 * Callback to download the given datastream to the users computer.
 *
 * @param AbstractDatastream $datastream
 *   The datastream to download.
 * 
 * Note: This function is the same as islandora_download_datastream(), however expanded to support the counting of downloads.
 */
function lib4ridora_download_datastream(AbstractDatastream $datastream) {
  $dsInfo = array();
  if ( module_exists("islandora_usage_stats") ) {
    $fileTypeOk = trim(variable_get('lib4ridora_download_count_file_type','application/pdf'));
    foreach( str_getcsv($fileTypeOk) as $type ) {
      if ( stripos($datastream->mimetype,trim($type)) !== false ) { $fileTypeOk = ""; break; }
    }
    if ( empty($fileTypeOk) || substr($fileTypeOk,0,1) == "#" ) {
      $pdfa_suffix = variable_get('lib4ridora_extra_pdfa_datastream_suffix','_PDF-A');
      if ( !substr(strval($datastream->id),0-strlen($pdfa_suffix)) != $pdfa_suffix ) {		// do not count PDF/A files
        module_load_include('inc', 'lib4ridora', 'includes/utilities');
        $dsInfo = lib4ridora_get_embargo_info($datastream);
        if ( stripos($dsInfo['availability'],"private") === false ) {		// do not count private files/datastreams
          $docVerOk = trim(variable_get('lib4ridora_download_count_doc_version','published,accepted'));
          foreach( str_getcsv($docVerOk) as $docVer ) {
            if ( stripos($dsInfo['document_version'],trim($docVer)) !== false ) { $docVerOk = ""; break; }
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
  lib4ridora_view_datastream($datastream, TRUE, NULL, $dsInfo);
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
  header('Content-type: ' . $datastream->mimetype);
  if ($datastream->controlGroup == 'M' || $datastream->controlGroup == 'X') {
    header('Content-length: ' . $datastream->size);
  }
  if ($download) {
    // Browsers will not append all extensions.
    $extension = '.' . islandora_get_extension_for_mimetype($datastream->mimetype);
    // Prevent adding on a duplicate extension.
    $label = $datastream->label;
    $extension_length = strlen($extension);
    $duplicate_extension_position = strlen($label) > $extension_length ?
      strripos($label, $extension, -$extension_length) :
      FALSE;

    $filename = $label;
    if ( $pid = strval($datastream->parent->id) ) {
	  if ( empty($dsInfo) ) {
        module_load_include('inc', 'lib4ridora', 'includes/utilities');
		$dsInfo = lib4ridora_get_embargo_info($datastream);
	  }
	  module_load_include('inc', 'lib4ridora', 'theme/theme');
	  $ext = ($duplicate_extension_position === FALSE) ? trim($extension,'.') : "";
	  $filename = lib4ridora_download_name_pdf( $pid, $ext, $dsInfo['document_version'], $datastream->id );
	}
    elseif ($duplicate_extension_position === FALSE) {
      $filename .= $extension;
    }
    header("Content-Disposition: attachment; filename=\"$filename\"");
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

    if (isset($_SERVER['HTTP_RANGE'])) {
      if ($chunk_headers) {
        islandora_view_datastream_deliver_chunks($datastream, $chunk_headers);
      }
    }
    else {
      $datastream->getContent('php://output');
    }
  }
  exit();
}
