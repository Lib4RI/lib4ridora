<?php
/**
 * @file
 * Template file for the listing of all PDFs available.
 */
?>
<div class="<?php print $classes; ?>">
  <?php if ( @!empty($variables['pdf_listing']) ) { print $variables['pdf_listing']; } ?>
</div>
