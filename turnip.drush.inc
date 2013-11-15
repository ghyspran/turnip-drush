<?php
/**
 * @file
 * Defines Drush commands for building Turnip.
 *
 * Provides Drush with the basic commands necessary to download and start the
 * build process for the Turnip Drupal starter kit.
 *
 * @author Ryan Whitehurst (https://drupal.org/user/871400)
 */

/**
 * Implements hook_drush_help().
 */
function turnip_drush_help($command) {
  switch ($command) {
    case 'drush:turnip-init':
      return dt('Initialize a Turnip project');
    case 'drush:turnip-install':
      return dt('Install or reinstall a Turnip project');
    case 'drush:turnip-rebuild':
      return dt('Rebuild a Turnip project, rerunning Drush make');
  }
}

/**
 * Implements hook_drush_command().
 */
function turnip_drush_command() {
  $items = array();
  $items['turnip-init'] = array(
    'description' => dt('Initialize a Turnip project'),
    'arguments' => array(
      'directory' => dt('The directory to create the Turnip project in'),
    ),
    'options' => array(
      'repo' => array(
        'description' => 'The URL for the git repo.',
        'example_value' => 'https://github.com/opensourcery/turnip.git',
        'value' => 'optional',
      ),
      'branch' => array(
        'description' => 'The branch, tag, or commit hash that you want to check out.',
        'example_value' => '7.x',
        'value' => 'optional',
      ),
    ),
    'examples' => array(
      'drush turnip-init turnip' => 'Initialize a new Turnip project in the directory turnip by cloning the (default) 7.x branch of the (default) https://github.com/opensourcery/turnip.git repo.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );
  $items['turnip-install'] = array(
    'description' => dt('Install or reinstall a Turnip project'),
    'arguments' => array(),
    'options' => array(
      'directory' => array(
        'description' => 'The Turnip root directory. Overrides --root',
        'value' => 'optional',
      ),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
  );
  $items['turnip-rebuild'] = array(
    'description' => dt('Build a Turnip project, running Drush make'),
    'arguments' => array(),
    'options' => array(
      'directory' => array(
        'description' => 'The Turnip root directory',
        'value' => 'optional',
      ),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_MAX,
  );

  return $items;
}

/**
 * Initialize and set up a Turnip install.
 */
function drush_turnip_init() {
  $args = func_get_args();
  if (empty($args)) {
    $directory = 'turnip';
  }
  else {
    $directory = array_shift($args);
  }

  $url = drush_get_option('repo', 'https://github.com/opensourcery/turnip.git');
  $branch = drush_get_option('branch', '7.x');

  if (!drush_confirm("A Turnip project will be initialized in $directory. Is this okay?")) {
    return drush_user_abort(dt('Turnip initialization aborted.'));
  }

  $clone_command = 'git clone -o turnip %s %s';
  if (!drush_shell_exec($clone_command, $url, $directory)) {
    drush_set_error(dt('Could not clone repository @repo into directory @dir',
        array(
          '@repo' => $url,
          '@dir' => $directory,
        )));
  }

  drush_log(dt('Turnip cloned from @repo into directory @dir',
      array(
        '@repo' => $url,
        '@dir' => $directory,
      )), 'ok');
  $cwd = getcwd();
  chdir($directory);

  $checkout_command = 'git checkout %s';
  if (drush_shell_exec($checkout_command, $branch)) {
    drush_log(dt('Checked out @branch', array('@branch' => $branch)), 'ok');
  }
}

function drush_turnip_install() {
  _turnip_run_command('_turnip_install');
}

function drush_turnip_rebuild() {
  _turnip_run_command('_turnip_rebuild');
}

/**
 * Load the supplied command from Turnip.
 */
function _turnip_run_command($command) {
  // First, we see if we passed the Turnip root directory on the commandline. If
  // so, then we use it, otherwise we need to try to figure it out.
  $turnip_root = drush_get_option(array('directory'), '');

  // If we didn't pass in the directory, we check to see if we know the Drupal
  // root. If we do, we assume the parent directory is the Turnip root.
  if (empty($turnip_root)) {
    if (($drupal_root = drush_get_option(array('r', 'root'), drush_locate_root()))) {
      // We want this to work even if the Drupal root is a symlink.
      if (is_link($drupal_root)) {
        $drupal_root = readlink($drupal_root);
      }
      $turnip_root = dirname($drupal_root);
    }
    else {
      $turnip_root = getcwd();
    }
  }

  // Once we have Turnip's root directory, we can load the function definitions.
  if (include($turnip_root . '/includes/drush.inc')) {
    $command($turnip_root);
  }
  else {
    return drush_set_error('DRUSH_ERROR_CODE', dt('Could not find local command file.'));
  }
}
