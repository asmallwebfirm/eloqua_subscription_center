<?php

/**
 * @file
 * Mock Elomentary client, used for testing the subscription center.
 */

class MockElomentaryClient {

  /**
   * Tracks the current active API.
   * @var string
   */
  protected $active_api = '';

  /**
   * Stores contact data for behaviour tests.
   * @var array
   */
  protected $contacts = array(
    // Valid contact.
    '1' => array(
      'stored' => 'data',
      'emailAddress' => 'foobar@example.com',
    ),
  );

  /**
   * Mocks the Elomentary "api" facade.
   */
  public function api($type) {
    $this->active_api = $type;
    return $this;
  }

  /**
   * Mocks the Elomentary contact subscription client.
   */
  public function subscriptions($id) {
    $this->active_api = 'subscriptions';
    return $this;
  }

  /**
   * Mocks all Elomentary "show" method calls.
   */
  public function show($id, $depth = null, $extensions = null) {
    switch ($this->active_api) {
      case 'contact':
      case 'contacts':
        return isset($this->contacts[$id]) ? $this->contacts[$id] : array();
        break;
    }
  }

  /**
   * Mocks all Elomentary "search" method calls.
   */
  public function search($search, $options = array()) {
    switch ($this->active_api) {
      case 'subscriptions':
        return array('elements' => array());
        break;
    }
  }
}
