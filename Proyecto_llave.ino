
#include <Keypad.h> // Lib para Keypad 
#include <LiquidCrystal.h> // Lib para LCD 16x2
#include <SPI.h> // Lib adicional para comunicación
#include <MFRC522.h> // lib para lector RC522
/****************************** VARIABLES GLOBALES ************************************/
#define RST_PIN         47           // Configurable, see typical pin layout above
#define SS_PIN          53          // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
MFRC522::MIFARE_Key key;
int llavepresente;
int Tcredito;
LiquidCrystal lcd(32,31,25, 24, 23, 22);
int credito=0;
int i;
char creditoINPUT;
/***************************************************************************************/
/*TABLA DE PRODUCTOS MAQUINA*/
// [0]-> Codigo de producto [1]-> Disponibilidad (0-1) [2]-> Precio(€)
/* Futura implementacion, sensores para disponibilidad de producto, por el momento siempre en 1*/
int producto1[]={10,1,3};
int producto2[]={20,1,2};
int producto3[]={30,1,1};
int numerodeproductos=3;
/*  ******************************* */
const byte ROWS = 4; 
const byte COLS = 4; 
byte rowPins[ROWS] = {9, 8, 7, 6}; 
byte colPins[COLS] = {5, 4, 3, 2}; 
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 
String sumatorio  ;
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte smiley[8] = {
  B00000,
  B10001,
  B00000,
  B00000,
  B10001,
  B01110,
  B00000,
};





void setup(){
  lcd.createChar(0, smiley);
  Serial.begin(9600);
  lcd.begin(16, 2);
    while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
    SPI.begin();        // Inicializar SPI bus
    mfrc522.PCD_Init(); // Inicializar MFRC522 card

    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
}


  
void loop(){
  /*Serial.println( mfrc522.PICC_ReadCardSerial() );
  delay(500);*/
  
 /* SI CUANDO METEMOS LA TARJETA HAY CREDITO INTROUDCIDO, LO DEVOLVEMOS */
 if(mfrc522.PICC_IsNewCardPresent()==1 && credito >0 ){
      lcd.setCursor(8,0);
    lcd.write(byte(0));
  Serial.println("Devolviendo credito...");
  lcd.setCursor(0,1);
  lcd.print("Devolviendo");
  while(credito>0){
  credito=credito-1;
  delay(250);
  lcd.setCursor(12,1);
  
  if(credito<=9){
    lcd.print(" ");
    lcd.setCursor(13,1);
  }
  lcd.print(credito);
  }
  lcd.setCursor(0,1);
  lcd.print("                ");
 }
 /*************************************************************************/
  /* SI TENEMOS TARJETA INTRODUCIDA Y EL CREDITO DE LA MAQUINA ES 0, SEGUIMOS CON EL PROGRAMA */
 else if (mfrc522.PICC_IsNewCardPresent()==1 && credito == 0 && mfrc522.PICC_ReadCardSerial()==1 ){
      lcd.setCursor(8,0);
    lcd.write(byte(0));
 /* LEEMOS LOS DATOS DE LA TARJETA INTRODUCIDA Y SU CREDITO */
    // Datos de la tarjeta introducida (UID y piccType)
    Serial.print(F("Card UID:"));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print(F("PICC type: "));
    MFRC522::PICC_Type piccType = MFRC522::PICC_TYPE_MIFARE_1K;
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    // Comprueba que la tarjeta que tenemos sea compatible (puenteado)
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("This sample only works with MIFARE Classic cards."));
        return;
    }
    // In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    byte Bcredito = byte(credito);
    byte sector         = 1;
    byte blockAddr      = 4;
    byte dataBlock[]    = {
        0x00, 0x00, 0x00, 0x00, //  0,  0,   0,  0,
        0x00, 0x00, 0x00, 0x00, //  0,  0,   0,  0,
        0x00, 0x00, 0x00, 0x00, //  0, 0, 0, 0,
        0x00, 0x00, 0x00, Bcredito  // 0, 0,  0, Bcredito
    };

    byte trailerBlock   = 7;
    MFRC522::StatusCode status;
    byte buffer[18];
    byte size = sizeof(buffer);

    // Authenticate using key A (comprobando comunicacion con la llave)
    Serial.println(F("Authenticating using key A..."));
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Show the whole sector as it currently is (enseña datos de la llave)
    Serial.println(F("Current data in sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();

    // Read data from the block (Lee datos de la llave)
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("El credito de la tarjeta es: ")); 
    dump_credito(buffer, 16);
    Serial.print(Tcredito);
    Serial.print("€");
    Serial.println(" ");
    credito=Tcredito;
    
    }

 

/**************************************************************************************************************************************************** Acabamos de leer credito */


/* PARA AÑADIR CREDITO A LA TARJETA (SIN USO) */
 /*  // Write data to the block
    Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    dump_byte_array(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();

    // Read data from the block (again, should now be what we have written)
    Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Read() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
        
    // Check that data in block is what we have written
    // by counting the number of bytes that are equal
    Serial.println(F("Checking result..."));
    byte count = 0;
    for (byte i = 0; i < 16; i++) {
        // Compare buffer (= what we've read) with dataBlock (= what we've written)
        if (buffer[i] == dataBlock[i])
            count++;
    }
    Serial.print(F("Number of bytes that match = ")); Serial.println(count);
    if (count == 16) {
        Serial.println(F("Success :-)"));
    } else {
        Serial.println(F("Failure, no match :-("));
        Serial.println(F("  perhaps the write didn't work properly..."));
    }
    Serial.println();
        
    // Dump the sector data
    Serial.println(F("Current data in sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();
*/
    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();



   // SI NO HAY NINGUNA TARJETA PRESENTE
  if( mfrc522.PICC_IsNewCardPresent()==0 &&  mfrc522.PICC_ReadCardSerial()==0){ 
 /* if (Tcredito != 0){
    Tcredito =0;
    credito =0;
 
  }*/
  lcd.setCursor(10,0);
  lcd.print("Cred.:");
  lcd.setCursor(0,0);
  lcd.print("Selec.:  ");
  lcd.setCursor(12,1);
  if(credito<9){
    lcd.print("   ");
     lcd.setCursor(13,1);
  }
  lcd.print(credito);
  lcd.print("e");
  lcd.setCursor(0,1);
  if(sumatorio==""){
    lcd.print("         ");
  }
  else{  lcd.print(sumatorio);}
  
  char introducido = customKeypad.getKey();

/* CODIGO PARA TECLADO*/
  
if (introducido){                   
   if(introducido=='*'){            /*CODIGO PARA AUMENTAR CREDITO EN MAQUINA (MONEDAS y billetes de 5)*/
      Serial.println("Introduce el credito (SOLO MONEDAS de 1€ ,2€ y billetes de 5€)");
      Serial.print("Tu credito actual es ");
      Serial.print(credito);
      Serial.print("€");    
      for(i=0;i<80;i++){
        char creditoINPUT = customKeypad.getKey();
        if(creditoINPUT=='1'){          
          credito = credito + 1;
          }
        if(creditoINPUT=='2'){          
          credito = credito + 2;
        }
         if(creditoINPUT=='5'){          
          credito = credito + 5;
        }
       delay(25);
       }
      // delay(2000);
      Serial.println(" ");
      Serial.print("Tu credito actual es ");
      Serial.print(credito);
       Serial.print("€");
   }
   /*CODIGO PARA SELECCIONAR PRODUCTO*/
   else if(introducido=='1' || introducido=='2' || introducido=='3' || introducido=='4' || introducido=='5' || introducido=='6' || introducido=='7' || introducido=='8' || introducido=='9' || introducido=='0'){
    Serial.println("Selecciona el codigo del producto");
    sumatorio=String(sumatorio + introducido);
    Serial.println(sumatorio);
    

    int caracteres=sumatorio[4];
   if(caracteres >= 1){sumatorio="     ";}

    
    
   }
   /*PARA FINALIZAR SELECCION DE PRODUCTO PULSA #*/
   else if (introducido=='#'){
  //  sumatorio=String(sumatorio + introducido);  
    Serial.println("");
    Serial.print("Tu selección es : ");
    Serial.print(sumatorio);
    Serial.println("");
    /* ************************* Busca productos *********************************** */
if (sumatorio == "10"){
    if( credito >= producto1[2] && producto1[1]==1){
  Serial.println("Has seleccionado el producto1");
  credito = credito - producto1[2];
   lcd.setCursor(0,1);
  lcd.print("producto1");
  delay(2000);
  sumatorio="";  
   }
   
   else {Serial.println("Sin € / No disponible");
   lcd.setCursor(0,1);
  lcd.print("NoDisp");
  delay(2000);
  sumatorio="";}
}
else if (sumatorio == "20"){
  if ( credito >= producto2[2] && producto2[1]==1){
    Serial.println("Has seleccionado el producto2");
credito = credito - producto2[2];
  lcd.setCursor(0,1);
  lcd.print("producto2");
  delay(2000);
  sumatorio="";  
  }
   else {Serial.println("Sin € / No disponible");
       lcd.setCursor(0,1);
  lcd.print("NoDisp");
  delay(2000);
  sumatorio="";}
}
else if (sumatorio == "30"){
  if(credito >= producto3[2] && producto3[1]==1 ){
      Serial.println("Sin € / No disponible");
  credito = credito - producto3[2];
    lcd.setCursor(0,1);
    analogWrite(salidamotor,255);
    delay(10000);
  lcd.print("producto3");
  delay(2000);
  sumatorio="";  
  
  
  }
  else {Serial.println("No hay credito suficiente");
    lcd.setCursor(0,1);
  lcd.print("NoDisp");
  delay(2000);
  sumatorio="";}
}
else if (sumatorio == ""){
  Serial.println("Devolviendo credito...");
  lcd.setCursor(0,1);
  lcd.print("Devolviendo");
 while(credito>0){
  credito=credito-1;
  delay(250);
  lcd.setCursor(12,1);
  
  if(credito<=9){
    lcd.print(" ");
    lcd.setCursor(13,1);
  }
  lcd.print(credito);
  }
  lcd.setCursor(0,1);
  lcd.print("                ");
}
else {Serial.println("El producto seleccionado no existe");
  lcd.setCursor(0,1);
  lcd.print("No existe");
  delay(2000);
  sumatorio="";
}

    
   }
   /*SI NO ES NINGUN DELOS ANTERIORES -> ERROR*/
   else{Serial.println("ERROR - Codigo introducido no es valido");
   sumatorio = "";}
}
}
}

/*Acaba el Loop*/
/** Helper routine to dump a byte array as hex values to Serial. */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
        
    }
}
void dump_credito(byte *buffer, byte bufferSize){ /*Imprime credito y lo guarda en la variable Tcredito*/
  Serial.println(buffer[15],HEX);
  Serial.println("-");
  Tcredito = int(buffer[15]);
}

