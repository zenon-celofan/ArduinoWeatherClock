#include "url_utils.h"

String buildLokiUrl(const String &lokiIP, const String &lokiPort) {
  return "http://" + lokiIP + ":" + lokiPort + "/loki/api/v1/push";
}

String buildOpenMeteoUrl(const String &latitude, const String &longitude) {
  return "http://api.open-meteo.com/v1/forecast?latitude=" + latitude
       + "&longitude=" + longitude
       + "&current_weather=true&timezone=auto";
}
