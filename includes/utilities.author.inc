<?php

/**
 * @file
 * Utility functions for the module.
 */

/*
 * Will remvoe the @ symbol (possibly followed by a number) from strings like
 * - "Bravo@ , C.D."
 * - "Foxtrot@26, G."
 * - "The CMS @"
 */
function lib4ridora_no_at_2( $term ) {
	if ( strpos($term,"@") ) {
		return rtrim( preg_replace( "/\s*@\d*\s*,/", ",", preg_replace( "/\s*@\d*\s*$/", "", $term ) ) );
	}
	return $term;
}
	
/*
 * Auxiliary function to compose the portion of xml code used in MODS.
 * 
 * It is intended to parse/split a special structure of the value in tag 'originalAuthorList'
 * where the family name may contain an @ symbol to point on an affiliation/unit.
 * 
 * Called by lib4ridora_parse_orig_author_list()
 * 
 * Currently this is done on a 'text level' for migration related legacy reasons, another approach could be 
 * to create a temporary DOM with loadXML('<mods xmlns="http://www.loc.gov/mods/v3">' . $codeFromBelow . '</mods>')
 * and then to extract all object nodes/elements a far as needed.
 */
function lib4ridora_mods_code_author( $nameFamily, $nameGiven, $pid, $authorCount, $unitId = '', $unitName = '' ) {
	$unitNum = strpos($nameFamily,"@");
	if ( empty($unitId) && empty($unitName) && $unitNum ) {
		$uTmpAry = explode("@",$nameFamily,2);		// to split "Jones@1234" (from "Bucher, G.|Jones@1234, F.|Meier, J.D.")
		$nameFamily = rtrim($uTmpAry[0]);
		$unitNum = @intval(ltrim($uTmpAry[1]));		// tolerated if $unitNum becomes zero (as a placeholder)
		$unitId = strtok($pid,":") . "-units:" . strval($unitNum);		// build the unit ID like 'psi-units:123'
		if ( $uTmpObj = @islandora_object_load($unitId) ) {
			$unitName = @strtok(substr(strchr($uTmpObj['DC']->Content,"<dc:title>"),10),"<");
		}
	}
	$eCodeAry = array();
	$eCodeAry[]     = '<name type="personal"' . ( empty($authorCount) ? ' usage="primary">' : '>' );
	$eCodeAry[]     = '  <role>';
	$eCodeAry[]     = '    <roleTerm authority="marcrelator" type="text">author</roleTerm>';
	$eCodeAry[]     = '  </role>';
	$eCodeAry[]     = '  <namePart type="family">' . $nameFamily . '</namePart>';
	$eCodeAry[]     = '  <namePart type="given">' . $nameGiven . '</namePart>';
	if ( strtok($pid,":") == "psi" ) {
		$eCodeAry[] = '  <alternativeName altType="formal_name">';
		$eCodeAry[] = '    <namePart/>';
		$eCodeAry[] = '    <nameIdentifier type="authorId"/>';
		$eCodeAry[] = '  </alternativeName>';
		$eCodeAry[] = '  <nameIdentifier type="organizational unit id"' . ( empty($unitId) ? "/>" : ">{$unitId}</nameIdentifier>" );
		$eCodeAry[] = '  <affiliation type="group"' . ( !empty($unitId) ? ( ( empty($unitName) || $unitNum === 0 ) ? ">Unknown</affiliation>" : ">{$unitName}</affiliation>" ) : "/>" );
		$eCodeAry[] = '  <affiliation type="section"/>';
		$eCodeAry[] = '  <affiliation type="department"/>';
		$eCodeAry[] = '  <affiliation type="division"/>';
	} else {
		$eCodeAry[] = '  <fullName' . ( empty($nameGiven) ? "/>" : ">{$nameGiven} {$nameFamily}</fullName>" );
		$eCodeAry[] = '  <nameIdentifier type="authorId"/>';
		$eCodeAry[] = '  <nameIdentifier type="organizational unit id"' . ( empty($unitId) ? "/>" : ">{$unitId}</nameIdentifier>" );
		$eCodeAry[] = '  <affiliation' . ( !empty($unitId) ? ( ( empty($unitName) || $unitNum === 0 ) ? ">Unknown</affiliation>" : ">{$unitName}</affiliation>" ) : "/>" );
	}
	$eCodeAry[]     = '</name>';
	return ("  " . implode("\n  ",$eCodeAry) . "\n");
}

/*
 * function to split a MODS text-wise
 * 
 * will return an array. Index 'name' contains all author related name elements of the MODS,
 * index 'rest' contains the rest of the MODS.
 * 
 * Optionally allows a position mark/needle where first author related name element was cut out.
 */
function lib4ridora_parse_orig_author_list( &$object, $ignLimit = 50, $showLimit = 10, $addAutLast = true, $showMsg = false, $dirBackup = "/tmp/OrigAuthorListParsing", $modsNeedle = "authorsHere" ) {

	$pid = $object->id;
	if ( !strpos($pid,":") ) { return; }
	if ( $ignLimit <= $showLimit || $showLimit < 3 || $ignLimit < 5 ) { return; }
	if ( !( $modsOrig = $object['MODS']->content ) ) { return; }

	// ========================== MAIN EVALUATION AND DATA COLLECTION PART =======================

	$authorCount = 0;
	$autListNew = array();
	$autAffiliated = array();
	$autLastCache = array();
	$origAutAry = array();		// authors taken from the originalAuthorList element
	$hasNameEtAl = false;
	$modsRest = "";		// !
	$userMsg = "";
	$tagAuthorList = trim( variable_get('lib4ridora_author_list_original', 'originalAuthorList') );		// may exist (only) in MODS 

	$domOrig = new DOMDocument();
	$domOrig->loadXML( str_replace("&apos;","+@p0z;",$modsOrig) );		// optional(?) work-around not to get &apos; decoded into a single-quote by xml functions.
	$xpath = new DOMXPath($domOrig);
	$xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');

	if ( $elements = $xpath->query('//mods:mods/mods:*') ) {
		$domRest = new DOMDocument;
		$domRest->formatOutput = true;
		$domRest->loadXML('<mods xmlns="http://www.loc.gov/mods/v3" xmlns:mods="http://www.loc.gov/mods/v3" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xlink="http://www.w3.org/1999/xlink"></mods>');
		foreach( $elements as $element ) {
			if ( !$hasNameEtAl && $element->nodeName == "name" && $element->hasChildNodes() ) {
				foreach($element->childNodes as $nameChild ) {
					if ( $nameChild->nodeName == "etal" ) {
						$hasNameEtAl = true;	// not as expected, so better cancel it instantly.
						return;		// EXIT condition
					}
				}
			}
			if ( empty($origAutAry) && $element->nodeName == "extension" && $element->hasChildNodes() ) {
				foreach($element->childNodes as $extChild ) {
					if ( $extChild->nodeName == $tagAuthorList && !empty(trim($extChild->nodeValue)) ) {
						$textList = trim($extChild->nodeValue);
						$origAutAry = explode("|",$textList);
						if ( strpos($textList,"@") || strpos($textList," |") || strpos($textList,"| ") ) {
							$textList = preg_replace( "/\s*@\d*\s*\|/", "|", lib4ridora_no_at_2($textList) );
							$extChild->nodeValue = trim( preg_replace( "/\s*\|\s*/", "|", $textList ) );
						}
						break;
					}
				}
			}
			$role_term = ( $element->nodeName != "name" ) ? "" : $xpath->evaluate('normalize-space(mods:role/mods:roleTerm[@authority="marcrelator"]/text())', $element);
			$nameFamily = ( $element->nodeName != "name" ) ? "" : $xpath->evaluate('normalize-space(mods:namePart[@type="family"]/text())', $element);
			if ( $role_term != "author" || empty($nameFamily) ) {
				$node = $domRest->importNode($element, true);			// importing node incl. children
				$node = $domRest->documentElement->appendChild($node);		// finally appending it into the temporary node
				continue;
			}

			// if there is a special 'key name' allow overwriting exising authors:
			if ( $authorCount == 0 ) {
				if ( !empty($modsNeedle) ) {		// let's add a text maker there to insert the selected name elements.
					$posAux = $domRest->createElement($modsNeedle,'!');		// to get e.g.: <authorsHere>!</authorsHere>
					$domRest->documentElement->appendChild($posAux);
				}
				$authorCount = ( str_replace(" ","",strtolower($nameFamily)) == "dummyname" ? -1 : 1 );
			}
			elseif ( $authorCount > 0 ) {
		//		$authorCount++;
				return;		// by default let's EXIT with 2 or more authors
			}
		}
		$modsRest = $domRest->saveXML();
	}

	// more EXIT conditions:
	if ( empty($modsRest) ) { return; }
	if ( $authorCount > 1 ) { return; }
	if ( sizeof($origAutAry) < 2 ) { return; }


	// ===================================== for MODS + DC =======================================

	if ( !empty($dirBackup) ) {
		// secure orig. datatreams/files:
		$dirSafe = rtrim( $dirBackup, "/" );		// expecting something like "/tmp/50-Author-Tuning";
		if ( @!is_dir($dirSafe) ) { mkdir($dirSafe); }
		$dirSafe .= "/" . strtok($pid.":",":") . "-" . str_pad( substr(strchr($pid,":"),1), 6, "0", STR_PAD_LEFT );
		if ( @!is_dir($dirSafe) ) { mkdir($dirSafe); }
		$fileSafeBase = $dirSafe . "/";
		$timeStamp = date("Y-m-d.H-i");
		if ( @isset($object['PDF']) ) { $object['PDF']->getContent($fileSafeBase."PDF.{$timeStamp}.pdf"); }
		if ( @isset($object['PDF2']) ) { $object['PDF2']->getContent($fileSafeBase."PDF2.{$timeStamp}.pdf"); }
		if ( @isset($object['RELS-INT']) ) { $object['RELS-INT']->getContent($fileSafeBase."RELS-INT.{$timeStamp}.xml"); }
		if ( @isset($object['RELS-EXT']) ) { $object['RELS-EXT']->getContent($fileSafeBase."RELS-EXT.{$timeStamp}.xml"); }
		if ( @isset($object['DC']) ) { $object['DC']->getContent($fileSafeBase."DC.{$timeStamp}.xml"); }
		$object['MODS']->getContent($fileSafeBase."MODS.{$timeStamp}.xml");
	}

	$authorCount = 0; // reset!
	foreach($origAutAry as $autData) {
		$autAry = explode(",",$autData,2);
		if ( $nameFamily = htmlentities(html_entity_decode(trim($autAry[0]))) ) {

			$nameGiven = ( @empty(rtrim($autAry[1])) ? "" : htmlentities(html_entity_decode(trim($autAry[1]))) );

			$nameArea = lib4ridora_mods_code_author( $nameFamily, $nameGiven, $pid, $authorCount );
			$authorCount++; // afterwards!
			$nameFamily = lib4ridora_no_at_2($nameFamily);
		
			if ( strpos($nameFamily,"@") !== false ) {
				$autAffiliated[] = array( $nameArea, "{$nameFamily}, {$nameGiven}" );
			}
			
			if ( sizeof($autListNew) < $showLimit ) {
				$autListNew[] = array( $nameArea, "{$nameFamily}, {$nameGiven}" );
			} elseif ( !empty($pers_unit) ) {
				$autListNew[] = array( $nameArea, "{$nameFamily}, {$nameGiven}" );
				$autLastCache = array();
			} else {
				$autLastCache = array( $nameArea, "{$nameFamily}, {$nameGiven}" );
			}
		}
	}
	if ( $addAutLast && sizeof($autLastCache) ) { $autListNew[] = $autLastCache; }


	// ====================================== UPDATE MODS ========================================

	$nameArea = "";
	foreach( $autListNew as $nAry ) { $nameArea .= $nAry[0]; }
	if ( !$hasNameEtAl && $authorCount > $ignLimit ) { $nameArea .= "\n  <name>\n    <etal/>\n  </name>"; }

	if ( !empty($modsNeedle) ) {
		$modsNew = str_replace("<{$modsNeedle}>!</{$modsNeedle}>",$nameArea,$modsRest);
	}
	$modsNew = str_replace("+@p0z;","&apos;", $modsNew );			// revert the 'single-quote' work-around

	$tmpAry = explode("\n",$modsNew);		// optional/minor clean-up of empty rows.
	$modsNew = "";
	foreach($tmpAry as $row) { 
		if ( !empty(rtrim($row)) ) { $modsNew .= $row . "\n"; }
	}

	if ( strlen($modsNew) != strlen($modsOrig) || $modsNew != $modsOrig ) {
		$object['MODS']->content = $modsNew;
		$userMsg .= "{$pid} : MODS datasream created! <br>";
	} else {
		$userMsg .= "{$pid} : MODS datastream already up to date, so nothing done! <br>";
	}


	// ======================================= UPDATE DC =========================================

	if ( $dcCode = @$object['DC']->content ) {		// working well though, but to be revised (since text based):
			
		$dcAry = explode("<dc:creator",$dcCode);			// note: dc.creator may have an attribute sometimes: xmlns:dc="http://purl.org/dc/elements/1.1/"
		$dcRest = array_shift($dcAry);
		$dcAtt = "";	// if there are attributes for dc:creator then just get it once for all.
		foreach($dcAry as $dcIdx => $dcPart ) {
			if ( $pos = strpos($dcPart,"</dc:creator>") ) {
				if ( substr($dcPart,$pos-1,1) != ")" || substr($dcPart,$pos-8,8) == "(author)" ) {
					if ( $dcIdx < 2 && empty($dcAtt) ) {
						$dcAtt = substr(strtok(" ".$dcPart,">"),1);
					}
					$dcRest .= rtrim(substr($dcPart,$pos+13),"\r\n");
					continue;
				}
			}
			$dcRest .= "<dc:creator" . $dcPart;
		}

		$dcAry = array();
		foreach( $autListNew as $nAry ) { 
			$dcAry[] = "<dc:creator" . $dcAtt . ">" . $nAry[1] . " (author)</dc:creator>";
		}

		$dcNew = "";
		if ( $pos = strpos($dcRest,"</dc:title>") ) {
			$dcNew = substr($dcRest,0,$pos+11) . "\n  " . implode("\n  ",$dcAry) . substr($dcRest,$pos+11);
		} elseif ( $pos = strpos($dcRest,"</oai_dc:dc>") ) {
			$dcNew = substr($dcRest,0,$pos) . "  " . implode("\n  ",$dcAry) . "\n" . substr($dcRest,$pos);
		}

		$dcAry = explode("\n",$dcNew);		// optional/minor clean-up of empty rows.
		$dcNew = "";
		foreach($dcAry as $row) { 		// remove rows like "<dc:creator> (editor)</dc:creator>" or empty rows generally
			if ( !strpos($row,"/>") && strpos($row,"(") && empty(rtrim(strip_tags(strtr($row,"()","<>")))) ) { continue; }
			if ( !empty(rtrim($row)) ) { $dcNew .= $row . "\n"; }
		}

		if ( strlen($dcNew) != strlen($dcCode) || $dcNew != $dcCode ) {
			$object['DC']->content = $dcNew;
			$userMsg .= "{$pid} : DC datastream created! <br>";
		} else {
			$userMsg .= "{$pid} : DC datastream already up to date, so nothing done! <br>";
		}
	}

	if ( $showMsg && !empty($userMsg) ) { dpm( $userMsg ); }
}
