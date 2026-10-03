#pragma once

#include <Arduino.h>

/* ─────────────────────────────────────────────
   Minimal JSON number extractors (flat objects only)
   ───────────────────────────────────────────── */

// e.g. jsonInt("{\"joint\":2,\"steps\":100}", "steps") → 100
// Returns 0 if the key is missing.
int jsonInt(const String& body, const char* key);

// Returns NAN if the key is missing or has no numeric value.
float jsonFloat(const String& body, const char* key);
