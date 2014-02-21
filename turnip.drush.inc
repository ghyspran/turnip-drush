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
      'drush turnip-init turnip' => 'Initialize a new Turnip project in the directory turnip by cloning the (default) 7.x branch of the (default) https://github.com/opensourcery/turnip.git repo.',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH,
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

  _turnip_clone_turnip(array(
      'url' => $url,
      'directory' => $directory,
      'branch' => $branch,
    ));

  _turnip_make_init();
}

/**
 * Function to clone a Turnip repo from source.
 */
function _turnip_clone_turnip($args) {
  $clone_command = 'git clone -o turnip %s %s';

  $url = $args['url'];
  $directory = $args['directory'];
  $branch = $args['branch'];

  drush_log(dt('Cloning @repo into @dir', array(
        '@repo' => $url,
        '@dir' => $directory
      )));
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
  drush_op('chdir', $directory);

  $checkout_command = 'git checkout %s';
  if (drush_shell_exec($checkout_command, $branch)) {
    drush_log(dt('Checked out @branch', array('@branch' => $branch)), 'ok');
  }
}

/**
 * Setup up initial project values.
 */
function _turnip_make_init() {
  $machine_name = drush_prompt('Enter the machine name for the profile', NULL, FALSE);
  $theme_name = drush_prompt('Enter the machine name for the theme', NULL, FALSE);
  $proj_name = drush_prompt('Enter the project name', NULL, FALSE);
  $proj_desc = drush_prompt('Enter the project description', NULL, FALSE);

  drush_op('file_put_contents', 'bin/.config', "PROFILE=\"$machine_name\"", FILE_APPEND);
  drush_op('file_put_contents', 'bin/.config', "\nTHEME=\"$theme_name\"", FILE_APPEND);
  drush_op('file_put_contents', 'bin/.config', "\nPROJECTNAME=\"$proj_name\"", FILE_APPEND);
  drush_op('file_put_contents', 'bin/.config', "\nDESCRIPTION=\"$proj_desc\"", FILE_APPEND);
  drush_op('file_put_contents', 'bin/.config', "\nPRODUCTIONURL=\"http://\"", FILE_APPEND);

  // Move the project to the proper machine name.
  drush_op('rename', 'os_project', $machine_name);
  opendir($machine_name);
  while ($file = readdir()) {
    if (preg_match('/^os_project\..*/', $file)) {
      drush_op('rename',
        implode('/', array(
            $machine_name,
            $file
          )
        ),
        implode('/', array(
            $machine_name,
            preg_replace('/os_project/', $machine_name, $file)
          )
        )
      );
    }
  }
  closedir();

  // Remove bin/.gitignore, which only contains a reference to .config.
  drush_op('unlink', 'bin/.gitignore');

  // Process placeholders in project files.
  drush_shell_exec(sprintf('sed -e "s/os_project/%1$s/" -i.orig %1$s/%1$s.*', $machine_name));
  drush_shell_exec(sprintf('sed -e "s/PROJECTNAME/%1$s/" -i.orig %2$s/%2$s.*', $proj_name, $machine_name));
  drush_shell_exec(sprintf('sed -e "s/DESCRIPTION/%1$s/" -i.orig %2$s/%2$s.*', $proj_desc, $machine_name));
  drush_shell_exec(sprintf('sed -e "s/THEME/%1$s/" -i.orig %2$s/%2$s.info', $theme_name, $machine_name));
  drush_shell_exec(sprintf('sed -e "s/THEME/%1$s/" -i.orig %2$s/theme.make', $theme_name, $machine_name));
  opendir($machine_name);
  while ($file = readdir()) {
    if (preg_match('/\.orig$/', $file)) {
      drush_op('unlink', implode('/', array($machine_name, $file)));
    }
  }
  closedir();

  // Set install profile in settings.php.
  drush_op('file_put_contents', 'shared/settings.php', sprintf("\n\$conf['install_profile'] = '%s';\n", $machine_name), FILE_APPEND);

  // Move local settings file into place.
  if (!is_file('shared/settings.local.php')) {
    drush_op('copy', 'shared/settings.local.php.example', 'shared/settings.local.php');
  }

  // Move Turnip README.md to README.dist.md
  drush_op('rename', 'README.md', 'README.dist.md');
  drush_op('file_put_contents', 'README.md', sprintf("##%s", $proj_name . "\n\n" . $proj_desc));

  // Allow skipping initializing the theme.
  // @TODO Update this to print theme system requirements before prompting.
  if (drush_confirm('Do you want to initialize the theme now?')) {
    _turnip_init_theme();
  }
}


/**
 * Initialize a new theme.
 */
function _turnip_init_theme() {
  drush_op_system('bin/init-theme');
}
