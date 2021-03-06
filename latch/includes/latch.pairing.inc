<?php

/*
  Latch Drupal 6 plugin - Integrates Latch into the Drupal 6 authentication process.
  Copyright (C) 2013 Eleven Paths

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

//Administration posible routes
function latch_pairing($uid) {
    global $user;
    if ($uid == $user->uid) {
        setPageTitle($user);
        return buildPairingPageContent($uid);
    } else {
        return drupal_access_denied();
    }
}

function setPageTitle($user) {
    $title = isset($user->title) ? $user->title : $user->name;
    drupal_set_title(check_plain($title));
}

function buildPairingPageContent($uid) {
    $query = db_query("SELECT * FROM {latch} WHERE uid=%d", $uid);
    $result = db_fetch_object($query);
    if (!$result) {
        $content = build_pairingform($uid);
    } else {
        $content = "<br><div>Click the button to unpair with your Latch account</div>";
        $content .= build_unpairingform($uid);
    }
    return $content;
}

function pairingform($form_state) {
    $form['token'] = array(
        '#type' => 'textfield',
        '#title' => t('Latch Pairing Token'),
        '#default_value' => '',
        '#suffix' => "</ul>",
        '#size' => 20,
        '#maxlength' => 10,
        '#required' => TRUE,
    );

    $form['pair'] = array(
        '#type' => 'submit',
        '#value' => t('Pair Account'),
    );

    return $form;
}

function build_pairingform($uid) {
    return drupal_get_form('pairingform', $uid);
}

function pairingform_validate($form, &$form_state) {
    //TODO: Add validation for pairingtoken
}

function pairingform_submit($form, $form_state) {
    $uid = $form['#parameters']['2'];
    $token = $form_state['values']['token'];
    pairAccount($token, $uid);
}

function unpairingform($form_state) {
    $form['unpair'] = array(
        '#type' => 'submit',
        '#value' => t('Unpair Account'),
    );
    return $form;
}

function build_unpairingform($uid) {
    return drupal_get_form('unpairingform', $uid);
}

function unpairingform_submit($form, $form_state) {
    $uid = $form['#parameters']['2'];
    unpairAccount($uid);
}

function pairAccount($token, $uid) {
    $query = db_query("SELECT * FROM {latch} WHERE uid=" . $uid);
    $userLatchData = db_fetch_object($query);  
    if (!$userLatchData) { // Check this to avoid pairing a user twice
        performPairing($token, $uid);
    }
}

function performPairing($token, $uid) {
    $appId = variable_get('latch_appid', '');
    $appSecret = variable_get('latch_secret', '');
    if (!empty($appId) && !empty($appSecret)) {
        $api = new Latch($appId, $appSecret);
        $pairResponse = $api->pair($token);
        $responseData = $pairResponse->getData();
        processPairingResponseData($responseData, $uid);
    }
}

function processPairingResponseData($responseData, $uid) {
    if (!empty($responseData)) {
        $accountId = $responseData->{"accountId"};
    }
    if (!empty($accountId)) {
        //OK PAIRING
        db_query("INSERT INTO {latch} (uid,latch_account) VALUES (%d,'%s')", $uid, $accountId);
        drupal_set_message('Pairing success');
    } else {
        //NOK PAIRING
        drupal_set_message('Pairing token not valid', 'error');
    }
}

function unpairAccount($uid) {
    $query = db_query("SELECT * FROM {latch} WHERE uid=%d", $uid);
    $result = db_fetch_object($query);
    if ($result) { // If the user is paired
        performUnpairing($result->latch_account, $uid);
    }
}

function performUnpairing($latchAccountId, $uid) {
    $appId = variable_get('latch_appid', '');
    $appSecret = variable_get('latch_secret', '');
    if (!empty($appId) && !empty($appSecret)) {
        $api = new Latch($appId, $appSecret);
        $api->unpair($latchAccountId);
        db_query("DELETE FROM {latch} WHERE uid=%d", $uid);
    }
}
