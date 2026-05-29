/**
 * @file
 * Automatically will select in the ingest+editing form 'private' as availability
 * as soon as the document version is 'internal document'.
 * 
 */

<script type='text/javascript'><!--

// Automatical switching internal documents to 'private' availability
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

// Addtional selection boxes to compose license-string for PDFs:
function ccLicenseComposer() {
	var style = document.createElement('style');
	style.textContent = [
		'[class*="-use-enhanced"] { display: flex; align-items: center; flex-wrap: wrap; position:relative; top:-1.5em; }',
		'[class*="-use-enhanced"] > div { margin-right: 0; }',
		'select[id*="pdf"][id$="-use-permission"] { max-width: 450px; }'
	].join('\n');
	document.head.appendChild(style);

	var usePermSelects = document.querySelectorAll('select[id$="-use-permission"]');

	usePermSelects.forEach(function(sel) {
		// Store each option's original value so we can rebase cleanly on every change
		var ccFull = sel.options[sel.selectedIndex].value;
		var ccBase = ccFull.split(' ').slice(0,2).join(' ');
		let ccTmp = ccFull.split(' ').slice(2,5); // array!
		var ccDeed = ccTmp.find(d => /^[A-Z]{2,3}$/.test(d)) || null;
		var ccVer = ccFull.split(' ').find(v => /^\d+\.\d+/.test(v)) || null;
	//	console.log('[ccLicenseComposer] ' + sel.id + ' | ccFull:"' + ccFull + '" ccTmp:"' + ccTmp.join('/') + '" ccBase:"' + ccBase + '" ccVer:"' + ccVer + '" ccDeed:"' + ccDeed + '"');

		// Build the "License Version" label + select and inject them after the use-permission select
		var verLabel = document.createElement('label');
		verLabel.textContent = 'License Version ';

		var verSel = document.createElement('select');
		verSel.id = sel.id.replace('-use-permission', '-license-version');
		verSel.className = sel.className; // reuse Drupal form-select styling
	//	var versionList = ['---','1.0','2.0','2.5','3.0','4.0']; // 3 dashes ('---') will mean no selection
		var versionList = ['---','0.0']; // future PHP-replacement-placeholder
		versionList.forEach(function(v) {
			var opt = document.createElement('option');
			opt.value = v;
			opt.textContent = v;
			verSel.appendChild(opt);
		});
		verSel.value = ccVer || versionList[0];
		verLabel.htmlFor = verSel.id;

		// Derive specific class from the parent form-item div
		// e.g. form-item-files-PDF-use-permission => used as base for -license-version and -use-enhanced
		var usePermDiv = sel.closest('div.form-item');
		var specificClass = usePermDiv
			? Array.from(usePermDiv.classList).find(function(c) {
				return c.startsWith('form-item-') && c.endsWith('-use-permission');
			})
			: null;

		let wrapV = document.createElement('div');
		wrapV.className = 'form-item form-type-select';
		if (specificClass) {
			wrapV.classList.add(specificClass.replace('-use-permission', '-license-version'));
		}
		wrapV.style.display = 'inline-block';
		wrapV.style.marginLeft = '1em';
		wrapV.appendChild(verLabel);
		wrapV.appendChild(verSel);


		// Build the "Localization" (= 'deed'!?) label + select
		var deedLabel = document.createElement('label');
		deedLabel.textContent = 'Localization ';

		var deedSel = document.createElement('select');
		deedSel.id = sel.id.replace('-use-permission', '-deed-adaption');
		deedSel.className = sel.className;
	//	var countryList = ['---','IGO','UK']; // 3 dashes ('---') will mean no selection
		var countryList = ['---','XYZ']; // future PHP-replacement-placeholder
		countryList.forEach(function(d) {
			var opt = document.createElement('option');
			opt.value = d;
			opt.textContent = d;
			deedSel.appendChild(opt);
		});
		deedSel.value = ccDeed || countryList[0];
		deedLabel.htmlFor = deedSel.id;

		var wrapD = document.createElement('div');
		wrapD.className = 'form-item form-type-select';
		if (specificClass) {
			wrapD.classList.add(specificClass.replace('-use-permission', '-deed-adaption'));
		}
		wrapD.style.display = 'inline-block';
		wrapD.style.marginLeft = '1em';
		wrapD.appendChild(deedLabel);
		wrapD.appendChild(deedSel);

		// Wrap clones of use-permission div and the license-version div in an wrapMain container
		var wrapMain = document.createElement('div');
		if (specificClass) {
			wrapMain.className = specificClass.replace('-use-permission', '-use-enhanced');
		}
		var usePermClone = usePermDiv.cloneNode(true);
		var cloneSel = usePermClone.querySelector('select');
		if (cloneSel) {
			var oldId = cloneSel.id;
			var newId = oldId.replace('-use-permission', '-use-perm-manual');
			cloneSel.id = newId;
			var cloneLabel = usePermClone.querySelector('label[for="' + oldId + '"]');
			if (cloneLabel) { cloneLabel.htmlFor = newId; }
			
			Array.from(cloneSel.options).forEach(function(opt) {
				ccTmp = opt.value + " ";
				ccTmp = ccTmp.split(' ').slice(0,2).join(' ');
				opt.dataset.baseValue = ccTmp;
				// only show those option with full/extended description:
				if ( cloneSel.options[cloneSel.selectedIndex] != opt.value && opt.innerHTML.startsWith("CC") ) {
					opt.hidden = true;
					opt.disabled = true;
				}
			});

			// Initialize cloneSel to the base license type from the existing selection
			for (let i = 0; i < cloneSel.options.length; i++) {
				if (cloneSel.options[i].dataset.baseValue === ccBase) {
					cloneSel.selectedIndex = i;
					cloneSel.options[i].hidden = false;
					cloneSel.options[i].disabled = false;
					break;
				}
			}

		}
		wrapMain.appendChild(usePermClone);
		wrapMain.appendChild(wrapV);
		wrapMain.appendChild(wrapD);
		usePermDiv.parentNode.insertBefore(wrapMain, usePermDiv);
		// Hide the original/native selection box for the re-use permission:
		usePermDiv.style.display = 'none';

		function composeLicense() {
			if (!cloneSel || !cloneSel.value) return;
			var parts = [cloneSel.value];
			if (verSel.value !== '---') parts.push(verSel.value);
			if (deedSel.value !== '---') parts.push(deedSel.value);
			sel.value = parts.join(' ');
			var pdfId = sel.id.replace('edit-files-', '').replace('-use-permission', '').toUpperCase();
			console.log('[use-permission] ' + pdfId + ': "' + sel.value + '"');
		}

		if (cloneSel) {
			cloneSel.addEventListener('change', composeLicense);
			verSel.addEventListener('change', composeLicense);
			deedSel.addEventListener('change', composeLicense);
		}

		var form = sel.closest('form');
		if (form) form.addEventListener('submit', composeLicense);
	});
}

document.addEventListener('DOMContentLoaded',ccLicenseComposer);
	
//--></script>
