#include "CredentialStore.h"

#include <Preferences.h>

namespace {
constexpr const char* NAMESPACE = "wifi";
constexpr const char* KEY_SSID = "ssid";
constexpr const char* KEY_PASS = "pass";
}

bool CredentialStore::load(String& ssid, String& password) {
  Preferences prefs;
  if (!prefs.begin(NAMESPACE, true)) {
    return false;
  }
  ssid = prefs.getString(KEY_SSID, "");
  password = prefs.getString(KEY_PASS, "");
  prefs.end();
  return ssid.length() > 0;
}

void CredentialStore::save(const String& ssid, const String& password) {
  Preferences prefs;
  prefs.begin(NAMESPACE, false);
  prefs.putString(KEY_SSID, ssid);
  prefs.putString(KEY_PASS, password);
  prefs.end();
}

void CredentialStore::clear() {
  Preferences prefs;
  prefs.begin(NAMESPACE, false);
  prefs.clear();
  prefs.end();
}
