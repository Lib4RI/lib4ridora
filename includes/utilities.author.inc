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
