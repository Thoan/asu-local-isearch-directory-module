<?php

/**
 * There are four essential components to set up in your constructor:
 *  $this->source - An instance of a class derived from MigrateSource, this
 *    will feed data to the migration.
 *  $this->destination - An instance of a class derived from MigrateDestination,
 *    this will receive data that originated from the source and has been mapped
 *    by the Migration class, and create Drupal objects.
 *  $this->map - An instance of a class derived from MigrateMap, this will keep
 *    track of which source items have been imported and what destination objects
 *    they map to.
 *  Mappings - Use $this->addFieldMapping to tell the Migration class what source
 *    fields correspond to what destination fields, and additional information
 *    associated with the mappings.
 */
class MigrateIsearchProfileMigration extends Migration {

  private static $expertiseAreasCache = array();
  private static $expertiseVocabulary = null;

  private static $titleRanksCache = array();
  private static $titleRanksVocabulary = null;

  private $profilePhotoDirectory = null;

  public $forceUpdate = FALSE;

  public static $currentFeedUrl = null;

  public function __construct($arguments) {
    parent::__construct($arguments);

    $this->description = t('Migrates iSearch profiles by department.');

    // Create a map object for tracking the relationships between source rows
    // and their resulting Drupal objects. Usually, you'll use the MigrateSQLMap
    // class, which uses database tables for tracking. Pass the machine name
    // of this migration to use in generating map and message tables.
    // And, pass schema definitions for the primary keys of the source and
    // destination - we need to be explicit for our source, but the destination
    // class knows its schema already.

    // Define the source schema
    $this->map = new MigrateSQLMap($this->machineName, array(
        'eid' => array(
          'type' => 'varchar',
          'length' => 255,
          'not null' => TRUE,
          'description' => 'Employee ID',
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );

    // query and store the title ranks vocabulary id
    if (is_null(self::$titleRanksVocabulary)) {
      self::$titleRanksVocabulary = taxonomy_vocabulary_machine_name_load('isearch_ranks');
      $ranks_term_tree = taxonomy_get_tree(self::$titleRanksVocabulary->vid, 0, NULL, TRUE);

      foreach ($ranks_term_tree as $term) {
        $termKey = $term->field_isearch_rank_tid[LANGUAGE_NONE][0]['value'];
        self::$titleRanksCache[$termKey] = $term;
      }
    }

    // query and store the expertise areas vocabulary id
    if (is_null(self::$expertiseVocabulary)) {
      self::$expertiseVocabulary = taxonomy_vocabulary_machine_name_load('isearch_expertise');
      $expertise_term_tree = taxonomy_get_tree(self::$expertiseVocabulary->vid, 0, NULL, TRUE);

      foreach ($expertise_term_tree as $term) {
        $termKey = $term->field_isearch_expertise_tid[LANGUAGE_NONE][0]['value'];
        self::$expertiseAreasCache[$termKey] = $term;
      }
    }

    // Create a MigrateSource object, which manages retrieving the input data.
    global $base_url;
    $source_urls = $base_url . url(drupal_get_path('module', 'asu_isearch')) . '/inc/asu_isearch.migrate.json';

    $config_source_urls = asu_isearch_get_migrate_source_urls(TRUE);
    if (count($config_source_urls)) {
      $source_urls = $config_source_urls;
    }

    $this->setMigrateSource($source_urls);
    $this->destination = new MigrateDestinationNode('asu_isearch_profile');

    // Setup profile photo directory
    $this->setupProfilePhotoDirectory();

    $this->addFieldMapping('status'                                  , 'status');
    $this->addFieldMapping('field_isearch_displayname'               , 'displayName');
    $this->addFieldMapping('field_isearch_firstname'                 , 'firstName');
    $this->addFieldMapping('field_isearch_lastname'                  , 'lastName');
    $this->addFieldMapping('field_isearch_employment_title'          , 'primaryTitle');
    $this->addFieldMapping('field_isearch_eid'                       , 'eid');
    $this->addFieldMapping('field_isearch_asurite_id'                , 'asuriteId');
    $this->addFieldMapping('field_isearch_email'                     , 'emailAddress');
    $this->addFieldMapping('field_isearch_phone'                     , 'phone');
    $this->addFieldMapping('field_isearch_fax'                       , 'fax');
    $this->addFieldMapping('field_isearch_campus_building'           , 'addressLine1');
    $this->addFieldMapping('field_isearch_campus_room'               , 'addressLine2');
    $this->addFieldMapping('field_isearch_primary_campus'            , 'primaryJobCampus');
    $this->addFieldMapping('field_isearch_photo_preference'          , 'photoPreference');
    $this->addFieldMapping('field_isearch_photo_permission'          , 'photoPermission');
    $this->addFieldMapping('field_isearch_social_twitter'            , 'twitterLink');
    $this->addFieldMapping('field_isearch_social_facebook'           , 'facebook');
    $this->addFieldMapping('field_isearch_social_linkedin'           , 'linkedin');
    $this->addFieldMapping('field_isearch_social_googleplus'         , 'googlePlus');
    $this->addFieldMapping('field_isearch_social_personalweb'        , 'website');
    $this->addFieldMapping('field_isearch_short_bio'                 , 'shortBio');
    $this->addFieldMapping('field_isearch_bio'                       , 'bio');
    $this->addFieldMapping('field_isearch_education'                 , 'education');
    $this->addFieldMapping('field_isearch_cv_url'                    , 'cvUrl');
    $this->addFieldMapping('field_isearch_rsrch_website'             , 'researchWebsite');
    $this->addFieldMapping('field_isearch_rsrch_interests'           , 'researchInterests');
    $this->addFieldMapping('field_isearch_rsrch_group'               , 'researchGroup');
    $this->addFieldMapping('field_isearch_rsrch_activity'            , 'researchActivity');
    $this->addFieldMapping('field_isearch_rsrch_publications'        , 'publications');
    $this->addFieldMapping('field_isearch_tch_website'               , 'teachingWebsite');
    $this->addFieldMapping('field_isearch_tch_courses'               , 'courses');
    $this->addFieldMapping('field_isearch_tch_presentations'         , 'presentations');
    $this->addFieldMapping('field_isearch_honors_awards'             , 'honorsAwards');
    $this->addFieldMapping('field_isearch_editorships'               , 'editorships');
    $this->addFieldMapping('field_isearch_prof_associations'         , 'professionalAssociations');
    $this->addFieldMapping('field_isearch_graduate_faculties'        , 'graduateFaculties');
    $this->addFieldMapping('field_isearch_work_history'              , 'workHistory');
    $this->addFieldMapping('field_isearch_service'                   , 'service');
    $this->addFieldMapping('field_isearch_bio:format'                , 'htmlFormat');
    $this->addFieldMapping('field_isearch_editorships:format'        , 'htmlFormat');
    $this->addFieldMapping('field_isearch_education:format'          , 'htmlFormat');
    $this->addFieldMapping('field_isearch_honors_awards:format'      , 'htmlFormat');
    $this->addFieldMapping('field_isearch_rsrch_activity:format'     , 'htmlFormat');
    $this->addFieldMapping('field_isearch_rsrch_group:format'        , 'htmlFormat');
    $this->addFieldMapping('field_isearch_rsrch_interests:format'    , 'htmlFormat');
    $this->addFieldMapping('field_isearch_rsrch_publications:format' , 'htmlFormat');
    $this->addFieldMapping('field_isearch_service:format'            , 'htmlFormat');
    $this->addFieldMapping('field_isearch_short_bio:format'          , 'htmlFormat');
    $this->addFieldMapping('field_isearch_tch_presentations:format'  , 'htmlFormat');
    $this->addFieldMapping('field_isearch_tch_courses:format'        , 'coursesFormat');
    $this->addFieldMapping('field_isearch_hash'                      , 'migrateHash');
  }

  public function setMigrateSource($urls) {
    $this->source = new MigrateSourceJSON($urls, 'eid', $this->getSourceFields(), array('reader_class' => 'IsearchProfileJSONReader'));
  }

  public function processImport(array $options = array()) {
    $this->forceUpdate = (isset($options['force']) && $options['force']);
    parent::processImport($options);
  }

  public function getSourceFields() {
    $source_fields = array(
      'eid'                      => 'Profile identifier',
      'displayName'              => 'The profile display name',
      'firstName'                => 'First name',
      'lastName'                 => 'Last name',
      'emailAddress'             => 'Email address',
      'asuriteId'                => 'ASURITE ID',
      'primaryTitle'             => 'Primary title',
      'phone'                    => 'Phone',
      'fax'                      => 'Fax',
      'addressLine1'             => 'Building',
      'addressLine2'             => 'Room',
      'primaryJobCampus'         => 'Primary job campus',
      'shortBio'                 => 'Short bio',
      'bio'                      => 'Biography',
      'education'                => 'Education',
      'cvUrl'                    => 'CV url',
      'website'                  => 'Personal website',
      'twitter'                  => 'Profile twitter link',
      'facebook'                 => 'Profile facebook link',
      'linkedin'                 => 'Profile linkedin link',
      'googlePlus'               => 'Profile googleplus link',
      'photoPreference'          => 'Photo preference',
      'photoPermission'          => 'Photo permission',
      'researchInterests'        => 'Research interests',
      'researchGroup'            => 'Research group',
      'researchActivity'         => 'Research activity',
      'publications'             => 'Publications',
      'researchWebsite'          => 'Research website',
      'courses'                  => 'Courses',
      'presentations'            => 'Presentations',
      'teachingWebsite'          => 'Teaching website',
      'honorsAwards'             => 'Honors awards',
      'editorships'              => 'Editorships',
      'professionalAssociations' => 'Professional associations',
      'graduateFaculties'        => 'Graduate Faculties',
      'workHistory'              => 'Work history',
      'service'                  => 'Service',
      'primaryAffiliation'       => 'PrimaryAffiliation',
      'departmentIds'            => 'DepartmentIds',
      'departments'              => 'Departments',
      'vpAreas'                  => 'VP areas',
      'campuses'                 => 'Campuses',
      'mailCodes'                => 'Mail codes',
      'employeeTypes'            => 'Employee types',
      'personalWebsite'          => 'Personal website',
      'htmlFormat'               => 'Placeholder source field used for setting HTML format on text fields',
      'migrateHash'              => 'Migrate hash used to determine if the entity should be updated or skipped on import',
    );

    return $source_fields;
  }

  private function isDevMode() {
    return variable_get('asu_isearch_devmode', FALSE);
  }

  private function setupProfilePhotoDirectory() {
    // Setup other variables we need
    $this->profilePhotoDirectory = file_build_uri('asu_isearch_photos');

    if (!file_exists($this->profilePhotoDirectory)) {
      drupal_mkdir($this->profilePhotoDirectory);
    }
  }

  // This method gives us an opportunity to modify the raw source data
  // to create additional fields that will get mapped possibly
  public function prepareRow($row) {

    // placeholder to set isearch_html as body format
    $row->htmlFormat = 'isearch_html';

    // keep courses field as full_html
    $row->coursesFormat = 'full_html';

    if (isset($row->phone)) {
      // format phone number
      $row->phone = asu_isearch_format_phone($row->phone);
    }

    // We have enough information to generate a hash from
    $row_hash = (array) $row;
    sort($row_hash);
    $row_hash = md5(serialize($row_hash));

    $row->migrateHash = $row_hash;

    // lets see if we need to update this record at all
    $query = new EntityFieldQuery();
    $result = $query->entityCondition('entity_type', 'node')
      ->entityCondition('bundle', 'asu_isearch_profile')
      ->fieldCondition('field_isearch_eid', 'value', $row->eid)
      ->fieldCondition('field_isearch_hash', 'value', $row_hash)
      ->execute();

    if ($this->forceUpdate === FALSE && isset($result['node'])) {
      // Fetch and store profile photo
      $entity = node_load(current($result['node'])->nid);
      $this->fetchProfilePhoto($entity, $row);
      return FALSE;
    }

    // prepare affiliations array
    $row->affiliations_info = array();

    $additional_titles = array();

    if (isset($row->deptids)) {
      foreach ($row->deptids as $index => $department_id) {
        $affiliation = new stdclass();
        $affiliation->id              = $department_id;
        $affiliation->deptId          = $department_id;
        $affiliation->deptName        = $row->departments[$index];
        $affiliation->title           = $row->titles[$index];
        $affiliation->vpArea          = $row->vpAreas[$index];
        $affiliation->campus          = $row->jobCampuses[$index];
        $affiliation->mailCode        = $row->mailCodes[$index];
        $affiliation->employeeType    = $row->employeeTypes[$index];
        $affiliation->employeeTypeKey = preg_replace('/[^a-zA-Z]/', '_', trim($row->employeeTypes[$index]));
        $affiliation->isManager       = $row->managers[$index];
        $affiliation->isRankTenure    = $row->departmentTitleRanksTenure[$index];
        $affiliation->ranksTid        = $row->departmentTitleRanksTids[$index];
        $affiliation->ranksWeight     = $row->departmentTitleRanksWeight[$index];
        $affiliation->isPrimary       = ($row->deptids[$index] == $row->primaryDeptid);

        $row->affiliations_info[$affiliation->id] = $affiliation;

        // store additional titles into the additional titles array in case shortBio field is empty
        // we don't want to include it if it's the primary title
        if (!$affiliation->isPrimary && $affiliation->title != $row->primaryTitle) {
          $additional_titles[] = $affiliation->title;
        }
      }
    }

    preg_match('/\/([0-9]+)?$/', self::$currentFeedUrl, $matches);
    $row->localPrimaryAffiliation = $matches[1];

    // prepare expertise areas array
    $row->expertise = array();

    if (isset($row->expertiseAreaTids)) {
      foreach ($row->expertiseAreaTids as $index => $expertise_key) {
        $expertiseArea = new stdclass();
        $expertiseArea->id = $expertise_key;
        $expertiseArea->name = $row->expertiseAreas[$index];

        $row->expertise[$expertiseArea->id] = $expertiseArea;
      }
    }

    // prepare rank title array
    $row->titleranks = array();

    if (isset($row->departmentTitleRanksTids)) {
      foreach ($row->departmentTitleRanksTids as $index => $rank_key) {
        $titleRank = new stdclass();
        $titleRank->name   = $row->departmentTitleRanks[$index];
        $titleRank->id     = $rank_key;
        $titleRank->tid    = $row->departmentTitleRanksTids[$index];
        $titleRank->weight = $row->departmentTitleRanksWeight[$index];
        $titleRank->tenure = $row->departmentTitleRanksTenure[$index];

        $row->titleranks[$titleRank->id] = $titleRank;
      }
    }
  }

  // This will get called just before the destination entity is saved
  public function prepare($entity, stdClass $row) {
    // Some required adjustments to nodes that already exist
    if (isset($entity->is_new) && $entity->is_new === FALSE) {
      $node = node_load($entity->nid);
      $entity->status = $node->status;
      $entity->field_isearch_affiliations = $node->field_isearch_affiliations;
      $entity->field_isearch_expertise_areas = $node->field_isearch_expertise_areas;
    }

    $entity->title = $row->displayName;

    // Setup expertise taxonomy terms
    $this->setupExpertiseAreas($entity, $row);

    // Setup title rank terms
    $this->setupTitleRanks($entity, $row);
  }

  // This will get called after the destination entity has been created,
  // giving access to the entity id to perform other operations
  public function complete($entity, stdClass $row) {

    // Setup affiliations
    $this->setupAffiliations($entity, $row);

    // Fetch and store profile photo
    $this->fetchProfilePhoto($entity, $row);
  }

  private function fetchProfilePhoto($entity, stdClass $row) {

    if ($row->photoPreference != 'none') {
      $eid = $row->eid;

      $destination = $this->profilePhotoDirectory . "/{$eid}.jpg";
      $photo_url = ASU_ISEARCH_DIRECTORY_PHOTO_ENDPOINT . $eid;

      $data = array(
        'destination' => $destination,
        'photo_url' => $photo_url,
        'entity' => $entity->nid,
      );

      $queue = DrupalQueue::get('isearch_fetch_profile_photo');
      $queue->createItem($data);
    }

  }

  private function setupExpertiseAreas($entity, stdClass $row) {

    // keeps track of existing expertise values
    $existing_expertise = array();

    // Loop through the expertise areas we received from $row and add if necessary
    foreach ($row->expertise as $expertise_key => $item) {
      // if this value already exists for this entity, continue (no need to create/update)
      if (isset($existing_expertise[$expertise_key])) {
        continue;
      }

      // this wasn't an existing expertise, but we already are aware of the term
      // add it to existing_expertise and continue
      if (isset(self::$expertiseAreasCache[$expertise_key])) {
        $existing_expertise[$expertise_key] = self::$expertiseAreasCache[$expertise_key]->tid;
        if ($item->name != self::$expertiseAreasCache[$expertise_key]->name) {
          self::$expertiseAreasCache[$expertise_key]->name = $item->name;
          taxonomy_term_save(self::$expertiseAreasCache[$expertise_key]);
        }
        continue;
      }

      $term = new stdclass();
      $term->vid  = self::$expertiseVocabulary->vid;
      $term->field_isearch_expertise_tid[LANGUAGE_NONE][0] = array('value' => $expertise_key);

      // We set the name property here in case the label was changed on feed
      $term->name = $item->name;
      taxonomy_term_save($term);

      // add to list of expertise we will save on entity
      $existing_expertise[$expertise_key] = $term->tid;

      // store these values in our cache
      self::$expertiseAreasCache[$expertise_key] = $term->tid;
    }

    $expertise_areas = array();
    foreach ($existing_expertise as $term_id) {
      $expertise_areas[] = array('tid' => $term_id);
    }

    $entity->field_isearch_expertise_areas[LANGUAGE_NONE] = $expertise_areas;
  }

  private function setupTitleRanks($entity, stdClass $row) {

    // keeps track of existing expertise values
    $existing_ranks = array();

    // Loop through the expertise areas we received from $row and add if necessary
    foreach ($row->titleranks as $rank_key => $item) {
      // if this value already exists for this entity, continue (no need to create/update)
      if (isset($existing_ranks[$rank_key])) {
        continue;
      }

      // this wasn't an existing expertise, but we already are aware of the term
      // add it to existing_ranks and continue
      if (isset(self::$titleRanksCache[$rank_key])) {
        $existing_ranks[$rank_key] = self::$titleRanksCache[$rank_key]->tid;

        // do we need to update the name of this existing term?
        if ($item->name != self::$titleRanksCache[$rank_key]->name) {
          self::$titleRanksCache[$rank_key]->name = $item->name;
          taxonomy_term_save(self::$titleRanksCache[$rank_key]);
        }
        continue;
      }

      // ensure we don't have a blank term name
      if (trim($item->name) == '') {
        continue;
      }

      $term = new stdclass();
      $term->vid  = self::$titleRanksVocabulary->vid;
      $term->field_isearch_rank_tid[LANGUAGE_NONE][0]['value']    = intval($item->tid);
      $term->field_isearch_rank_weight[LANGUAGE_NONE][0]['value'] = intval($item->weight);
      $term->field_isearch_rank_tenure[LANGUAGE_NONE][0]['value'] = intval($item->tenure) ? 1 : 0;

      // We set the name property here in case the label was changed on feed
      $term->name = $item->name;
      taxonomy_term_save($term);

      // add to list of ranks we will save on entity
      $existing_ranks[$rank_key] = $term->tid;

      // store these values in our cache
      self::$titleRanksCache[$rank_key] = $term->tid;
    }

    $row->existing_ranks = $existing_ranks;
  }

  private function setupAffiliations($entity, stdClass $row) {
    $update_affiliations = array();

    // determine if we need to update or delete existing affiliations
    if (!empty($entity->field_isearch_affiliations)) {
      // loop through existing affiliations and see if we need to delete or update them
      foreach ($entity->field_isearch_affiliations[LANGUAGE_NONE] as $index => $item) {
        $affiliation = field_collection_item_load($item['value']);

        $affiliation_key = $affiliation->field_isearch_affil_dept_id[LANGUAGE_NONE][0]['value'];

        if (!isset($row->affiliations_info[$affiliation_key])) {
          entity_delete('field_collection_item', $affiliation->item_id);
        }
        else {
          $update_affiliations[$affiliation_key] = $affiliation;
        }
      }
    }

    // create/update new affiliations
    foreach ($row->affiliations_info as $affiliation_key => $item) {
      if (isset($update_affiliations[$affiliation_key])) {
        // update an existing affiliation
        $affiliation = $update_affiliations[$affiliation_key];
      }
      else {
        // this affiliation does not exist, create it
        $affiliation = entity_create('field_collection_item', array('field_name' => 'field_isearch_affiliations'));
        $affiliation->setHostEntity('node', $entity);
      }

      // store the affiliation title
      $affiliation_title = trim($item->title);

      // if this is a tenure rank affiliation, and the affiliation title
      // was left blank, set it to the rank title
      if ($affiliation_title == '' && $item->ranksTid) {
        $rank = taxonomy_term_load($row->existing_ranks[$item->ranksTid]);
        $affiliation_title = $rank->name;
      }

      $affiliation->field_isearch_affil_dept_id[LANGUAGE_NONE][0]['value']     = $item->deptId;
      $affiliation->field_isearch_affil_dept_name[LANGUAGE_NONE][0]['value']   = $item->deptName;
      $affiliation->field_isearch_affil_title[LANGUAGE_NONE][0]['value']       = $affiliation_title;
      $affiliation->field_isearch_affil_campus[LANGUAGE_NONE][0]['value']      = $item->campus;
      $affiliation->field_isearch_affil_empl_type[LANGUAGE_NONE][0]['value']   = $item->employeeType;
      $affiliation->field_isearch_affil_empl_key[LANGUAGE_NONE][0]['value']    = $item->employeeTypeKey;
      $affiliation->field_isearch_affil_mailcode[LANGUAGE_NONE][0]['value']    = $item->mailCode;
      $affiliation->field_isearch_affil_manager[LANGUAGE_NONE][0]['value']     = ($item->isManager) ? 1 : 0;
      $affiliation->field_isearch_affil_primary[LANGUAGE_NONE][0]['value']     = ($item->isPrimary) ? 1 : 0;
      $affiliation->field_isearch_affil_rank_tenure[LANGUAGE_NONE][0]['value'] = ($item->isRankTenure) ? 1 : 0;

      if ($item->ranksTid && isset($row->existing_ranks[$item->ranksTid])) {
        $affiliation->field_isearch_affil_rank[LANGUAGE_NONE][0]['tid']          = $row->existing_ranks[$item->ranksTid];
      }

      $affiliation->save();

      if ($row->localPrimaryAffiliation == $item->deptId) {
        $entity->field_isearch_primary_aff[LANGUAGE_NONE][0]['target_id'] = $affiliation->item_id;
        node_save($entity);
      }
    }
  }
}

class IsearchProfileJSONReader extends MigrateJSONReader {
  /**
   * Implements MigrateJSONReader::next().
   */
  public function next() {
    // Do the default fetch.
    parent::next();

    // If we need to tweak
  }

  public function current() {
    $row = parent::current();
    MigrateIsearchProfileMigration::$currentFeedUrl = $this->url;

    if ($row->eid == 'debugging') {
      $row->rand = mktime();
    }

    return $row;
  }
}
