<?php

/**
 * @file
 * Administration callbacks and functions for Eloqua Subscription Center.
 */


/**
 * A token used to simplify the admin experience when defining the path alias.
 */
define('ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN', '[#contact_id]');


/**
 * Administration form for Eloqua Subscription Center
 */
function eloqua_subscription_center_admin($form, &$form_state) {
  $form = array();

  // Configurable path alias where contacts can manage their subscriptions.
  $path = variable_get('eloqua_subscription_center_path', 'subscription-center/%');
  $path = _eloqua_subscription_center_translate_path($path, 'to_admin');
  $form['eloqua_subscription_center_path'] = array(
    '#title' => t('Subscription center path'),
    '#description' => t('A valid path at which contacts will manage their subscriptions; this string must contain the %token token, representing the unique ID of an Eloqua contact.', array(
      '%token' => ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN,
    )),
    '#type' => 'textfield',
    '#default_value' => $path,
    '#required' => TRUE,
    '#element_validate' => array('eloqua_subscription_center_path_validate'),
  );

  // Configurable title for the subscription management page.
  $form['eloqua_subscription_center_title'] = array(
    '#title' => t('Subscription center title'),
    '#description' => t('The page title for the subscription management page.'),
    '#type' => 'textfield',
    '#default_value' => variable_get('eloqua_subscription_center_title', 'Subscriptions'),
    '#element_validate' => array('eloqua_subscription_center_title_validate'),
    '#required' => TRUE,
  );

  // Eloqua form name to which subscription management re-posts should be sent.
  $form['eloqua_subscription_center_form_name'] = array(
    '#title' => t('Eloqua form name'),
    '#description' => t('The elqFormName value used to re-post subscription settings to Eloqua.'),
    '#type' => 'textfield',
    '#default_value' => variable_get('eloqua_subscription_center_form_name', ''),
    '#required' => TRUE,
  );

  // Checkbox for default subscription status.
  $form['eloqua_subscription_center_default_status'] = array(
    '#title' => t('Default subscription status'),
    '#description' => t('Value used when no subscription data yet exists for a given contact and subscription group.'),
    '#type' => 'radios',
    '#options' => array(
      TRUE => t('Subscribed'),
      FALSE => t('Unsubscribed'),
    ),
    '#default_value' => variable_get('eloqua_subscription_center_default_status', TRUE),
  );

  // Whether or not to show a contact's e-mail address on the management page.
  $form['eloqua_subscription_center_hide_email'] = array(
    '#title' => t('Hide e-mail'),
    '#description' => t("Check this to hide the contact's e-mail address on the subscription management page."),
    '#type' => 'checkbox',
    '#default_value' => variable_get('eloqua_subscription_center_hide_email', FALSE),
  );

  // @todo Need a fieldset + way to configure subscription group overrides.
  // @todo Also need a way to configure filtering of subscription groups on the form.

  // Add custom validation and submission handlers; ensure ours come last.
  $form = system_settings_form($form);
  $form['#submit'][] = 'eloqua_subscription_center_admin_submit';

  return $form;
}

/**
 * Validation handler for the Eloqua subscription center path value.
 */
function eloqua_subscription_center_path_validate($element, &$form_state, $form) {
  // Automatically trim unnecessary slashes from the beginning/end.
  form_set_value($element, trim($element['#value'], '/'), $form_state);

  // Before any new values are saved, temporarily store the existing path.
  $form_state['_existing_path'] = variable_get('eloqua_subscription_center_path', 'subscription-center/%');
  $new_path_raw = $form_state['values']['eloqua_subscription_center_path'];
  $new_path = _eloqua_subscription_center_translate_path($new_path_raw, 'to_menu');

  // Ensure that the given path is valid.
  if ($error = _eloqua_subscription_center_invalid_token_placement($new_path_raw)) {
    form_set_error($element['#name'], $error);
  }

  // If the given path is new, ensure it does not already exist.
  if ($form_state['_existing_path'] != $new_path && drupal_valid_path($new_path, TRUE)) {
    form_set_error($element['#name'], t('The path you specified is already in use; please provide a unique, unused path.'));
  }

  // Translate the token to its expected form.
  $translated = _eloqua_subscription_center_translate_path($form_state['values']['eloqua_subscription_center_path'], 'to_menu');
  form_set_value($element, $translated, $form_state);
}

/**
 * Validation handler for the Eloqua subscription center title value.
 */
function eloqua_subscription_center_title_validate($element, &$form_state, $form) {
  // Before any new values are saved, temporarily store the existing title.
  $form_state['_existing_title'] = variable_get('eloqua_subscription_center_title', 'Subscriptions');
}

/**
 * Submit handler for the Eloqua subscription center configuration form.
 */
function eloqua_subscription_center_admin_submit($form, &$form_state) {
  $existing_path = $form_state['_existing_path'];
  $new_path = $form_state['values']['eloqua_subscription_center_path'];
  $existing_title = $form_state['_existing_title'];
  $new_title = $form_state['values']['eloqua_subscription_center_title'];

  // Only rebuild the menu if the path has been modified.
  if ($existing_path != $new_path || $existing_title != $new_title) {
    menu_rebuild();

    // Set an HTTP header for debugging / testing purposes.
    drupal_add_http_header('X-Menu-Rebuilt', TRUE);
  }
}

/**
 * Takes a given path string and returns the appropriate string translation.
 *
 * @param string $path
 *   A Drupal path alias containing either a wildcard suitable for use in a
 *   hook_menu() implementation or an admin-facing token.
 *
 * @param string $direction
 *   One of "to_menu" or "to_admin", indicating the direction of translation. If
 *   "to_menu" is supplied, $path is expected to contain an human-friendly token
 *   otherwise if "to_admin" is supplied, $path is expected to contain a wild-
 *   card suitable for use in hook_menu().
 *
 * @return string
 *   The $path string translated as specified.
 */
function _eloqua_subscription_center_translate_path($path, $direction) {
  switch ($direction) {
    case 'to_menu':
      $path = str_replace(ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN, '%', $path);
      break;

    case 'to_admin':
      $path = str_replace('%', ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN, $path);
      break;
  }

  return $path;
}

/**
 * Returns whether or not a given path's token's placement is invalid.
 *
 * @param string $path
 *   The Drupal path alias containing the contact ID token.
 *
 * @return string
 *   If the token placement in the given path alias is valid, an empty value
 *   will be returned, otherwise an error message will be returned.
 */
function _eloqua_subscription_center_invalid_token_placement($path) {
  $token_pos = strpos($path, ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN);
  $token_len = strlen(ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN);

  // Ensure that the given path contains the contact ID token.
  if ($token_pos === FALSE) {
    return t('The path you specified did not include the %token token.', array(
      '%token' => ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN,
    ));
  }

  // The token must not be the first item in the path part.
  if ($token_pos === 0) {
    return t('The %token token cannot be at the beginning of the path.', array(
      '%token' => ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN,
    ));
  }

  // The character immediately preceding the token must be a slash.
  if ($path[$token_pos - 1] !== '/') {
    return t('The %token token must be a distinct part of the path. Make sure that it is placed between slashes or a slash and the end of the path.', array(
      '%token' => ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN,
    ));
  }

  // The token must be at the path's end, or be immediately followed by a slash.
  if ($token_pos + $token_len != strlen($path) && $path[$token_pos + $token_len] !== '/') {
    return t('The %token token must be a distinct part of the path. Make sure that it is placed between slashes or a slash and the end of the path.', array(
      '%token' => ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN,
    ));
  }

  // The path must not contain the token more than once.
  if (substr_count($path, ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN) > 1) {
    return t('The %token token must not occur more than once in the specified path.', array(
      '%token' => ELOQUA_SUBSCRIPTION_CENTER_CONTACT_ID_TOKEN,
    ));
  }

  return FALSE;
}
