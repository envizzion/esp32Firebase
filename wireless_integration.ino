//-------------Configuration methods--------------------
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    Serial.println("bluetooth device connected");

      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("bluetooth device disconnected");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic){
    std::string value = pCharacteristic->getValue();

    if(value.length() > 0){
      Serial.print("Value : ");
      Serial.println(value.c_str());
      writeString(wifiAddr, value.c_str());
      inBleMode = false;
      receveivedNewBleData = true;
      ESP.restart();
    }
  }

  void writeString(int add, String data){
    int _size = data.length();
    for(int i=0; i<_size; i++){
      EEPROM.write(add+i, data[i]);
    }
    EEPROM.write(add+_size, '\0');
    EEPROM.commit();

    EEPROM.write(hasCredAddr, 1);
    EEPROM.commit();
  }

  
};


void bleTask(){
  // Create the BLE Device
  inBleMode = true;
  BLEDevice::init("Jesus-light");
   
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
  
}

void wifiTask() {
  String receivedData;
  receivedData = read_String(wifiAddr);
  inBleMode = false;
  
  if(receivedData.length() > 0){
    String wifiName = getValue(receivedData, ',', 0);
    String wifiPassword = getValue(receivedData, ',', 1);

    if(wifiName.length() > 0 && wifiPassword.length() > 0){
      Serial.print("WifiName : ");
      Serial.println(wifiName);

      Serial.print("wifiPassword : ");
      Serial.println(wifiPassword);

      WiFi.begin(wifiName.c_str(), wifiPassword.c_str());
      Serial.print("Connecting to Wifi");


      
    }
  }
}

String read_String(int add){
  char data[100];
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while(k != '\0' && len< 500){
    k = EEPROM.read(add+len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';
  return String(data);
}

String getValue(String data, char separator, int index){
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found <=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
      found++;
      strIndex[0] = strIndex[1]+1;
      strIndex[1] = (i==maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void InitialWiFiCredentialCheckAndUpdate(){
  if(!EEPROM.begin(EEPROM_SIZE)){
    delay(1000);
  }

//  modeIdx = EEPROM.read(modeAddr);
//  Serial.print("modeIdx : ");
//  Serial.println(modeIdx);

//  EEPROM.write(modeAddr, modeIdx !=0 ? 0 : 1);
//  EEPROM.commit();

  hasCredentials = EEPROM.read(hasCredAddr);
  Serial.print("modeIdx : ");
  Serial.println(hasCredentials);

  if(hasCredentials!=1){
    //BLE MODE
    Serial.println("BLE MODE : No Credentials found");
    bleTask();
  }else{
    //WIFI MODE
    Serial.println("WIFI MODE: found credentials");
    wifiTask();
//    bleTask();
 
  }

  
  }



void reconnectWifi(){


if(reconnectCheckTime > millis())return;

Serial.println(".");

reconnectCheckTime = reconnectCheckTimeLimit + millis();
  
if(inBleMode &&   bleTimeout <  millis() ){
 Serial.print("bluetooth timedout Connecting to Wi-Fi");
 wifiTask();
}

else if( !inBleMode || deviceConnected ){
 bleTimeout = millis() + bleTimeoutLimit ; 
}

  
 if (WiFi.status() != WL_CONNECTED && wifiTimeout < millis() && !inBleMode)
  {
   Serial.println();
   Serial.println("wifi dissconnect limit reached switching to bluetooth mode");
   bleTask();
  }
  else if(WiFi.status() == WL_CONNECTED || inBleMode){
         wifiTimeout = millis() + wifiTimeoutLimit ; 
  }

if(WiFi.status() == WL_CONNECTED && !firebaseActive){
        Serial.println("reconnecting firebase");
        connectFirebase();
    }

if(WiFi.status() == WL_CONNECTED ){
        Serial.println("wifi connected");

    }

if(receveivedNewBleData){
   receveivedNewBleData = false;
   wifiTask();  
  }  
}

//-----------------------------------------------------------------------


//-----------------------fireabase methods--------------------------------

void printResult(StreamData &data)
{

  if (data.dataType() == "int")
    Serial.println(data.intData());
  else if (data.dataType() == "float")
    Serial.println(data.floatData(), 5);
  else if (data.dataType() == "double")
    printf("%.9lf\n", data.doubleData());
  else if (data.dataType() == "boolean")
    Serial.println(data.boolData() == 1 ? "true" : "false");
  else if (data.dataType() == "string")
    Serial.println(data.stringData());
  else if (data.dataType() == "json")
  {
    Serial.println();
    FirebaseJson *json = data.jsonObjectPtr();
    //Print all object data
    Serial.println("Pretty printed JSON data:");
    String jsonStr;
    json->toString(jsonStr, true);
    Serial.println(jsonStr);
    Serial.println();
    Serial.println("Iterate JSON data:");
    Serial.println();
    size_t len = json->iteratorBegin();
    String key, value = "";
    int type = 0;
    for (size_t i = 0; i < len; i++)
    {
      json->iteratorGet(i, type, key, value);
      Serial.print(i);
      Serial.print(", ");
      Serial.print("Type: ");
      Serial.print(type == FirebaseJson::JSON_OBJECT ? "object" : "array");
      if (type == FirebaseJson::JSON_OBJECT)
      {
        Serial.print(", Key: ");
        Serial.print(key);
      }
      Serial.print(", Value: ");
      Serial.println(value);
    }
    json->iteratorEnd();
  }
  else if (data.dataType() == "array")
  {
    Serial.println();
    //get array data from FirebaseData using FirebaseJsonArray object
    FirebaseJsonArray *arr = data.jsonArrayPtr();
    //Print all array values
    Serial.println("Pretty printed Array:");
    String arrStr;
    arr->toString(arrStr, true);
    Serial.println(arrStr);
    Serial.println();
    Serial.println("Iterate array values:");
    Serial.println();

    for (size_t i = 0; i < arr->size(); i++)
    {
      Serial.print(i);
      Serial.print(", Value: ");

      FirebaseJsonData *jsonData = data.jsonDataPtr();
      //Get the result data from FirebaseJsonArray object
      arr->get(*jsonData, i);
      if (jsonData->typeNum == FirebaseJson::JSON_BOOL)
        Serial.println(jsonData->boolValue ? "true" : "false");
      else if (jsonData->typeNum == FirebaseJson::JSON_INT)
        Serial.println(jsonData->intValue);
      else if (jsonData->typeNum == FirebaseJson::JSON_DOUBLE)
        printf("%.9lf\n", jsonData->doubleValue);
      else if (jsonData->typeNum == FirebaseJson::JSON_STRING ||
               jsonData->typeNum == FirebaseJson::JSON_NULL ||
               jsonData->typeNum == FirebaseJson::JSON_OBJECT ||
               jsonData->typeNum == FirebaseJson::JSON_ARRAY)
        Serial.println(jsonData->stringValue);
    }
  }
}


void streamCallback(StreamData data)
{

  Serial.println("Stream Data1 available...");
  Serial.println("STREAM PATH: " + data.streamPath());
  Serial.println("EVENT PATH: " + data.dataPath());
  Serial.println("DATA TYPE: " + data.dataType());
  Serial.println("EVENT TYPE: " + data.eventType());
  Serial.print("VALUE: ");
  printResult(data);
  uint8_t scen = data.intData();
 
  ChangeLightScene(scen);
  
  Serial.println();
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
  {
    Serial.println();
    Serial.println("Stream timeout, resume streaming...");
    Serial.println();
//    firebaseActive = false;
  }
}

void initializeFirebase(){
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  Firebase.setReadTimeout(firebaseData1, 1000 * 2);
  
  }

 void connectFirebase(){
   initializeFirebase();
    if (!Firebase.beginStream(firebaseData1, path))
  {
    Serial.println("------------------------------------");
    Serial.println("Can't begin stream connection...");
    Serial.println("REASON: " + firebaseData1.errorReason());
    Serial.println("------------------------------------");
    Serial.println();
    firebaseActive = false;
  }
  else{
    firebaseActive = true;
    Firebase.setStreamCallback(firebaseData1, streamCallback, streamTimeoutCallback);
  }  
    }
  
//-----------------------------------------------------------------------
