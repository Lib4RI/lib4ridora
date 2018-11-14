<?php

/**
 * @file
 * Helper for text_format usage in XML forms.
 */

/**
 * Helper; fetch the target element.
 *
 * @param array $element
 *   The element being processed, targetting another element.
 * @param array $form_state
 *   A reference to the form state.
 * @param array $form
 *   Complete copy of the form, as provided by element #process callbacks.
 *
 * @return array
 *   The targeted text_format element.
 *
 * @throws InvalidArgumentException
 *   If the target element could not be determined.
 */
function _lib4ridora_target_text_format_fetch_element(array $element, array &$form_state, array $form) {
  $info = drupal_array_get_nested_value($element, array(
    '#user_data',
    'lib4ridora_target_text_format',
  ));
  $subs = array(
    '@key' => 'lib4ridora_target_text_format',
    '@data' => '#user_data',
    '@type' => 'text_format',
  );
  if (!isset($info)) {
    throw new InvalidArgumentException(t('Invalid argument; missing "@key" in "@data".', $subs));
  }

  $target_offset = json_decode($info, TRUE);
  if (!isset($target_offset)) {
    throw new InvalidArgumentException(t('Invalid argument; failed to parse JSON from "@key" in "@data".', $subs));
  }
  // Handle strings of the ".." magic value, to allow for relative structures,
  // supporting repeatable elements like fieldpane or tabpanel setups.
  foreach ($target_offset as $index => $offset) {
    if ($offset != '..') {
      if ($index != 0) {
        $target_offset = array_merge(
          array_slice($element['#array_parents'], 0, -$index),
          array_slice($target_offset, 1)
        );
      }
      break;
    }
  }

  $item = drupal_array_get_nested_value($form, $target_offset);
  if (!$item) {
    throw new InvalidArgumentException(t('Invalid argument; failed to locate the element indicated by "@key" in "@data".', $subs));
  }
  elseif ($item['#type'] != 'text_format') {
    throw new InvalidArgumentException(t('Invalid argument; the element indicated by "@key" in "@data" does not appear to be of the "@type" type.', $subs));
  }

  return $item;
}

/**
 * Helper; wrap element call; but, render markup.
 *
 * @see _lib4ridora_target_text_format_fetch_element()
 * @see check_markup()
 */
function _lib4ridora_target_text_format_fetch_markup(array $element, array &$form_state, array $form) {
  $item = _lib4ridora_target_text_format_fetch_element($element, $form_state, $form);
  return check_markup(
    drupal_array_get_nested_value($item, array('value', '#value')),
    drupal_array_get_nested_value($item, array('format', 'format', '#value'))
  );
}
