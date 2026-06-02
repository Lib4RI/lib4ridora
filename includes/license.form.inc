<?php

/**
 * @file
 * Ingest step form for licensing.
 */

/**
 * License form.
 */
function lib4ridora_license_form($form, &$form_state) {
  $form['#attached'] = array(
    'css' => array(
      drupal_get_path('module', 'lib4ridora') . '/css/license.css',
    ),
  );
  $form['license'] = array(
    '#type' => 'item',
    '#title' => t('Deposit License'),
    '#markup' => t('For full-text publications to be made public available.'),
  );
  $form['rights'] = array(
    '#type' => 'item',
    '#title' => t('Your right to publish your work'),
    '#markup' => t('Rights granted to the repository through this agreement are non-exclusive. You are free to publish the Work in its present or future version(s) elsewhere and no ownership is assumed by the repository when storing your Work.'),
  );
  $form['declaration'] = array(
    '#type' => 'item',
    '#title' => t("Depositor's declaration"),
    'agree' => array(
      '#markup' => t('By agreeing to this license, you (the author(s), copyright owner or assignee), grant a non-exclusive license to the repository that authorises the following:'),
    ),
    'list' => array(
      '#theme' => 'item_list',
      '#type' => 'ol',
      '#items' => array(
        t('You are the author, copyright holder or assignee and have authority to make this agreement, and give the Lib4RI the right to publish the Work in a repository.'),
        t("That the work is original and does not, to the best of you knowledge infringe upon anyone's copyright"),
      ),
      '#attributes' => array(
        'class' => array('lib4ri-license-list'),
      ),
    ),
  );
  $form['permission'] = array(
    '#type' => 'item',
    '#title' => t('Permissions given to the repository'),
    '#markup' => t('The repository shall distribute electronic copies of the Work for the lifetime of the repository and translate it as necessary to ensure it can be read by computer systems in the future. No charge will be made for electronic distribution.'),
  );
  $form['withdraw'] = array(
    '#type' => 'item',
    '#title' => t('Withdrawing your work'),
    '#markup' => t('You may request the Work is removed at any point in the future. Equally, the Lib4RI reserves the right to remove the Work for any professional, administrative or legal reason. A metadata record indicating the Work was stored in the repository will remain visible in perpetuity.'),
  );
  $form['disclaimer'] = array(
    '#type' => 'item',
    '#title' => t('Disclaimer'),
    '#markup' => t('While every care will be taken to preserve the Work, the Lib4RI is not liable for loss or damage to the work or other data while it is stored within the repository.'),
  );
  $form['definition'] = array(
    '#type' => 'item',
    '#title' => t('Definition & terms'),
    '#markup' => t('In this licence document ‘Work’ means the electronic version of the scientific publication being deposited including abstract, text, images and related data.'),
  );
  return $form;
}


/*
	Function to return and array with all possibe CC license terms (='use permissions)
 */ 
function lib4ridora_pdf_use_permission_helper( $ccTermAry = null ) {
  if ( $ccTermAry === null || !is_array($ccTermAry) ) {
	$ccTermAry = array(
      NULL => t('Unspecified'),
      'CC BY' => t('Creative Commons: Attribution'),
      'CC BY-SA' => t('Creative Commons: Attribution-ShareAlike'),
      'CC BY-ND' => t('Creative Commons: Attribution-NoDerivs'),
      'CC BY-NC' => t('Creative Commons: Attribution-NonCommercial'),
      'CC BY-NC-SA' => t('Creative Commons: Attribution-NonCommercial-ShareAlike'),
      'CC BY-NC-ND' => t('Creative Commons: Attribution-NonCommercial-NoDerivs'),
      'CC0' => t('Creative Commons: Zero (CC0)'),
    );
  }
  // We must compose+add all allowed/possible CC license strings to the form,
  // otherwise Drupal will not accept new JS-created terms:
  $usePermAry = $ccTermAry;
  $ccVersion = variable_get("lib4ridora_pdf_cc_version","---,1.0,2.0,2.5,3.0,4.0");
  $ccVerAry = array_map('trim',explode(',',$ccVersion));
  $ccCountry = variable_get("lib4ridora_pdf_cc_country","---,IGO,UK");
  $ccLocAry = array_map('trim',explode(',',$ccCountry));
  foreach( $ccTermAry as $cc => $desc ) {
	if ( !$cc ) { continue; }  
    foreach( $ccVerAry as $ccVer ) {
      if ( substr($ccVer,0,2) == '--' ) { continue; }
      $key = $cc . ' ' . $ccVer;
      $usePermAry[$key] = $key;
      foreach( $ccLocAry as $ccLoc ) {
        if ( substr($ccLoc,0,2) == '--' ) { continue; }
        $key = $cc . ' ' . $ccVer . ' ' . $ccLoc;
        $usePermAry[$key] = $key;
      }
    }
    foreach( $ccLocAry as $ccLoc ) { // no version but with country/localization 'deed'
      if ( substr($ccLoc,0,2) == '--' ) { continue; }
      $key = $cc . ' ' . $ccLoc;
      $usePermAry[$key] = $key;
    }
  }	
  return $usePermAry;
}

/*
	Function to help expanding the	CC license string with JavaScript
	Code is very specific to ccLicenseComposer() in lib4ridora/js/lib4ridora.pdf_support.js

	Via PHP we are going to add/replace some selection options stored in system variables: 
	sudo drush -r /var/www/html -y @sites php-eval 'variable_set("lib4ridora_pdf_cc_version","4.0,---,1.0,2.0,2.5,3.0")'
	sudo drush -r /var/www/html -y @sites php-eval 'variable_set("lib4ridora_pdf_cc_country","---,IGO,UK")'
*/
function lib4ridora_pdf_cc_license_helper($jsCode = '') {
  $ccVersion = variable_get("lib4ridora_pdf_cc_version","---,1.0,2.0,2.5,3.0,4.0");
  $ccVersion = "'" . str_replace(',',"','",str_replace(' ','',$ccVersion)) . "'";
  $jsCode = str_replace("'---','0.0'",$ccVersion,$jsCode);
  $ccCountry = variable_get("lib4ridora_pdf_cc_country","---,IGO,UK");
  $ccCountry = "'" . str_replace(',',"','",str_replace(' ','',$ccCountry)) . "'";
  return str_replace("'---','XYZ'",$ccCountry,$jsCode);
}
