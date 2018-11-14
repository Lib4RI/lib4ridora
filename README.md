# Lib4ridora

## Introduction

A module for specific customizations for the Library for the Research Institutes.

## Requirements

This module requires the following modules/libraries:

* [Islandora](https://github.com/islandora/islandora)
* [Islandora Scholar](https://github.com/islandora/islandora_scholar)
* [Islandora Plupload](https://github.com/discoverygarden/islandora_plupload)
* [Islandora Solr Metadata](https://github.com/Islandora/islandora_solr_metadata)

## Installation

Install as usual, see [this](https://drupal.org/documentation/install/modules-themes/modules-7) for further information.

## Usage

### XML Form Field Filtering

Fields in Islandora XML forms can be filtered by either role or permissions, by settings some properties in the "User Data" (under "More Advanced Controls", the third tab in the form builder; corresponds to `#user_data` in the form element array).

First, to activate this behaviour on an element (and all sub-elements), add `lib4ridora_field_filter`, with the value of `true`.

Then:
* To add a restriction to a single role to a field, add the property `lib4ridora_field_filter_role` on the particular field you wish to restrict. The value of the property must be the name of the role. To allow multiple roles, properties can be added prefixed with `lib4ridora_field_filter_role_` and ending with an arbitrary suffix. Note that properties on each field must be unique (attempting to specify multiple properties with the same name will result in only the last one being applied).
* To add a permission restriction, the same process as roles may be used, substituting the string "permission" for role: So properties named `lib4ridora_field_filter_permission` or prefixed with `lib4ridora_field_filter_permission_`, with the value being the name of the permission as might be passed to [`user_access()`](https://api.drupal.org/api/drupal/modules!user!user.module/function/user_access/7). Note that the Drupal's superuser (user '1') will always have permission when using permission restrictions.

Role and permission restrictions may be combined in the same element.

When restrictions are specified on an element, the element (and its children) will only be shown if the current user has one of the specified roles or one of the specified permissions.

For example, if one wanted to allow access to an element to those with the "Replace datastreams" permission or the "metadata manager" or "repository administrator" roles, your configuration might look like:
![Screenshot](http://puu.sh/dgOMH/ad3d3d7964.png)

### Islandora Solr Metadata Author and Org linking

The `lib4ridora_mods_pseudo_field` pseudo-field has been introduced to produce markup for authors in Solr Metadata displays. The produced markup should create a link to both:
* Documents by the same author
* Documents from the same affiliated organization

### Context-based Islandora Solr Metadata Displays

The ability to associate Solr metadata configurations according to the MODS genre on `ir:citationCModel` objects has been facilitated via the menu path at `admin/islandora/solution_pack_config/lib4ridora/solr_metadata`.

### Escaped HTML and plain-text in XML from `text_format` elements

In order to facilitate the inclusion of markup in records, parallel to
plain-text variants based on values provided from `text_format` elements, two
element process functions have been implemented:

* `_lib4ridora_target_text_format_to_fully_escaped`
* `_lib4ridora_target_text_format_to_plain_text`

When specified on an element (either programmatically, or in the XML Form
Builder in the "More Advanced Controls" -> "Process" section on a given
element), these elements will look for a
`#user_data['lib4ridora_target_text_format']` value ("More Advanced Controls"
-> "User Data", with the key `lib4ridora_target_text_format`) containing a JSON
list of offsets within the form, indicating the location of a `text_format`
element. We allow for the magic value `..` to be used at the beginning of such
a set of offset, to permit targeting `text_format` elements in a relative
fashion... only of particular import for recurring elements in
`tabs`/`tabpanel` (or `fieldpanel`/`fieldpane`) implementations.

Elements with these process functions _must_ occur later in document order than
the elements they target; otherwise, they will not be able to lookup the target
values in order to build their own.

The expected patterns of use have been implemented in
[an example form](xml/text_format_example.xml) (`lib4ridora example text_format
form`). Effectively, this boils down to:
* the `text_format` element targets the XML element made to contain the escaped
  HTML in its  `read` XPath, to show the content when editing (or creating
  using template XML).
* a `hidden` element made to contain the plain-text via the
  `_lib4ridora_target_text_format_to_plain_text` function, with `create`,
  `read` and `update` XPaths (and `delete`, if relevant; no `read`), with
  `#user_data['lib4ridora_target_text_format']` specified to point at the above
  `text_format` element.
* a `hidden` element made to contain the escaped HTML via the
  `_lib4ridora_target_text_format_to_plain_text` function, with `create`,
  `read` and `update` XPaths (and `delete`, if relevant), with
  `#user_data['lib4ridora_target_text_format']` specified to point at the above
  `text_format` element.

## Maintainers/Sponsors

Current maintainers:

* [discoverygarden Inc.](http://github.com/discoverygarden)
