/**
 * @file
 * Automatically will select in the ingest+editing form 'private' as availability
 * as soon as the document version is 'internal document'.
 * 
 */

<script type="text/javascript"><!--

function PdfInternalAsPrivate(uuOrPdfId) {
	var expId = ( uuOrPdfId != null && uuOrPdfId != '' ) ? ( 's-' + uuOrPdfId ) : '';
	var docVer = document.getElementById("edit-file" + expId + "-document-version");
	if ( docVer != null && docVer.value == 'internal document' ) {
		var avail = document.getElementById("edit-file" + expId + "-availability");
		for(c=0;c<avail.options.length;c++) {
			if ( avail.options[c].value == "private" ) { avail.selectedIndex = c; break; }
		}
	}
}

//--></script>