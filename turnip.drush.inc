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
      'drush ti turnip' => 'Initialize a new Turnip project in the directory turnip by cloning the (default) 7.x branch of the (default) https://github.com/opensourcery/turnip.git repo.',
    ),
    'aliases' => array('ti'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
  );

  return $items;
}

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

  drush_log(dt('Turnip clone from @repo into directory @dir',
      array(
        '@repo' => $url,
        '@dir' => $directory,
      )));
  $cwd = getcwd();
  chdir($directory);

  $checkout_command = 'git checkout %s';
  if (drush_shell_exec($checkout_command, $branch)) {
    drush_log(dt('Checked out @branch', array('@branch', $branch)));
  }
}
