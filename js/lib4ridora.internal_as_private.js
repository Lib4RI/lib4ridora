/**
 * @file
 * Automatically will select in the ingest+editing form 'private' as availability
 * as soon as the document version is 'internal document'.
 * 
 */

<script type='text/javascript'><!--
	
function PdfInternalAsPrivate(uuOrPdfId) {
	var expId = ( uuOrPdfId != null && uuOrPdfId != '' ) ? ( 's-' + uuOrPdfId ) : '';
	var docVer = document.getElementById('edit-file' + expId + '-document-version');
	var avail = document.getElementById('edit-file' + expId + '-availability');
	if ( docVer != null && avail != null ) {
		var eStyle = '';
		if ( docVer.value == 'internal document' ) {
			for(c=0;c<avail.options.length;c++) {
				if ( avail.options[c].value == 'private' ) {
					avail.selectedIndex = c;
					eStyle = 'none';
					break;
				}
			}
		}
		if ( eStyle == '' ) {
			for(c=0;c<avail.options.length;c++) {
				if ( avail.options[c].value == 'date' ) {
					eStyle = 'block';
					break;
				}
			}
		}
		if ( eStyle != '' ) {
			var eTmp = document.getElementById('edit-file' + expId + '-embargo-date');
			if ( eTmp != null ) {
				eTmp.style.display = eStyle;
			}
		}
	}
}
	
//--></script>
