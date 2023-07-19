<?php

/**
 * @file
 * Batch functionality for exporting objects in a BiBTeX format.
 */

/**
 * Builds up batch operations for BiBTeX export.
 *
 * @param array $pids
 *   An array of PIDs to be processed.
 *
 * @return array
 *   An array containing the batch operations to be executed.
 */
function lib4ridora_bibtex_batch_export_operations(array $pids) {
  module_load_include('inc', 'citation_exporter', 'includes/db');
  $temp_file_dir = file_directory_temp();
  $temp_text_file = file_create_filename("BiBTeX_export.txt", $temp_file_dir);
  $set_id = citation_exporter_create_set($pids);
  $total = count($pids);
  $operations = [];

  $path = drupal_get_path('module', 'lib4ridora');
  $style = [
    'name' => t('LIB4Ri BiBTeX'),
    'data' => file_get_contents("$path/xml/csl/bibtex.csl"),
  ];
  $operations[] = [
    'lib4ridora_batch_append_citations_to_file',
    [$set_id, $total, $temp_text_file, $style],
  ];
  $operations[] = [
    'lib4ridora_convert_txt_to_bib',
    [$temp_text_file],
  ];
  $operations[] = [
    'citation_exporter_batch_redirect',
    [
      url(request_path(), ['query' => drupal_get_query_parameters()]),
      [$temp_text_file],
      $set_id,
    ],
  ];
  return $operations;
}

/**
 * Generates and places BiBTeX representations into a file.
 *
 * @param string $set_id
 *   The ID of the set being built.
 * @param int $total
 *   The total number of PIDs to process.
 * @param string $text_filename
 *   The name of the file that the generated BiBTeX will be output to.
 * @param array $style
 *   An array representing the CSL style to be used, containing:
 *   - name (string): The name of the CSL.
 *   - data (string): The CSL XML to be used by Citeproc.
 * @param array $context
 *   The Drupal batch context.
 *
 * @see citation_exporter_batch_append_citations_to_text_file
 */
function lib4ridora_batch_append_citations_to_file($set_id, $total, $text_filename, array $style, array &$context) {
  module_load_include('inc', 'citation_exporter', 'includes/db');
  module_load_include('inc', 'citeproc', 'includes/converter');

  if (empty($context['sandbox'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['total'] = $total;
  }
  $pids = citation_exporter_get_pids($set_id);
  $context['sandbox']['progress'] += count($pids);

  $citeproc = citeproc_get_citeproc_php_instance($style);
  $file = fopen($text_filename, 'ab');
  foreach ($pids as $pid) {
    $object = islandora_object_load($pid);
    if (isset($object) && isset($object['MODS']) && islandora_datastream_access(ISLANDORA_VIEW_OBJECTS, $object['MODS'])) {
      $mods = trim($object['MODS']->content);
      if (!empty($mods)) {
        $citeproc_array = convert_mods_to_citeproc_jsons($mods);
        $json = lib4ridora_convert_citeproc_array_to_object($citeproc_array);
        lib4ridora_add_citation_key($json);
        $text = $citeproc->render($json, 'bibliography');
        // XXX: Depending on the source input it's possible to get into a state
        // where the source MODS contain invalid characters and so forth.
        // When this is encountered log the error the watchdog and do not
        // include it in the output.
        $xml_to_test = "<div>$text</div>";
//dsm($xml_to_test, 'output');
        if (!@simplexml_load_string($xml_to_test)) {
          watchdog('lib4ridora', 'Unable to process BiBTeX output for @pid, encountered invalid values.', ['@pid' => $object->id]);
          continue;
        }
        if ($file) {
          fwrite($file, "$text");
        }
      }
    }
  }
  fclose($file);

  citation_exporter_drop_pids($set_id, $pids);

  if ($context['sandbox']['progress'] < $context['sandbox']['total']) {
    $context['message'] = t('Gathered metadata: @progress/@total', [
      '@progress' => $context['sandbox']['progress'],
      '@total' => $context['sandbox']['total'],
    ]);
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['total'];
  }
}

/**
 * Replaces special strings/characters with their BiBTeX representations.
 *
 * @param string $value
 *   The raw value from MODS being processed.
 *
 * @return string
 *   A string with BiBTeX replacements included.
 *
 * @note This mapping was directly provided by Lib4RI.
 */
function lib4ridora_replace_to_bibtex($value) {

  $caps_value = $value;
  return strtr($caps_value, [
    // HTML elements.
    '<sub>' => '\textsubscript{',
    '</sub>' => '}',
    '<sup>' => '\textsuperscript{',
    '</sup>' => '}',
    '<em>' => '\emph{',
    '</em>' => '}',
    '<i>' => '\emph{',
    '</i>' => '}',

    '&lt;sub&gt;' => '\textsubscript{',
    '&lt;/sub&gt;' => '}',
    '&lt;sup&gt;' => '\textsuperscript{',
    '&lt;/sup&gt;' => '}',
    '&lt;em&gt;' => '\emph{',
    '&lt;/em&gt;' => '}',
    '&lt;i&gt;' => '\emph{',
    '&lt;/i&gt;' => '}',

    '&lt;SUB&gt;' => '\textsubscript{',
    '&lt;/SUB&gt;' => '}',
    '&lt;SUP&gt;' => '\textsuperscript{',
    '&lt;/SUP&gt;' => '}',
    '&lt;EM&gt;' => '\emph{',
    '&lt;/EM&gt;' => '}',
    '&lt;I&gt;' => '\emph{',
    '&lt;/I&gt;' => '}',
      
    // Entities.
    '&prime;' => '{$\prime$}',
    '&Prime;' => '{$\second$}',
    '&deg;' => '{\textdegree}',
    '&nbsp;' => '~',
    '&raquo;' => '{\guillemotright}',
    '&laquo;' => '{\guillemotleft}',
    '&rdquo;' => '{\textquotedblright}',
    '&ldquo;' => '{\textquotedblleft}',
    '&rsquo;' => '{\textquoteright}',
    '&lsquo;' => '{\textquoteleft}',
    '&Alpha;' => '$Α$',
    '&Beta;' => '$Β$',
    '&Gamma;' => '$\Gamma$',
    '&Delta;' => '$\Delta$',
    '&Epsilon;' => '$Ε$',
    '&Zeta;' => '$Ζ$',
    '&Eta;' => '$Η$',
    '&Theta;' => '$\Theta$',
    '&Iota;' => '$Ι$',
    '&Kappa;' => '$Κ$',
    '&Lambda;' => '$\Lambda$',
    '&Mu;' => '$Μ$',
    '&Nu;' => '$Ν$',
    '&Xi;' => '$Xi$',
    '&Omicron;' => '$Ο$',
    '&Pi;' => '$\Pi$',
    '&Rho;' => '$Ρ$',
    '&Sigma;' => '$\Sigma$',
    '&Tau;' => '$Τ$',
    '&Upsilon;' => '$\Upsilon$',
    '&Phi;' => '$\Phi$',
    '&Chi;' => '$Χ$',
    '&Psi;' => '$\Psi$',
    '&Omega;' => '$\Omega$',
    '&alpha;' => '$\alpha$',
    '&beta;' => '$\beta$',
    '&gamma;' => '$\gamma$',
    '&delta;' => '$\delta$',
    '&epsilon;' => '$\epsilon$',
    '&zeta;' => '$\zeta$',
    '&eta;' => '$\eta$',
    '&theta;' => '$\theta$',
    '&iota;' => '$\iota$',
    '&kappa;' => '$\kappa$',
    '&lambda;' => '$\lambda$',
    '&mu;' => '$\mu$',
    '&nu;' => '$\nu$',
    '&xi;' => '$\xi$',
    '&omicron;' => '$o$',
    '&pi;' => '$\pi$',
    '&rho;' => '$\rho$',
    '&sigmaf;' => '$\varsigma$',
    '&sigma;' => '$\sigma$',
    '&tau;' => '$\tau$',
    '&upsilon;' => '$\upsilon$',
    '&phi;' => '$\phi$',
    '&chi;' => '$\chi$',
    '&psi;' => '$\psi$',
    '&omega;' => '$\omega$',
    '&micro;' => '$\mathrm{\mu}$',
    '&thetasym;' => '$\vartheta$',
    '&upsih;' => '$\Upsilon$',
    '&#913;' => '$Α$',
    '&#914;' => '$Β$',
    '&#915;' => '$\Gamma$',
    '&#916;' => '$\Delta$',
    '&#917;' => '$Ε$',
    '&#918;' => '$Ζ$',
    '&#919;' => '$Η$',
    '&#920;' => '$\Theta$',
    '&#921;' => '$Ι$',
    '&#922;' => '$Κ$',
    '&#923;' => '$\Lambda$',
    '&#924;' => '$Μ$',
    '&#925;' => '$Ν$',
    '&#926;' => '$Xi$',
    '&#927;' => '$Ο$',
    '&#928;' => '$\Pi$',
    '&#929;' => '$Ρ$',
    '&#931;' => '$\Sigma$',
    '&#932;' => '$Τ$',
    '&#933;' => '$\Upsilon$',
    '&#934;' => '$\Phi$',
    '&#935;' => '$Χ$',
    '&#936;' => '$\Psi$',
    '&#937;' => '$\Omega$',
    '&#945;' => '$\alpha$',
    '&#946;' => '$\beta$',
    '&#947;' => '$\gamma$',
    '&#948;' => '$\delta$',
    '&#949;' => '$\epsilon$',
    '&#950;' => '$\zeta$',
    '&#951;' => '$\eta$',
    '&#952;' => '$\theta$',
    '&#953;' => '$\iota$',
    '&#954;' => '$\kappa$',
    '&#955;' => '$\lambda$',
    '&#956;' => '$\mu$',
    '&#957;' => '$\nu$',
    '&#958;' => '$\xi$',
    '&#959;' => '$o$',
    '&#960;' => '$\pi$',
    '&#961;' => '$\rho$',
    '&#962;' => '$\varsigma$',
    '&#963;' => '$\sigma$',
    '&#964;' => '$\tau$',
    '&#965;' => '$\upsilon$',
    '&#966;' => '$\phi$',
    '&#967;' => '$\chi$',
    '&#968;' => '$\psi$',
    '&#969;' => '$\omega$',
    '&#977;' => '$\vartheta$',
    '&#978;' => '$\Upsilon$',
    '&#x391;' => '$Α$',
    '&#x392;' => '$Β$',
    '&#x393;' => '$\Gamma$',
    '&#x394;' => '$\Delta$',
    '&#x395;' => '$Ε$',
    '&#x396;' => '$Ζ$',
    '&#x397;' => '$Η$',
    '&#x398;' => '$\Theta$',
    '&#x399;' => '$Ι$',
    '&#x39A;' => '$Κ$',
    '&#x39B;' => '$\Lambda$',
    '&#x39C;' => '$Μ$',
    '&#x39D;' => '$Ν$',
    '&#x39E;' => '$Xi$',
    '&#x39F;' => '$Ο$',
    '&#x3A0;' => '$\Pi$',
    '&#x3A1;' => '$Ρ$',
    '&#x3A3;' => '$\Sigma$',
    '&#x3A4;' => '$Τ$',
    '&#x3A5;' => '$\Upsilon$',
    '&#x3A6;' => '$\Phi$',
    '&#x3A7;' => '$Χ$',
    '&#x3A8;' => '$\Psi$',
    '&#x3A9;' => '$\Omega$',
    '&#x3B1;' => '$\alpha$',
    '&#x3B2;' => '$\beta$',
    '&#x3B3;' => '$\gamma$',
    '&#x3B4;' => '$\delta$',
    '&#x3B5;' => '$\epsilon$',
    '&#x3B6;' => '$\zeta$',
    '&#x3B7;' => '$\eta$',
    '&#x3B8;' => '$\theta$',
    '&#x3B9;' => '$\iota$',
    '&#x3BA;' => '$\kappa$',
    '&#x3BB;' => '$\lambda$',
    '&#x3BC;' => '$\mu$',
    '&#x3BD;' => '$\nu$',
    '&#x3BE;' => '$\xi$',
    '&#x3BF;' => '$o$',
    '&#x3C0;' => '$\pi$',
    '&#x3C1;' => '$\rho$',
    '&#x3C2;' => '$\varsigma$',
    '&#x3C3;' => '$\sigma$',
    '&#x3C4;' => '$\tau$',
    '&#x3C5;' => '$\upsilon$',
    '&#x3C6;' => '$\phi$',
    '&#x3C7;' => '$\chi$',
    '&#x3C8;' => '$\psi$',
    '&#x3C9;' => '$\omega$',
    '&#x3D1;' => '$\vartheta$',
    '&#x3D2;' => '$\Upsilon$',
    '&larr;' => '$\leftarrow$',
    '&uarr;' => '$\uparrow$',
    '&rarr;' => '$\rightarrow$',
    '&darr;' => '$\downarrow$',
    '&harr;' => '$\leftrightarrow$',
    '&lArr;' => '$\Leftarrow$',
    '&uArr;' => '$\Uparrow$',
    '&rArr;' => '$\Rightarrow$',
    '&dArr;' => '$\Downarrow$',
    '&hArr;' => '$\Leftrightarrow$',
    '&forall;' => '$\forall$',
    '&part;' => '$\partial$',
    '&exist;' => '$\exists$',
    '&empty;' => '$\emptyset$',
    '&nabla;' => '$\nabla$',
    '&isin;' => '$\in$',
    '&notin;' => '$\notin$',
    '&ni;' => '$\ni$',
    '&prod;' => '$\prod$',
    '&sum;' => '$\sum$',
    '&minus;' => '\textminus',
    '&lowast;' => '$\ast$',
    '&radic;' => '$\surd$',
    '&prop;' => '$\propto$',
    '&infin;' => '$\infty$',
    '&and;' => '$\wedge$',
    '&or;' => '$\vee$',
    '&cap;' => '$\cap$',
    '&cup;' => '$\cup$',
    '&int;' => '$\int$',
    '&sim;' => '$\sim$',
    '&cong;' => '$\approxeq$',
    '&asymp;' => '$\approx$',
    '&ne;' => '$\neq$',
    '&equiv;' => '$\equiv$',
    '&le;' => '$\leq$',
    '&ge;' => '$\geq$',
    '&sub;' => '$\subset$',
    '&sup;' => '$\supset$',
    '&nsub;' => '$\nsubset$',
    '&sube;' => '$\subseteq$',
    '&supe;' => '$\supseteq$',
    '&oplus;' => '$\oplus$',
    '&otimes;' => '$\otimes$',
    '&#8592;' => '$\leftarrow$',
    '&#8593;' => '$\uparrow$',
    '&#8594;' => '$\rightarrow$',
    '&#8595;' => '$\downarrow$',
    '&#8596;' => '$\leftrightarrow$',
    '&#8656;' => '$\Leftarrow$',
    '&#8657;' => '$\Uparrow$',
    '&#8658;' => '$\Rightarrow$',
    '&#8659;' => '$\Downarrow$',
    '&#8660;' => '$\Leftrightarrow$',
    '&#8704;' => '$\forall$',
    '&#8706;' => '$\partial$',
    '&#8707;' => '$\exists$',
    '&#8709;' => '$\emptyset$',
    '&#8711;' => '$\nabla$',
    '&#8712;' => '$\in$',
    '&#8713;' => '$\notin$',
    '&#8715;' => '$\ni$',
    '&#8719;' => '$\prod$',
    '&#8721;' => '$\sum$',
    '&#8722;' => '\textminus',
    '&#8727;' => '$\ast$',
    '&#8730;' => '$\surd$',
    '&#8733;' => '$\propto$',
    '&#8734;' => '$\infty$',
    '&#8743;' => '$\wedge$',
    '&#8744;' => '$\vee$',
    '&#8745;' => '$\cap$',
    '&#8746;' => '$\cup$',
    '&#8747;' => '$\int$',
    '&#8764;' => '$\sim$',
    '&#8773;' => '$\approxeq$',
    '&#8776;' => '$\approx$',
    '&#8800;' => '$\neq$',
    '&#8801;' => '$\equiv$',
    '&#8804;' => '$\leq$',
    '&#8805;' => '$\geq$',
    '&#8834;' => '$\subset$',
    '&#8835;' => '$\supset$',
    '&#8836;' => '$\nsubset$',
    '&#8838;' => '$\subseteq$',
    '&#8839;' => '$\supseteq$',
    '&#8853;' => '$\oplus$',
    '&#8855;' => '$\otimes$',
    '&#x2190;' => '$\leftarrow$',
    '&#x2191;' => '$\uparrow$',
    '&#x2192;' => '$\rightarrow$',
    '&#x2193;' => '$\downarrow$',
    '&#x2194;' => '$\leftrightarrow$',
    '&#x21D0;' => '$\Leftarrow$',
    '&#x21D1;' => '$\Uparrow$',
    '&#x21D2;' => '$\Rightarrow$',
    '&#x21D3;' => '$\Downarrow$',
    '&#x21D4;' => '$\Leftrightarrow$',
    '&#x2200;' => '$\forall$',
    '&#x2202;' => '$\partial$',
    '&#x2203;' => '$\exists$',
    '&#x2205;' => '$\emptyset$',
    '&#x2207;' => '$\nabla$',
    '&#x2208;' => '$\in$',
    '&#x2209;' => '$\notin$',
    '&#x220B;' => '$\ni$',
    '&#x220F;' => '$\prod$',
    '&#x2211;' => '$\sum$',
    '&#x2212;' => '\textminus',
    '&#x2217;' => '$\ast$',
    '&#x221A;' => '$\surd$',
    '&#x221D;' => '$\propto$',
    '&#x221E;' => '$\infty$',
    '&#x2227;' => '$\wedge$',
    '&#x2228;' => '$\vee$',
    '&#x2229;' => '$\cap$',
    '&#x222A;' => '$\cup$',
    '&#x222B;' => '$\int$',
    '&#x223C;' => '$\sim$',
    '&#x2245;' => '$\approxeq$',
    '&#x2248;' => '$\approx$',
    '&#x2260;' => '$\neq$',
    '&#x2261;' => '$\equiv$',
    '&#x2264;' => '$\leq$',
    '&#x2265;' => '$\geq$',
    '&#x2282;' => '$\subset$',
    '&#x2283;' => '$\supset$',
    '&#x2284;' => '$\nsubset$',
    '&#x2286;' => '$\subseteq$',
    '&#x2287;' => '$\supseteq$',
    '&#x2295;' => '$\oplus$',
    '&#x2297;' => '$\otimes$',
    '&#x2013;' => '--',
    '&ndash;' => '{\textendash}',
    '&mdash;' => '{\textemdash}',
    '&shy;' => '\-',  
    '&Aacute;' => "\'{A}",
    '&aacute;' => "\'{a}",
    '&Agrave;' => '\`{A}',
    '&agrave;' => '\`{a}',
    '&Acirc;' => '\^{A}',
    '&acirc;' => '\^{a}',
    '&Auml;' => '\"{A}',
    '&auml;' => '\"{a}',
    '&Atilde;' => '\~{A}',
    '&atilde;' => '\~{a}',
    '&Aring;' => '\°{A}',
    '&aring;' => '\°{a}',
    '&Aelig;' => '\AE',
    '&aelig;' => '\ae',
    '&Ccedil;' => '\c{C}',
    '&ccedil;' => '\c{c}',
    '&Eth;' => '\DH',
    '&eth;' => '\dh',
    '&Eacute;' => "\'{E}",
    '&eacute;' => "\'{e}",
    '&Egrave;' => '\`{E}',
    '&egrave;' => '\`{e}',
    '&Ecirc;' => '\^{E}',
    '&ecirc;' => '\^{e}',
    '&Euml;' => '\"{E}',
    '&euml;' => '\"{e}',
    '&Iacute;' => "\'{I}",
    '&iacute;' => "\'{i}",
    '&Igrave;' => '\`{I}',
    '&igrave;' => '\`{i}',
    '&Icirc;' => '\^{I}',
    '&icirc;' => '\^{i}',
    '&Iuml;' => '\"{I}',
    '&iuml;' => '\"{e}',
    '&Ntilde;' => '\~{N}',
    '&ntilde;' => '\~{n}',
    '&Oacute;' => "\'{O}",
    '&oacute;' => "\'{o}",
    '&Ograve;' => '\`{O}',
    '&ograve;' => '\`{o}',
    '&Ocirc;' => '\^{O}',
    '&ocirc;' => '\^{o}',
    '&Ouml;' => '\"{O}',
    '&ouml;' => '\"{o}',
    '&Otilde;' => '\~{O}',
    '&otilde;' => '\~{o}',
    '&Oslash;' => '\O',
    '&oslash;' => '\o',
    '&szlig;' => '\ss',
    '&Uacute;' => "\'{U}",
    '&uacute;' => "\'{u}",
    '&Ugrave;' => '\`{U}',
    '&ugrave;' => '\`{u}',
    '&Ucirc;' => '\^{U}',
    '&ucirc;' => '\^{u}',
    '&Uuml;' => '\"{U}',
    '&uuml;' => '\"{u}',
    '&Yacute;' => "\'{Y}",
    '&yacute;' => "\'{y}",
    '&yuml;' => '\"{y}',
    '&scaron;' => '\u{s}',
    '&Scaron;' => '\u{S}',
    '&reg;' => '{\textregistered}',

    // Special cases.
    '&' => '\&',
    '%' => '\%',
    '$' => '\$',
    '#' => '\#',
    '_' => '\_',
    '{' => '\{',
    '}' => '\}',
    '~' => '\textasciitilde',
    '^' => '\textasciicircum',
    '\\' => '\textslash',
    '"' => '{"}',

    // Capitals.
    'A' => '{A}',
    'B' => '{B}',
    'C' => '{C}',
    'D' => '{D}',
    'E' => '{E}',
    'F' => '{F}',
    'G' => '{G}',
    'H' => '{H}',
    'I' => '{I}',
    'J' => '{J}',
    'K' => '{K}',
    'L' => '{L}',
    'M' => '{M}',
    'N' => '{N}',
    'O' => '{O}',
    'P' => '{P}',
    'Q' => '{Q}',
    'R' => '{R}',
    'S' => '{S}',
    'T' => '{T}',
    'U' => '{U}',
    'V' => '{V}',
    'W' => '{W}',
    'X' => '{X}',
    'Y' => '{Y}',
    'Z' => '{Z}',
  ]);
}

/**
 * Wraps the core citeproc conversion with BiBTeX replacements.
 *
 * @see _citeproc_array_to_object
 */
function lib4ridora_convert_citeproc_array_to_object($in) {

  if (!is_array($in)) {
    // XXX: SimpleXML likes to replace already encoded entities earlier up in
    // the Citeproc chain back to their unencoded values. Once again encode
    // them.
      return lib4ridora_replace_to_bibtex(htmlentities(html_entity_decode($in)));
  }
  elseif (count($in) > 0 && count(array_filter(array_keys($in), 'is_int')) == count($in)) {
    foreach ($in as &$value) {
      $value = lib4ridora_convert_citeproc_array_to_object($value);
    }
    return $in;
  }
  elseif (count($in) > 0 && count(array_filter(array_keys($in), 'is_string')) == count($in)) {
    $obj = new stdClass();
    foreach ($in as $key => $val) {
      if ($val) {
        $obj->$key = lib4ridora_convert_citeproc_array_to_object($val);
      }
    }
    return $obj;
  }
  else {
    return FALSE;
  }
}

/**
 * Helper that generates a valid citation-key for the BiBTeX output.
 *
 * @param object $json
 *   The JSON object created by Citeproc-PHP.
 */
function lib4ridora_add_citation_key(&$json) {
  // XXX: Citation key format doesn't have a hard format denoted by it. For
  // example Zotero and Google Scholar prefer different entries. The rules
  // around capitalization throws a wrench into the default CSL that is being
  // leveraged. Thus, we'll build up the citation-key ourselves in PHP. The
  // below is very much based off what the CSL is doing. That is use
  // the authors' last name(s) if present suffixed with the year.
  // Similarly, Zotero doesn't support much of anything beyond English
  // alphanumeric characters. It could be possible to pull in a transliteration
  // library to make this much nicer, however it would require bundling an
  // external module or library. Given that citation keys pure requirement are
  // being unique just replace it with nothing.
  $author_sanitization = function ($string) {
    $capital_replacements = [
      '{' => '',
      '}' => '',
    ];
    return preg_replace('/[^\w\s-]/', '', strtr($string, $capital_replacements));
  };
  $authors = [];
  if (isset($json->author)) {
    foreach ($json->author as $author) {
      $authors[] = $author_sanitization($author->family);
    }
  }
  // When there's no authors fallback to looking at editor and translator.
  if (empty($authors)) {
    if (isset($json->editor)) {
      foreach ($json->editor as $editor) {
        $authors[] = $author_sanitization($editor->family);
      }
    }
    if (isset($json->translator)) {
      foreach ($json->translator as $translator) {
        $authors[] = $author_sanitization($translator->family);
      }
    }
  }
  if (!empty($authors)) {
    if (count($authors) > 10) {
      $authors = array_slice($authors, 0, 10);
      $author_string = format_string('!authors_et_al', [
        '!authors' => implode('_', $authors),
      ]);
    }
    else {
      $author_string = implode('_', $authors);
    }
  }
  else {
    // XXX: Fallback to something unique.
    $author_string = uniqid();
  }

  $citation_key = format_string('!author!year', [
    '!author' => strtolower($author_string),
    // [0][0] is the first date's year.
    '!year' => isset($json->issued->{'date-parts'}) ? "_{$json->issued->{'date-parts'}[0][0]}" : '',
  ]);
  // XXX: This should really go into "citation-key" but that was introduced in
  // CSL 1.0.2. Similarly, the "citation-label" isn't used by Islandora's
  // implementation at this time so jam it in there.
  $json->{'citation-label'} = str_replace(' ', "_", $citation_key);
}

/**
 * Helper that removes Citeproc-PHP's markup and outputs only the BiBTeX.
 *
 * @param string $filename
 *   The file to output to.
 * @param array $context
 *   The Drupal batch context.
 */
function lib4ridora_convert_txt_to_bib($filename, array &$context) {
  $text = file_get_contents($filename);

  // XXX: To make the Citeproc markup parseable wrap it in a div to make it a
  // full document.
  $to_parse = "<div>$text</div>";
  $dom = new DOMDocument();
  $dom->loadXML($to_parse);
  $xpath = new DOMXPath($dom);
  $nodes = $xpath->query('//text()');
  $entries = [];
  foreach ($nodes as $node) {
    $entries[] = $node->nodeValue;
  }
  $parsed_text = implode("\n", $entries);
  $temp_file_dir = file_directory_temp();
  $temp_file_name = file_create_filename("BiBTeX_export.bib", $temp_file_dir);
  $parsed_text = str_replace('_attribute_left_delimiter_', '{', $parsed_text);
  $parsed_text = str_replace('_attribute_right_delimiter_', '}', $parsed_text);
  file_put_contents($temp_file_name, $parsed_text);
  $context['results']['filename'] = $temp_file_name;
  $context['results']['download_filename'] = 'export.bib';
  $context['results']['mime_type'] = 'application/x-bibtex';
}
