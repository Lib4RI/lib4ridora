<?php
/**
 * @file
 * Default template for lib4ridora-islandora-solr-metadata-injected-funder-info.
 *
 * Somewhat gross, due to the default nl2br-type stuffs.
 *
 * Available variables:
 * - $fundername_attributes: An associative array which may contain:
 *   - href: URL for the funder.
 * - $awardtitle_attributes: An associative array which may contain:
 *   - href: URL for the funder's org.
 * - $awardnumber_attributes: An associative array which may contain:
 *   - href: URL for the funder's org.
 * - $info: Associative array containing:
 *   - fundername
 *     - name
 *   - awardtitle
 *     - title
 *   - awardnumber
 *     - number
 */

$tagFN = isset($fundername_attributes['href']) ? 'a' : 'span';
$tagAT = isset($awardtitle_attributes['href']) ? 'a' : 'span';
$tagAN = isset($awardnumber_attributes['href']) ? 'a' : 'span';

$htmlTmp = '<'.$tagFN.' '.drupal_attributes($fundername_attributes).'>'.$info['fundername']['name'].'</'.$tagFN.'>:&nbsp;';
$htmlTmp .= '<'.$tagAT.' '.drupal_attributes($awardtitle_attributes).'>'.$info['awardtitle']['title'].'</'.$tagAT.'>&nbsp;(';
$htmlTmp .= '<'.$tagAN.' '.drupal_attributes($awardnumber_attributes).'>'.$info['awardnumber']['number'].'</'.$tagAN.'>)';

?>
<span class="<?php print $classes;?>"><b class="lib4ri_space_nowrap_unbold"><?php print $htmlTmp;?></b></span>
