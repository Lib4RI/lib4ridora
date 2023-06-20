<?php

function lib4ridora_doi_finder_form(array $form, array &$form_state) {
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/utilities');
  form_load_include($form_state, 'inc', 'lib4ridora', 'includes/doi-finder.form');
  form_load_include($form_state, 'inc', 'islandora_doi', 'includes/utilities');

  $form = array();

  $form['doi_finder'] = array(
    '#title' => t('List of DOIs'),
    '#description' => t('Enter the list of DOIs you want to check (one per line)'),
    '#type' => 'textarea',
    '#required' => TRUE,
  );
  $form['submit'] = array(
    '#value' => t('Find objects with those DOIs'),
    '#type' => 'submit',
  );

  $doilist = array();
  $duplicateinput = array();
  if (isset($form_state['values'])) {
    foreach (array_map('trim', preg_split('/\n/', $form_state['values']['doi_finder'])) as $doi) {
      if (array_key_exists($doi, $doilist) && !in_array($doi, $duplicateinput)) {
        array_push($duplicateinput, $doi);
      }
      elseif ($doi !== '') {
        $doilist[$doi] = array();
      }
    }
  }

  foreach ($doilist as $doi => $val) {
//    $doilist[$doi]['isvalid'] = (lib4ridora_doi_get_document_url($doi) !== FALSE);
    $doilist[$doi]['isvalid'] = (lib4ridora_doi_resolve_doi($doi) !== FALSE);
    $doilist[$doi]['hasmods'] = (islandora_doi_get_mods($doi) !== FALSE);
  }
  
  $result = '';
  $colok = '#009000';
  $colinvalid = '#b7a000';
  $colnomods = '#207020';
  $colindora = '#c70000';
  $colboth = '#ff5050';
  foreach ($doilist as $doi => $val) {
    $siblings = lib4ridora_get_objects_by_doi($doi);
    $col = ($val['hasmods']) ? $colok : $colnomods;
    if ($siblings !== FALSE) {
      $col = $colindora;
      if (!$val['isvalid']) {
        $col = $colboth;
      }
    }
    elseif (!$val['isvalid']) {
      $col = $colinvalid;
    }
    $result .= "<span style='color: " . $col . "'>";
    $result .= "\"" . $doi . "\" is";
    if (!$val['isvalid']) {
      $result .= " not valid";
    }
    elseif (!$val['hasmods']) {
      $result .= " without MODS";
    }
    if ($siblings !== FALSE) {
      $sibsmarkup = array();
      foreach ($siblings['objects'] as $obj) {
        $sibsmarkup[] = $obj['#markup'];
      }
      if (!$val['isvalid'] || !$val['hasmods']) {
        $result .= " and";
      }
      $result .= " already in the system: " . implode(', ', $sibsmarkup);
    }
    elseif ($val['isvalid']) {
      if (!$val['hasmods']) {
        $result .= " but";
      }
      $result .= " ok to ingest";
    }
    $result .= ".<br/>";
    $result .= "</span>";
  }
  if (count($duplicateinput) > 0) {
    $result .= "<br/><span style='color: red'>Also, you specified the following doi's more than once: \"" . implode("\", \"", $duplicateinput) . "\"</span>";
  }

  if (!empty($result)){
    $form['result'] = array(
      '#type' => 'markup',
      '#markup' => '<div><br/><b>Result:</b><br/>' . $result . '</div>',
    );
  }

  return $form;
}

function lib4ridora_doi_finder_form_submit(array $form, array &$form_state) {
  $form_state['input']['doi_finder'] = preg_replace('/[ ]*/', '', $form_state['values']['doi_finder']);
  $form_state['rebuild'] = TRUE;
}

?>
