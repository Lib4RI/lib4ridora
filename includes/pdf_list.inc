<?php

/**
 * @file
 * PDF listing.
 */

/**
 * Build a listing of the relevent PDFs on the given object.
 *
 * @param AbstractObject $object
 *   The object from which to render the PDFs.
 *
 * @return array
 *   A Drupal renderable array.
 */
function lib4ridora_list_pdfs(AbstractObject $object) {
  $to_return = array(
    '#theme' => 'table',
    '#header' => array(
      t('Datastream ID'),
      t('Filename'),
      t('Availability'),
      t('Version'),
      t('Use Permission'),
    ),
    '#rows' => array(),
    '#empty' => t('No relevant datastreams.'),
  );

  module_load_include('inc', 'lib4ridora', 'includes/embargo.form');
  $datastreams = iterator_to_array($object);
  $pdf_datastreams = array_filter($datastreams, 'lib4ridora_multi_embargo_pdf_filter_soft');
  if ( sizeof($pdf_datastreams) > 1 ) {
	$pdf_datastreams = lib4ridora_ds_pdf_sort( $pdf_datastreams );
  }
  $options = lib4ridora_multi_pdf_get_options();
  $get_option = function ($name, $info, $default = NULL) use ($options) {
    return isset($options[$name][$info[$name]]) ?
      $options[$name][$info[$name]] :
      ($default === NULL ?
        t('Unknown value %value.', array(
          '%value' => $info[$name],
        )) :
        $default);
  };
  $get_availability_option = function ($info) use ($get_option) {
    $name = 'availability';
    $value = $get_option($name, $info, FALSE);
    if ($value === FALSE) {
      return t('Unknown value %value.', array(
        '%value' => $info[$name],
      ));
    }
    elseif ($info[$name] == 'date') {
      return t('@value (@year-@month-@day)', array(
        '@value' => $value,
        '@year' => $info['embargo_date']['year'],
        '@month' => str_pad($info['embargo_date']['month'], 2, 0, STR_PAD_LEFT),
        '@day' => str_pad($info['embargo_date']['day'], 2, 0, STR_PAD_LEFT),
      ));
    }
    else {
      return $value;
    }
  };
  foreach ($pdf_datastreams as $pdf_datastream) {
    if (!islandora_datastream_access(ISLANDORA_VIEW_OBJECTS, $pdf_datastream)) {
      continue;
    }

    $info = lib4ridora_get_embargo_info($pdf_datastream);

    $row = array(
      l($pdf_datastream->id, "islandora/object/{$object->id}/datastream/{$pdf_datastream->id}/view"),
      $pdf_datastream->label,
      $get_availability_option($info),
      $get_option('document_version', $info),
      $get_option('use_permission', $info),
    );
    $to_return['#rows'][] = $row;
  }

  return $to_return;
}
