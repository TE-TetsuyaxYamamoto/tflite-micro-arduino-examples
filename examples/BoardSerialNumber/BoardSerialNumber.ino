#include <ArduinoBLE.h>

void setup()
{
  Serial.begin( 9600 );
  while ( !Serial );
}

void loop()
{
  Serial.print( "Device ID 0: " );
  Serial.println( NRF_FICR->DEVICEID[0], HEX );
  Serial.print( "Device ID 1: " );
  Serial.println( NRF_FICR->DEVICEID[1], HEX );

  Serial.print( "Device Address 0: " );
  Serial.println( NRF_FICR->DEVICEADDR[0], HEX );
  Serial.print( "Device Address 1: " );
  Serial.println( NRF_FICR->DEVICEADDR[1], HEX );

  delay ( 5000 );
}
