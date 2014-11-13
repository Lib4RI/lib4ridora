<?php

/**
 * @file
 * Batch code for ingesting journal impact factors.
 */

/**
 * Get the batch for importing journal impact factors.
 *
 * @param string $year
 *   The year the factors apply to.
 * @param string $delimiter
 *   The delimiter to use for the CSV.
 * @param object $csv
 *   The CSV file of impact factors to import.
 */
function lib4ridora_impact_factor_batch($year, $delimiter, $csv) {
  return array(
    'title' => t('Journal Impact Factor Import'),
    'init_message' => t('Preparing to import impact factors.'),
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
    'error_message' => t('An error has occurred.'),
    'operations' => array(
      array(
        'lib4ridora_impact_factor_batch_process',
        array($year, $delimiter, $csv),
      ),
    ),
    'finished' => 'lib4ridora_impact_factor_batch_finished',
    'file' => drupal_get_path('module', 'lib4ridora') . '/includes/journal_impact.batch.inc',
  );
}

/**
 * Batch function for importing journal impact factors.
 *
 * @param string $year
 *   The year the factors apply to.
 * @param string $delimiter
 *   The delimiter to use for the CSV.
 * @param object $csv
 *   The CSV file of impact factors to import.
 */
function lib4ridora_impact_factor_batch_process($year, $delimiter, $csv, &$context) {
  module_load_include('inc', 'lib4ridora', 'includes/utilities');
  $sandbox = &$context['sandbox'];

  if (!isset($context['results']['csv'])) {
    $context['results']['csv'] = $csv;
  }
  $csv_file = fopen($csv->uri, 'r');

  if (isset($sandbox['file_position'])) {
    // So we don't read the entire file into memory.
    fseek($csv_file, $sandbox['file_position']);
  }
  else {
    // So we don't use the header.
    fgetcsv($csv_file, 0, $delimiter);
  }
  $csv_line = fgetcsv($csv_file, 0, $delimiter);
  $sandbox['file_position'] = ftell($csv_file);

  // Done if no more lines.
  if (!$csv_line) {
    $context['finished'] = 1;
    return;
  }
  $context['finished'] = 0;
  list($identifier, $factor) = $csv_line;

  // Set an error if there is no impact factor provided.
  if (empty($factor)) {
    $factor = "no value";
  }

  // Set an error if no journal found.
  $journal = lib4ridora_get_journal($identifier);
  if (!$journal) {
    drupal_set_message(
      t(
        'No journal found with identifier: @identifier.',
        array('@identifier' => $identifier)
      ),
      'warning'
    );
    return;
  }
  // Set an error if impact factor is set and doesn't match.
  $existing_factor = lib4ridora_get_impact_factor($journal, $year);
  if ($existing_factor != FALSE && $existing_factor != $factor) {
    drupal_set_message(
      t(
        'There is disagreement between the impact factor supplied (@factor) and
          the one existing (@existing_factor) at @identifier.',
        array(
          '@factor' => $factor,
          '@existing_factor' => $existing_factor,
          '@identifier' => $identifier,
        )
      ),
      'warning'
    );
    return;
  }
  lib4ridora_set_impact_factor($journal, $year, $factor);
}

/**
 * Import factor batch finished callback.
 */
function lib4ridora_impact_factor_batch_finished($success, $results, $operations) {
  file_delete($results['csv']);
}
