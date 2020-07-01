/**
 * @file
 * Automatically will select in the ingest+editing form 'private' as availability
 * as soon as the document version is 'internal document'.
 * 
 */

<script type="text/javascript"><!--

function PdfInternalAsPrivate(uuOrPdfId) {
	var docVer = document.getElementById("edit-files-" + uuOrPdfId + "-document-version");
	if ( docVer != null && docVer.value == 'internal document' ) {
		var avail = document.getElementById("edit-files-" + uuOrPdfId + "-availability");
		for(c=0;c<avail.options.length;c++) {
			if ( avail.options[c].value == "private" ) { avail.selectedIndex = c; break; }
		}
	}
}

//--></script>
