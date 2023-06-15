<?php
/*
Description:
	PHP code for assignment or removal of OA publications to institute's *:open_access collection.
	This is addtional/auxiliary code to run (e.g. as Crontab job) to cover cases of collection switches or OA status changes 
	(embargo lifting or temporary collection usages) that cannot be caught by the  tunings in lib4ridora/includes/full_text.inc.

Suggested setup:
	Copy this file to \\eaw-projects\Lib4RI_DORA_archive$\work-in-progress with file name 'lib4ridora.coll-oa-recheck.txt' (as used below).

	Command line via Drush:
	/usr/bin/drush -u 1 -r /var/www/html dora-batch --pids=single --command='process_code' --command_options='file=/var/www/html/data/work-in-progress/lib4ridora.coll-oa-recheck.txt|delay=6|inst=Eawag' --dry=false

	As Crontab job:
	10 16 * * fri /usr/bin/drush -u 1 -r /var/www/html dora-batch --pids=single --command='process_code' --command_options='file=/var/www/html/data/work-in-progress/lib4ridora.coll-oa-recheck.txt|delay=6|inst=*' --dry=false > /dev/null 2>&1

Caching+Logging:
	Json files will be cached and resued (for) each hour, see /tmp/_oa_coll*.json
 	Action logs will be crated, see /tmp/_oa_coll*.log
*/


$instAry = ['Eawag','Empa','PSI','WSL']; // optional defaults, will turn into array('eawag' => 'Eawag', 'empa' => 'Empa', 'psi' => 'PSI', 'wsl' => 'WSL')

if ( @!empty($inst) ) {	$instAry = array_map('trim',explode(',',$inst)); /* over-ride via parameter, $inst also can be an '*' only */ }
$instAry = array_combine( array_map('strtolower',array_values($instAry)), array_values($instAry) );


// Batch Runtime Parameters:
$delayAry = array('skip' => 2, 'next' => 4, 'wait' => ( @isset($delay) ? max(intval($delay),3) : 6 ), 'mode' => 9 , 'inst' => 12 );
$cpuAry = array( 'cores' => 8 /* TBA */, 'max1min' => 65, 'max5min' => 85 );

$exeAry = array( 'cmd' => 'lscpu', 'out' => array(), 'ret' => 127 );
exec( $exeAry['cmd'], $exeAry['out'], $exeAry['ret'] );
$cpuAry['cores'] = max( intval(substr(strchr(implode("\n",$exeAry['out']),'CPU(s):'),7)), 1 );


// looking for all OA publications in [institute]:publication OR any publications in OA collection. Then we are going to recheck them:
$solrLinkAry = array(
	/* these are *CSV* links below for *Eawag* (so we easily can test/recheck), finally however they will be modified for JSON and to work for all institutes */
	'to-join-oa' => 'http://lib-dora-prod1.emp-eaw.ch:8080/solr/collection1/select?sort=PID+asc&rows=987654321&wt=csv&csv.separator=;&indent=true&q=PID:eawag%5c%3a*+AND+RELS_EXT_isMemberOfCollection_uri_mt:"eawag:publications"+AND+RELS_EXT_fullText_literal_mt:"Open%20Access"+NOT+RELS_EXT_isMemberOfCollection_uri_mt:"eawag:open_access"&fl=PID%2c+fgs_lastModifiedDate_dt',
	'to-leave-oa' => 'http://lib-dora-prod1.emp-eaw.ch:8080/solr/collection1/select?sort=PID+asc&rows=987654321&wt=csv&csv.separator=;&indent=true&q=PID:*%5c%3a*+AND+RELS_EXT_isMemberOfCollection_uri_mt:"open_access"+NOT(RELS_EXT_fullText_literal_mt:"Open%20Access"+AND+RELS_EXT_isMemberOfCollection_uri_mt:"*:publications")&fl=PID%2c+fgs_lastModifiedDate_dt',
);


// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// functions:

module_load_include('inc', 'islandora_basic_collection', 'includes/utilities');

if ( !function_exists('islandora_basic_collection_get_parent_pids') ) {
	include_once('/var/www/html/sites/all/modules/islandora_basic_collection/includes/utilities.inc');
}

if ( !function_exists('lib4ridora_obj_date_last_modified') ) {
	// For a datastream there is only a public 'createdDate' property, however the date of the last modification is protected,
	// see https://wiki.lyrasis.org/display/ISLANDORA712/Build%2C+Access%2C+Modify+and+Delete+Fedora+objects+with+the+Tuque+interface
	// So let's grep it out of print_r($object,1), serialize the object and get it out of the/an array could work too though.
	function lib4ridora_obj_date_last_modified(AbstractObject $object) {
		$objAry = (array) $object;
		$tmp = strchr( print_r($objAry,1),'[objLastModDate]'); // ugly but fast (quite complex structre/hirarchy with mixed arrays+objects)
		$tmp = substr(strchr($tmp,'[date]'),6);
		$tmp = trim( strtok($tmp."\n","\n"), ":=> \r\n\t\v\x00");
		return ( intval($tmp) ? $tmp : FALSE );
	}
}

if ( !function_exists('lib4ridora_ds_version_date_created') ) {
	// see https://wiki.lyrasis.org/display/ISLANDORA712/Build%2C+Access%2C+Modify+and+Delete+Fedora+objects+with+the+Tuque+interface
	function lib4ridora_ds_version_date_created($object, $dsName, $dsVersion = 0) {
		if ( $object && !empty($dsName) && isset($object[$dsName]) ) {
			if ( $dsVersion < 0 ) {
				$dsVersion = sizeof($object[$dsName]) - 1;	// to get index of the oldest version
			}
			return $object[$dsName][$dsVersion]->createdDate;
		}
		return false;
	}
}

if ( !function_exists('lib4ridora_log_trivial') ) {
	function lib4ridora_log_trivial($logRow, $logPath, $logMode = 'a') {
		if ( !empty($logRow) && !empty($logPath) && !empty($logMode) ) {
			if ( $fp = @fopen($logPath,$logMode) ) {
				fwrite( $fp, $logRow );
				fclose( $fp );
				return true;
			}
		}
		return false;
	}
}

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


foreach( $instAry as $instLowerCase => $instName ) {

	$modeCount = 0;
	foreach( $solrLinkAry as $runMode => $solrLink ) {
		$modeCount++;

		if ( $runMode != 'to-join-oa' && $runMode != 'to-leave-oa' ) { /* only these TWO run mods are supported! */ continue; }

		$solrLink = str_replace('lib-dora-prod1',@strval(exec('hostname')),$solrLink);		// DEV or PROD server?
		$solrLink = str_replace('eawag',$instLowerCase,$solrLink);	// respect other institutes
		$solrLink = str_replace('wt=csv&csv.separator=;','wt=json',$solrLink);  // JSON instead of CSV
		
		
		// RunMode- and institute-related Logs and cache files:	
		$timeStamp = date("Y-m-d-H").'h'; // 1-hour-stepping
		$tmpAry = [ $timeStamp, ( $instName == '*' ? 'lib4ri' : $instLowerCase ), $runMode ];
		$logInput = '/tmp/_oa_coll.' . implode('.',$tmpAry) . '.json';
		$logWork = '/tmp/_oa_coll.' . implode('.',$tmpAry) . '.log';

		
	//	echo "\r\n" . implode("\r\n",[$logInput,$logWork,$solrLink]) . "\r\n". "\r\n"; contine; // TEST


		// Get JSON file from Solr (caching if for the current hour):
		$pidAry = [];
		$jsonData = '';
		if ( @filesize($logInput) ) {
			$jsonData = @file_get_contents($logInput);
		} else {
			$jsonData = @file_get_contents($solrLink);
			@file_put_contents($logInput,$jsonData);
		}
		$pidAry = @json_decode( $jsonData, true );
		// echo print_r( $pidAry['response']['docs'], 1 ) . 'Total ' . $pidAry['response']['numFound'] . "\r\n"; return;
		$pidAry = $pidAry['response']['docs'];

	//	echo "\r\n" . print_r( $pidAry , 1 ) . "\r\n". "\r\n"; contine; // TEST
	/*
		$pidAry will contain now items like:
			[5936] => Array
				(
					[PID] => eawag:17980
					[fgs_lastModifiedDate_dt] => 2017-07-08T15:50:56.393Z
					[RELS_EXT_isMemberOfCollection_uri_mt] => Array
						(
							[0] => info:fedora/eawag:publications
						)

					[RELS_EXT_fullText_literal_mt] => Array
						(
							[0] => Open Access
						)

				)
	*/

		echo "\r\n" . print_r( $pidAry, 1 ) . "\r\n";
		

		foreach( $pidAry as $pAry ) {
			$ary = explode(':',$pAry['PID']);
			if ( intval($ary[0]) || !intval($ary[1]) ) { continue; }
			$pid = $ary[0].':'.rtrim($ary[1]);

			$logAction = 'skipped';
			if ( !( $object = @islandora_object_load($pid) ) ) {
				$logAction = 'not found';
			} else {
				$idColOA = strtok(strtr(strval($object->id),'-',':'),':') . ':open_access';
				
			//	$inColOA = in_array($idColOA, islandora_basic_collection_get_parent_pids($object) );
			//
			//	$relAry = $object->relationships->get(ISLANDORA_RELS_EXT_URI, 'fullText');
			//	$isPdfOA = ( @strtolower($relAry[0]['object']['value']) == 'open access' );
			// a (re)check makes only sense when we distrust Solr, or if we query both together.

				if ( /* !$isPdfOA && $inColOA */ $runMode == 'to-leave-oa' ) {
					// remove from collection, see islandora_basic_collection_remove_from_collection()
					$object->relationships->autoCommit = FALSE;
					$object->relationships->remove(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $idColOA );
					$object->relationships->remove(FEDORA_RELS_EXT_URI, 'isMemberOf', $idColOA );
					$object->relationships->commitRelationships();
			//		echo '-- ' . $pid . ' REMOVEDed from ' . $idColOA . ' collection --' . "\r\n";
					$logAction = 'removed';
				}
				else /* if ( $isPdfOA && !$inColOA  ) */ {
					// add to collection, see islandora_basic_collection_add_to_collection()
					$object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $idColOA );
			//		echo '-- ' . $pid . ' ASSIGNed to ' . $idColOA . ' collection --' . "\r\n";
					$logAction = 'added';
				}
			}
			lib4ridora_log_trivial( $pid.';'.date("Y-m-d H:i:s").';'.$logAction."\r\n", $logWork );


			if ( $logAction == 'skipped' || $logAction == 'not found' ) {
				sleep( $delayAry['skip'] );
				if ( !( @filesize($logInput) ) ) { /* exit condition */ break; }
				continue;
			}
			sleep( $delayAry['next'] );
			if ( !( @filesize($logInput) ) ) { /* exit condition */ break; }

			// wait/delay if CPU is stressed:
			while ( TRUE ) {
				$loadAry = sys_getloadavg(); // e.g. array( 0.7, 0.32, 0.1 )
				if ( ( $loadAry[0] * 100.0 ) > ( $cpuAry['cores'] * $cpuAry['max1min'] ) ) {
					sleep( $delayAry['wait'] );
				}
				elseif ( ( $loadAry[1] * 100.0 ) > ( $cpuAry['cores'] * $cpuAry['max5min'] ) ) {
					sleep( $delayAry['wait'] );
				}
				else { break; }
			}
		}

		if ( $modeCount < sizeof($solrLinkAry) ) { sleep( $delayAry['mode'] ); }
	}

	// wait for next institute (if not the last one):
	if ( array_slice($instAry,-1) != [$instLowerCase=>$instName] ) { sleep( $delayAry['inst'] ); }
}
?>
