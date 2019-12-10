<?php

/**
 * @file
 * Utility functions for the module.
 */

/*
 * function to remove spaces from initials in 'given name' where needed.
 *
 * Example of initials (types of given name) that can be treated here:
 *	"F. M."          =>  "F.M."
 *	"Jan &#197;. D." =>  "Jan &#197;.D."
 *	"F. &#xD6;."     =>  "F.&#xD6;."
 *	"Chr. E. Th."    =>  "Chr.E.Th."
 *	"H.R. J&#252;rg" =>  "H.R. J&#252;rg"
 */
function lib4ridora_unspace_name_given( $nameGiven ) {
	if ( !strpos($nameGiven,".") ) { return $nameGiven; }

	$given = preg_replace("/\s+\./",".",trim($nameGiven));	// no space in front of dots, optional/convention
	$given = preg_replace("/\s+/","  ",$given);			// ensure (two) normal space(s), required!
	if ( strcmp($given,$nameGiven) === 0 ) { return $nameGiven; }

	$regExAry = array(	/* looking for terms where we need to keep spaces */
		"/\s[A-Z]([A-Z]+|[a-z]+)\s/",				/* e.g. ' Frank ' or ' DeLuca ' */
		"/\s\&\#[0-9|x|X|A-Fa-f]{1,4};\w*\s/",			/* e.g. ' Özmir ' */
		"/\s[A-Z][a-z]{0,1}&#[0-9|x|X|A-Fa-f]{1,4};\w*\s/",		/* e.g. ' Jürg ' */
	);
	$spacedAry = array();		// only for terms that need spaces around
	foreach( $regExAry as $regEx ) {
		$matchAry = array();
		preg_match_all( $regEx, " {$given} ", $matchAry );
		if ( @empty($matchAry[0]) ) { continue; }
		$spacedAry = array_merge( $spacedAry, $matchAry[0] );
	}

	$given = str_replace(" ","",$given);	// now removing all spaces...
	if ( @empty($spacedAry) ) { return $given; }

	foreach( $spacedAry as $spaced ) {  	// ...and adding 'spaced' terms
		$given = str_replace(trim($spaced," "), $spaced, $given);
	}
	return str_replace("  "," ",trim($given) );
}

/*
 * Helper function to remvoe the @ symbol (possibly followed by a number) from the end of a strings (or before | or comma)
 * - "Bravo@ , C.D."
 * - "Foxtrot@26, G."
 * - "The CMS @"
 */
function lib4ridora_pop_at( $term ) {
	if ( strpos($term,"@") ) {
		$term = preg_replace( "/\s*@\s*\d*\s*$/", "", $term );
		$term = preg_replace( "/\s*@\s*\d*\s*,/", ",", $term );
		$term = preg_replace( "/\s*@\s*\d*\s*\|/", "|", $term );
	}
	return trim($term);
}

/*
 * Helper function to extract the unit/affiliation related data (as array) from the family/given name string.
 * this data is indicate by an @ symbol, the @ alone implies any affliation (to be specified by editor later).
 *
 */
function lib4ridora_unit_by_at( $pid, &$nameFamily, &$nameGiven ) {
	$unitNum = -1;
	$unitId = "";
	$unitName = "";
	if ( $pos = strrpos($nameGiven,"@") ) {		// to split "Jones@1234"
		$unitNum = intval(trim(substr($nameGiven,$pos+1)));		// tolerated if $unitNum becomes zero (as a placeholder)
		$nameGiven = trim(substr($nameGiven,0,$pos),"@ \t\n\r\0\x0B");
		$unitId = strtok($pid,":") . "-units:" . strval($unitNum);		// build the unit ID like 'psi-units:123'
	}
	if ( $pos = strrpos($nameFamily,"@") ) {		// Do not elseif! An @ in the family name shell have more priority!(?)
		$unitNum = intval(trim(substr($nameFamily,$pos+1)));
		$nameFamily = trim(substr($nameFamily,0,$pos),"@ \t\n\r\0\x0B");
		$unitId = strtok($pid,":") . "-units:" . strval($unitNum);
	}
	if ( empty($unitName) && strpos($unitId,":") && $unitNum > 0 ) {
		if ( $uTmpObj = @islandora_object_load($unitId) ) {
			$unitName = @strtok(substr(strchr($uTmpObj['DC']->Content,"<dc:title>"),10),"<");
		}
	}
	if ( !empty($unitId) && empty($unitName) ) {
		$unitName = ( $unitNum == 0 ) ? "to be assigned" : "unknown";	// placeholders
	}
	return array( 'uName' => $unitName, 'uId' => $unitId, 'uNum' => $unitNum );
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
function lib4ridora_mods_code_author( $pid, $authorCount, $nameFamily, $nameGiven, $unitName = '', $unitId = '', $unitNum = -1 ) {
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
		$eCodeAry[] = '  <nameIdentifier type="organizational unit id"' . ( !empty($unitId) ? ">{$unitId}</nameIdentifier>" : "/>" );
		$eCodeAry[] = '  <affiliation type="group"' . ( !empty($unitId) ? ">{$unitName}</affiliation>" : "/>" );
		$eCodeAry[] = '  <affiliation type="section"/>';
		$eCodeAry[] = '  <affiliation type="department"/>';
		$eCodeAry[] = '  <affiliation type="division"/>';
	} else {
		$eCodeAry[] = '  <fullName' . ( empty($nameGiven) ? "/>" : ">{$nameGiven} {$nameFamily}</fullName>" );
		$eCodeAry[] = '  <nameIdentifier type="authorId"/>';
		$eCodeAry[] = '  <nameIdentifier type="organizational unit id"' . ( !empty($unitId) ? ">{$unitId}</nameIdentifier>" : "/>" );
		$eCodeAry[] = '  <affiliation' . ( !empty($unitId) ? ">{$unitName}</affiliation>" : "/>" );
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
	$autListAll = array();
	$autListTuned = array();
	$autAffiliated = array();
	$autLastCache = array();
	$origAutAry = array();		// authors taken from the originalAuthorList element, per author there is an sub-array
	$hasNameEtAl = false;
	$updateList = 0;
	$updateAuthor = 0;
	$modsRest = "";		// !
	$userMsg = "";
	$tagAuthorList = trim( variable_get('lib4ridora_author_list_original', 'originalAuthorList') );		// may exist (only) in MODS 
	$aut1unitId = "";

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
						$hasNameEtAl = true;
					}
				}
			}
			if ( empty($origAutAry) && $element->nodeName == "extension" && $element->hasChildNodes() ) {
				foreach($element->childNodes as $extChild ) {
					if ( $extChild->nodeName == $tagAuthorList && !empty(trim($extChild->nodeValue)) ) {
						$textList = $extChild->nodeValue;
						$textLength = strlen($textList);
						$textList = trim($textList,"| \t\n\r\0\x0B");
						if ( substr($textList,0,1) == "!" )	{
							$updateAuthor = 5;		// special meaning: User/Editor wants an update
						} elseif ( strpos($textList,"@") ) {
							$updateAuthor = 1;		// special meaning: update seems appropriate
						}
						$textList = preg_replace( "/\s*\|\s*/", "|", ltrim($textList,"!| \t\n\r\0\x0B") ); // trim the '!' too!
						$autAry = explode("|",$textList);
						$textList = "";
						foreach( $autAry as $nIdx => $nTxt ) {
							$nAry = str_getcsv($nTxt);
							$nameFamily = str_replace( " - ", "-", preg_replace( "/\s+/"," ", trim($nAry[0]) ) );
							$nameGiven = @implode(" ",array_slice($nAry,1));	// if there should be commas in the given name
							$unitAry = lib4ridora_unit_by_at( $pid, $nameFamily, $nameGiven );
							$nameGiven = lib4ridora_unspace_name_given($nameGiven); // after lib4ridora_unit_by_at() !
							$origAutAry[] = array_merge( array( 'family' => $nameFamily, 'given' => $nameGiven ), $unitAry );
							$textList .= ( $nIdx ? "|" : "" ) . $nameFamily . ", " . $nameGiven;
						}
						if ( strlen($textList) != $textLength ) {	// if $textList changed, then it will be smaller (we only removed charaters)
							$updateList = 1;
							$extChild->nodeValue = $textList;
						}
						break;
					}
				}
			}
		}
		foreach( $elements as $element ) {
			$role_term = ( $element->nodeName != "name" ) ? "" : $xpath->evaluate('normalize-space(mods:role/mods:roleTerm[@authority="marcrelator"]/text())', $element);
			$nameFamily = ( $element->nodeName != "name" ) ? "" : $xpath->evaluate('normalize-space(mods:namePart[@type="family"]/text())', $element);
			if ( $role_term != "author" || empty($nameFamily) ) {
				$node = $domRest->importNode($element, true);			// importing node incl. children
				$node = $domRest->documentElement->appendChild($node);		// finally appending it into the temporary node
				continue;
			}

			if ( $authorCount < 1 ) {
				if ( !empty($modsNeedle) ) {		// let's add a text maker there to insert the selected name elements.
					$posAux = $domRest->createElement($modsNeedle,'!');		// to get e.g.: <authorsHere>!</authorsHere>
					$domRest->documentElement->appendChild($posAux);
				}
				$xPathTmp = ( strtok($pid,":") == "psi" ) ? 'mods:affiliation[@type="group"]' : 'mods:nameIdentifier[@type="organizational unit id"]';
				$aut1unitId = trim( $xpath->evaluate('normalize-space('.$xPathTmp.'/text())', $element) );
				$authorCount = 1;
			}
			elseif ( $updateAuthor ) {
				$authorCount++;
			}
			elseif ( !$updateList ) {
				return;		// by default let's EXIT with 2 or more authors
			}
		}

		if ( $updateAuthor == 1 && $authorCount == 1 && empty($aut1unitId) ) {
			$updateAuthor = 3;
		}
		if ( $updateList && $updateAuthor < 3 ) {
			$modsNew = $domOrig->saveXML();
			$object['MODS']->content = $modsNew;
			return;
		}
		$modsRest = $domRest->saveXML();
	}

	// EXIT conditions:
	if ( empty($modsRest) ) { return; }
	if ( $authorCount < 1 || sizeof($origAutAry) < 2 ) { return; }
	if ( $updateAuthor < 3 ) { return; }

	// ===================================== for MODS + DC =======================================

	if ( $updateAuthor && !empty($dirBackup) ) {
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
	foreach($origAutAry as $autAry) {
		if ( $nameFamily = htmlentities(html_entity_decode(trim($autAry['family']))) ) {
			$nameGiven = htmlentities(html_entity_decode(trim($autAry['given'])));

			// Going to produce the XML code, following call will also remove the '@' from family & given name:
			$nameArea = lib4ridora_mods_code_author( $pid, $authorCount, $nameFamily, $nameGiven, $autAry['uName'], $autAry['uId'], $autAry['uNum'] );
			$authorCount++; // afterwards!
			$autListAll[] = array( $nameArea, "{$nameFamily}, {$nameGiven}" );

			$is_affiliated = strpos($autAry['uId'],":"); 	// just checking a unit id
			if ( $is_affiliated ) {
				$autAffiliated[] = array( $nameArea, "{$nameFamily}, {$nameGiven}" );
			}
			if ( sizeof($autListTuned) < $showLimit ) {		// not elseif !
				$autListTuned[] = array( $nameArea, "{$nameFamily}, {$nameGiven}" );
			} elseif ( $is_affiliated ) {
				$autListTuned[] = array( $nameArea, "{$nameFamily}, {$nameGiven}" );
				$autLastCache = array();
			} else {
				$autLastCache = array( $nameArea, "{$nameFamily}, {$nameGiven}" );
			}
		}
	}
	if ( $authorCount <= $ignLimit ) { $autListTuned = $autListAll; }
	elseif ( $addAutLast && sizeof($autLastCache) ) { $autListTuned[] = $autLastCache; }


	// ====================================== UPDATE MODS ========================================

	$nameArea = "";
	foreach( $autListTuned as $nAry ) { $nameArea .= $nAry[0]; }
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
		foreach( $autListTuned as $nAry ) { 
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
