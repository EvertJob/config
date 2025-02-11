#include "esphome.h"
#include "radar.h"

#define MESSAGE_HEAD 0x55
#define NEWLINE 0x0a

class UartReadRadarSensor : public Component, public UARTDevice {
public:
  radar RADAR;
  Sensor *action_status = new Sensor();
  Sensor *movement_status = new Sensor();

  UartReadRadarSensor(UARTComponent *parent) : UARTDevice(parent) {}

  void setup() override {
    // nothing to do here
  }

  int Bodysign_judgment(int ad1, int ad2, int ad3, int ad4, int ad5){
    float s;
    s = RADAR.Bodysign_val(ad1, ad2, ad3, ad4, ad5);

    ESP_LOGV("custom", "Bodysign_val = %f", s );

    if(s < 1.0){
      return 0; // NOBODY;
    }
    else if(s < 2.0){
      return 1; // STATIONARY;
    }
    else if(s >= 2.0 && s < 30.0){
      return 2; // MICRO MOVEMENTS;
    }
    else if(s >= 30.0 && s < 60.0){
      return 3; // SLOW MOVEMENT;
    }
    else if(s >= 60.0){
      return 4; // FAST MOVEMENT;
    }
    return -1;
  }

  int readline(int readch, int *thisMessage, int len)
  {
    static int pos = 0;
    int rpos;

    ESP_LOGV("custom", "this char = %2x; pos = %i", readch, pos );

    // if the read char is not the marker and the pos is zero, skip 
    // i.e. skip until the start marker is found
    if( readch != MESSAGE_HEAD && pos == 0 ){
      return -1;
    }

    // if the read char is a newline and the pos is 1, store the data 
    // I find i'm getting 0x55, then 0x0a, then the good data
    if( readch == NEWLINE && pos == 1 ){
      if (pos < len-1) {
        thisMessage[pos++] = readch;
        thisMessage[pos] = 0;
      }
      return -1;
    }

    // if the read char is the marker and the pos is not zero, return result 
    if( readch == MESSAGE_HEAD && pos != 0 ){
      rpos = pos;
      pos = 0;  // Reset position index ready for next time
      return rpos;
    }

    if (readch != -1) {
      switch (readch) {
//        case '\r': // Return on CR
//          break;
        case NEWLINE: // Return on new-line
          rpos = pos;
          pos = 0;  // Reset position index ready for next time
          return rpos;
        default:
          if (pos < len-1) {
            thisMessage[pos++] = readch;
            thisMessage[pos] = 0;
          }
      }
    }

    // No end of line has been found, so return -1.
    return -1;
  }

  void loop() override {
    const int max_line_length = 14;
    static int thisMessage[max_line_length];
    int activity_result = 0;
    int movement_result = 0;

    while (available()) {
      if(readline(read(), thisMessage, max_line_length) > 0) {

        activity_result = RADAR.Situation_judgment(thisMessage[4], thisMessage[5], thisMessage[6], thisMessage[7], thisMessage[8]);
        ESP_LOGD("custom", "The value of sensor is: %i = RADAR.Situation_judgment( %2x, %2x, %2x, %2x, %2x, )", activity_result, thisMessage[4], thisMessage[5], thisMessage[6], thisMessage[7], thisMessage[8] );
        if( activity_result > 0 ) {
          action_status->publish_state( activity_result );
        }

        movement_result = Bodysign_judgment(thisMessage[5], thisMessage[6], thisMessage[7], thisMessage[8], thisMessage[9]);
        ESP_LOGD("custom", "The value of sensor is: %i = Bodysign_judgment( %2x, %2x, %2x, %2x, %2x, )", movement_result, thisMessage[5], thisMessage[6], thisMessage[7], thisMessage[8], thisMessage[9] );
        if( thisMessage[5] == 6 ){
          movement_status->publish_state( movement_result );
        }
      }
    }
  }
};
