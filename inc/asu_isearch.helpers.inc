<?php

/**
 * Helper function to get the header column widths
 * used on the iSearch directory list view.
 */
function asu_isearch_get_view_column_widths(stdClass $display_options) {
  $columns = array(
    'photo'     => 2,
    'name'      => 4,
    'contact'   => 3,
    'expertise' => 3,
  );

  if (!$display_options->display_photo) {
    $columns['name'] += $columns['photo'];
    $columns['photo'] = 0;
  }

  if (!$display_options->display_expertise) {
    $columns['name'] += ($columns['expertise'] - 1);
    $columns['contact'] += 1;
    $columns['expertise'] = 0;
  }

  if (!$display_options->display_name) {
    $columns['contact'] += $columns['name'];
    $columns['name'] = 0;
  }

  if (!$display_options->display_contact) {
    $columns['name'] += ($columns['contact'] - 1);
    $columns['expertise'] += 1;
  }

  return $columns;
}

/**
 * Helper function to get the default view configuration
 * used on the iSearch directory list view.
 *
 * This will help avoid undefined warnings when new options added
 */
function asu_isearch_get_default_view_config() {
  return array(
    'employee_types'        => array('All'),
    'employee_type_exclude' => 0,
    'tenure_display'        => 0,
    'show_managers'         => 0,
    'show_filters'          => 1,
    'include_subdepts'      => 0,
    'view_display'          => 'isearch_directory:isearch_directory_list',
    'pager_display'         => 'paged',
    'pager_items_per_page'  => 10,
    'pager_offset'          => 0,
    'expertise_areas'       => array(),
    'local_tags'            => array(),
    'display_short_bio'     => TRUE,
    'display_header'        => TRUE,
    'display_photo'         => TRUE,
    'display_name'          => TRUE,
    'display_contact'       => TRUE,
    'display_expertise'     => TRUE,
    'display_building'      => TRUE,
    'display_room'          => TRUE,
    'default_sort_by'       => 'field_isearch_affil_rank_weight_value',
    'filter_title'          => '',
    'show_filter_titles'    => TRUE,
    'show_filter_omni'      => TRUE,
    'show_filter_expertise' => TRUE,
    'show_filter_sort'      => TRUE,
  );
}