/*  Reveladora de Film MacGyver - Versión 0.8.2 - 29 de Enero de 2019 ---------
 *   
 *   
  v0.8.2
  -funcion de apagado
  -incluye funcion de apagado en Llenar, Vaciar y Agitar
  -Se agregan 50ms de delay entre paso y paso para evitar errores.
  
  v0.8.1
  -Mejora salteo de control de temperatura
  
  v0.8
  -SubPasos en lavados (para mejorar el lavado)

  v0.75.1
  -Agitado continuo en los lavados   
  
  v0.75.0
  -Solucion de errores
  -Separamos tiempo llenado y vaciado
  
  v0.74
  -omitir lavado si el tiempo para el paso es 0 (solo los lavados 2 y 3)
  -prueba de primer revelado byn
  -(Para hacer:) 
                 limpiar tuberia(abrir desecho solo valvula 5, prender bomba salida del tanque 52) 
                 cambiar agua dentro de los lavados al menos una vez
                 mostrar temp actual-programa en el display
                 control del boton ausente
                 fijador agitado continuo
                 ##separar tiempo de llenado y vaciado del tanque## (hecho)
                 
  v0.73
  -Pruebas de tiempos
  -Agrego pines que acivan driver
  
  v0.72
  -Seteo de todos los pines para el armado de la maquina prototipo
  
  v0.71
  -Ajuste de umbral de temperatura de quimicos/agua/programa
  
  v0.7
  -agregado de driver de los motores para evitar los golpes de corriente que ocurrian usando los reles
  -flujo de tanque 3
  -sensores de temperatura
  -bomba de mezclado de agua
 
  v0.6
  -constantes de sensores (especial atencion de los reles)
  -elección de programa (mediante los botones mas y menos)
  -mejora del flujo de los subpasos

  v0.5
  -Esquema final de la salida del display 20x4
  -agregar codigo de bombeo de aguacalentador
  -agregar agua de desecho
  -incluir mensaje status por Bluetooh (BT)

  v0.4
  -ajustes con el arduino mega y el display de 20x4

  v0.3
  -control de tiempo (diferencias de tiempo, se usa tipo time_t)
  -condiciones de ejecucion dentro del loop (pasos/subpasos/estadosactuales)

  v0.2
  -boceto de funciones para cada paso y subpaso 9
  -manejo de tiempos (variables, tipo de datos)

  v0.1
  -prueba del lcd
  -boton de avance
  -estructura de registro de los diferentes programas
  -boceto de settings para los diferentes programas (C41, Byn)
  ------------------------------------------------------------------------*/

// OneWire - Version: Latest
#include <OneWire.h>

// LiquidCrystal_I2C - Version: Latest
#include <LiquidCrystal_I2C.h>

// DallasTemperature-3.8.0 - Version: Latest
#include <DallasTemperature.h>

// Time - Version: /v1/libraries/time_1_5_0
#include <Time.h>
#include <TimeLib.h>

// Versión del Software         01234567890123456789
const String MacGyverVersion = "MacGyver ver. 0.8.2 ";

LiquidCrystal_I2C lcd(0x3F, 20, 4); // LCD Display 20 4

// Constantes - no cambian (definir aquí los pines)
const int ledPin              = LED_BUILTIN;  // Pin del ledinterno (13)
const int buttonPin           = 45;           // Pin del Botón        4
const int botonmas            = 39;           // pin del boton mas
const int botonmenos          = 35;           // pin del boton menos
const int tiempollenadotanque = 30;           // tiempo en segundos que se demora en llenar el tanque (se estima unos 25 segundos, verificar esto)
const int tiempovaciadotanque = 35;           // tiempo en segundos que se demora en vaciar el tanque (se estima unos 15 segundos, verificar esto)

const int releoff = HIGH; // porque los rele son alreves del mundo
const int releon  = LOW;  // porque los rele son alreves del mundo

const int termtroagua     = 32;         //termometro del agua
const int termtroquim     = 30;         //termometro del quimico

const int calentador      = 23;         //calentador de agua

const int bombacalentador = 33;         //bomba que circula el agua durante el calentado de químicos
const int bombaentrada    = 28;         //bomba que manda hacia el tanque (conectadas al driver)
const int bombasalida     = 26;         //bomba que vacia el tanque       (conectadas al driver)



const int valvula1        = 24;         //valvulas del tanque 1 (revelador)
const int valvula2        = 22;         //valvula del tanque 2 (blanqueador/paro)
const int valvula3        = 25;         //valvulas del tanque 3 (fijador)
const int valvula4        = 27;         //valvula de toma agua para lavado
const int valvula5        = 29;         //valvula de salida agua desperdicio
const int valvula6        = 31;         //valvula aire
//const int valvula7        = 33;         //4 TANQUE SIN




// Definir valores de Progr.              0,               1,               2,               3,               4,               5,               6,               7, etc
//                         "01234567890123","01234567890123","01234567890123","01234567890123","01234567890123","01234567890123","01234567890123","01234567890123"
const String programa[] = {"Test Prototipo","Kent400 a 21c ","C41 Standard  ","C41 Forzado +1","Otro          "};  //nombre del programa
const int       paso1[] = {              21,              21,              36,              38,              22};  //temperatura de quimicos
const int       paso2[] = {              60,              60,              60,              60,              30};  // ##primer lavado,
const int       paso3[] = {              540,             540,             180,             330,              30};  //tanque1 (revelador) (tiempo que el quimico esta en el tanque de revelado
const int       paso4[] = {              0,               0,              60,              60,              30};  // ##segundo lavado
const int       paso5[] = {              120,             120,             360,             360,              30};  //tanque2 (blanqueador/paro)  (tiempo que el quimico esta en el tanque de revelado)
const int       paso6[] = {              0,               0,              60,              60,              30};  // ##tercer lavado
const int       paso7[] = {              300,             300,             300,             300,              30};  //tanque3 (fijador)(tiempo que el quimico esta en el tanque de revelado)
const int       paso8[] = {              60,              60,              60,              60,              30};  // ##lavado final

// Instancia a las clases OneWire y DallasTemperature
OneWire wireagua(termtroagua);
OneWire wirequim(termtroquim);
DallasTemperature sensortempa(&wireagua);
DallasTemperature sensortempq(&wirequim);

// Variables
String lcd1 =  MacGyverVersion;       // inicializa las variables del display LCD
String lcd2 = "                    "; // segunda linea lo ideal es tener un display de 20x4lineas
String lcd3 = "Auto-Reveladora Film"; // tercera linea del lcd
String lcd4 = "                    "; // cuarta linea

//variables para desplegar informacion, tanto en el LCD y el BT
String infoPrograma     = "---------";
String infoTiempo       = "--:--";
String infoTempa        = "--._°";
String infoTempq        = "--._°";
String infoPasosubpaso  = "P-S-";
String infoDescrpaso    = "-";
String infoTsubpaso     = "";
String infoMsj          = "";

time_t T       = now(); // tiempo de la librería time <Time.h> ver uso en https://www.prometec.net/time-arduino/
time_t tInicio = now(); // tiempo de inicio (se usa para controlar el tiempo de un proceso)
time_t tActual = now(); // tiempo actual    (se usa para controlar el tiempo de un proceso)

int buttonState = 0;    // variable para leer el estado del botón
int contCiclos  = 0;    // variable contador de cilcos de arduino para enviar el status la LCD y al BT

int prog    = 0;        // Programa 0 por defecto es C41
int paso    = 0;        // Paso 0 inicial, mientras se configuran el sistema y hasta que se dé OK a comenzar (cuando se suma paso, se reseta a 1 subpaso)
int subpaso = 1;        // Subpaso dentro de cada paso (por ej: abrir valvulas, encender bomba, agitar, temperatura, etc)

float tempagua = 0;     // Temperatura del agua
float tempquim = 0;     // Temperatura de los Quimicos

//boolean prototipando = true;


// setup -------------------------------------------------------------------------------------------------- SETUP ****************************
void setup() {
  setTime(00, 00, 00, 29, 9, 1985); //Formato: hora, minutos, segundos, días, mes, año (Primera emisión del programa MacGyver:	29 de septiembre de 1985)

  // pines de salida
  pinMode(ledPin,          OUTPUT);
  pinMode(calentador,      OUTPUT);
  pinMode(bombacalentador, OUTPUT);
  pinMode(bombaentrada,    OUTPUT);
  pinMode(bombasalida,     OUTPUT);
  pinMode(valvula1,        OUTPUT);
  pinMode(valvula2,        OUTPUT);
  pinMode(valvula3,        OUTPUT);
  pinMode(valvula4,        OUTPUT);
  pinMode(valvula5,        OUTPUT);
  pinMode(valvula6,        OUTPUT);
 

  // importante comenzar con los relee apagados (setearlos a releoff)
  digitalWrite(calentador,      releoff);
  digitalWrite(bombacalentador, releoff);
  digitalWrite(bombaentrada,        LOW);
  digitalWrite(bombasalida,         LOW);
  digitalWrite(valvula1,        releoff);
  digitalWrite(valvula2,        releoff);
  digitalWrite(valvula3,        releoff);
  digitalWrite(valvula4,        releoff);
  digitalWrite(valvula5,        releoff);
  digitalWrite(valvula6,        releoff);
  


  // pines de entrada
  pinMode(buttonPin,        INPUT);
  pinMode(botonmas,         INPUT);
  pinMode(botonmenos,       INPUT);

  // Iniciamos la comunicación serie
  Serial.begin(9600);
  // Iniciamos el bus 1-Wire
  sensortempa.begin();
  sensortempq.begin();

  // despliega la versión en el LCD
  lcd.init();
  lcd.backlight();
  MensajeLcd();
  delay(1000);
  lcd.clear();

    //  leer la temperatura de agua y quimicos al inicio
    sensortempa.requestTemperatures();
    sensortempq.requestTemperatures();
    tempagua = sensortempa.getTempCByIndex(0); //lee la temperatura del agua
    tempquim = sensortempq.getTempCByIndex(0); //lee la temperatura del quimico

}

// loop --------------------------------------------------------------------------------------------------- LOOP *****************************
void loop() {
  //botón se usa para pasar al siguiente paso
  BotonPaso();


  // Bluetooth sólo 1 cada X veces sobre el tiempo de reloj de arduino, no es necesario enviar datos 1600 veces por segundo.
  contCiclos ++;
  if (contCiclos > 100) {
    contCiclos = 0; // se resetea el contador
    sensortempa.requestTemperatures();
    sensortempq.requestTemperatures();
    tempagua = sensortempa.getTempCByIndex(0); //lee la temperatura del agua
    tempquim = sensortempq.getTempCByIndex(0); //lee la temperatura del quimico
    if ((paso > 0) && (paso < 9)) {
      T = now();
      infoPrograma     =  programa[prog];
      infoTiempo       =  formatommss(T);
      infoTempa        =  String(tempagua); //leer temperatura agua
      infoTempq        =  String(tempquim); //leer temperatura quim
      infoPasosubpaso  =  "P" + String(paso) + "S" + String(subpaso);
      StatusLcd(); // se actualiza la info del LCD en pasos distintos al de elegir programa
    }
    String auxdescrBT = infoPasosubpaso + " " + infoDescrpaso + " " + infoTsubpaso; //concatena las cadenas para la descripción en formato para BT
    StatusBT(infoPrograma, infoTiempo, infoTempa, infoTempq, auxdescrBT, infoMsj);  //llama a la funcion que envía el status al dispositivo android
  }

if (paso == 0) {
    //se llama la función de elegir programa de revelado
    //               01234567890123456789
    infoDescrpaso = "Elegir Programa     ";
    ElegirPrograma();
  }
  if (paso == 1) {
    //Ajustar la Temperatura de los quimicos
    //               01234567890123456789
    infoDescrpaso = "Ajustando Temp.     ";
    Paso1();
  }
  if (paso == 2) {
    //Primer Lavado;
    //               01234567890123456789
    infoDescrpaso = "Primer Lavado       ";
    Paso2();
  }
  if (paso == 3) {
    //Tanque 1
    //               01234567890123456789
    infoDescrpaso = "Revelando           ";
    Paso3();
  }
  if (paso == 4) {
    //Segundo Lavado
    if (paso4[prog]==0){
        paso ++;
        subpaso = 1;  
    }else {
        //               01234567890123456789 
        infoDescrpaso = "Segundo Lavado      ";
        Paso4();
    }
  }
  if (paso == 5) {
    //Tanque 2
    //               01234567890123456789
    infoDescrpaso = "Blanqueando/Paro    ";
    Paso5();
  }
  if (paso == 6) {
    //Tercer Lavado
    if (paso4[prog]==0){
        paso ++;
        subpaso = 1; 
    }else{    
        //               01234567890123456789
        infoDescrpaso = "Tercer Lavado       ";
        Paso6();
    }    
  }
  if (paso == 7) {
    //Tanque 3
    //               01234567890123456789
    infoDescrpaso = "Fijando             ";
    Paso7();
  }
  if (paso == 8) {
    //Lavado final
    //               01234567890123456789
    infoDescrpaso = "Lavado Final        ";
    Paso8();
  }
  if (paso == 9) {
      // apagar lo que se pueda :P
  ApagarValvulas();
    //  Se terminó todo el proceso se indica en el display - sumar alarma sonora
    lcd1 =  MacGyverVersion;
    lcd2 = "Auto-Reveladora Film";
    lcd3 = " Revelado           ";
    lcd4 = "          Completo! ";
    MensajeLcd();
    delay(1000);
    lcd3 = " Que buenas         ";
    lcd4 = "        esas fotos! ";
    MensajeLcd();
    delay(1000);
  }
  if (paso == 10) {
      // apagar lo que se pueda :P
  ApagarValvulas();
    lcd1 = "    ATENCION!!!     ";
    lcd2 = "Hubo un Error Grave ";
    // en lcd3 y lcd 4 estará una descprición de cual fue el error
    MensajeLcd();
    delay(1000);
  }
}


// funciones ----------------------------------------------------------------------------------------------- FUNCIONES ****************************
String formatommss(time_t tiempoaux) {
  String mmss = ""; //tiempo MM:SS string de 5 de ancho
  if (minute(tiempoaux) <= 9) mmss = "0";
  mmss = mmss + String(minute(tiempoaux)) + ":";
  if (second(tiempoaux) <= 9) mmss = mmss + "0";
  mmss = mmss + String(second(tiempoaux));
  return mmss;
}

int controltiempo(time_t tiempoactual, time_t tiempoinicio) {
  //devuelve la cantidad de segundos entre los tiempos pasados por parametro (variables tipo time_t)
  int cantsegundos = (hour(tiempoactual) * 3600 + minute(tiempoactual) * 60 + second(tiempoactual)) - (hour(tiempoinicio) * 3600 + minute(tiempoinicio) * 60 + second(tiempoinicio));
  return cantsegundos;
}

void LimpiaLcd() {
  lcd.clear();
  lcd.setCursor(0, 0);
}

void ApagarValvulas() {
  digitalWrite(calentador,      releoff);
  digitalWrite(bombacalentador, releoff);
  digitalWrite(bombaentrada,        LOW);
  digitalWrite(bombasalida,         LOW);
  digitalWrite(valvula1,        releoff);
  digitalWrite(valvula2,        releoff);
  digitalWrite(valvula3,        releoff);
  digitalWrite(valvula4,        releoff);
  digitalWrite(valvula5,        releoff);
  digitalWrite(valvula6,        releoff);
}

void MensajeLcd() {    //imprime en el lcd el contenido de las variables lcd1, lcd2, lcd3 y lcd4)
  lcd.setCursor(0, 0);
  lcd.print(lcd1);
  lcd.setCursor(0, 1);
  lcd.print(lcd2);
  lcd.setCursor(0, 2);
  lcd.print(lcd3);
  lcd.setCursor(0, 3);
  lcd.print(lcd4);
}

void StatusLcd() { // despliega en el LCD las variables de información de status
  // 01234567890123456789   // asi sería la forma de desplegar la info en un lcd 20x4
  //"Programa      MMM:SS"; // inicializa las variables del display LCD
  //"Agua:XX.X°Quim:XX.X°"; // segunda linea lo ideal es tener un display de 20x4lineas
  //"P-S- (XXX seg.)     "; // tercera linea del lcd
  //"descripcion         "; // cuarta linea

  lcd.setCursor(0, 0);
  lcd.print(infoPrograma);

  lcd.setCursor(15, 0);
  lcd.print(infoTiempo);

  lcd.setCursor(0, 1);
  lcd.print("Agua:" + infoTempa);

  lcd.setCursor(10, 1);
  lcd.print("Quim:" + infoTempq);

  lcd.setCursor(0, 2);
  lcd.print(infoPasosubpaso);

  lcd.setCursor(5, 2);
  lcd.print(infoTsubpaso);

  lcd.setCursor(0, 3);
  lcd.print(infoDescrpaso);
}

void StatusBT(String progBT, String tiempoBT, String tempaBT, String tempqBT, String descrBT, String msjBT) {
  String enviarBT = "arduino|" + progBT + "|" + tiempoBT + "|" + tempaBT + "|" + tempqBT + "|" + descrBT  + "|" + msjBT + "|fin\n";
  //revisar declarar módulo Bluetooh (SerialBT)
  //SerialBT.print(enviarBT); // se envía la cadena de texto con los datos via BT a la App Android
}

void BotonPaso() {
  // Leer el estado del botón
  buttonState = digitalRead(buttonPin);
  // chequea si el boton esta apretado, osea igual a HIGH
  if (buttonState == HIGH) {
    if (subpaso < 3) {
      subpaso ++;
    } else {
      paso ++;
      subpaso = 1;
    }
    delay(250);      // delay 250ms para evitar error por pulsacion prolongada
    tInicio = now(); // se resetea el tiempo de inicio del subpaso aumentado
  }
}

void BotonElegirPrograma() {
  // Leer el estado del botón
  buttonState = digitalRead(botonmas);
  // chequea si el boton esta apretado, osea igual a HIGH
  if ((buttonState == HIGH) && (prog < 4)) {
    prog ++;
    delay(250); // delay 250ms para evitar error por pulsacion prolongada
  }
  buttonState = digitalRead(botonmenos);
  // chequea si el boton esta apretado, osea igual a HIGH
  if ((buttonState == HIGH) && (prog > 0)) {
    prog --;
    delay(250); // delay 250ms para evitar error por pulsacion prolongada
  }
}

// funciones de cada paso ----------------------------------------------------------------------------------
void ElegirPrograma() {
  // Paso 0
  //menú de eleccion de programa de revelado

  if (subpaso == 1) { //lo si paso = 0 y subpaso = 1 la rueda de seleccion esta activa
    lcd.setCursor(0, 0);
    //         01234567890123456789
    lcd.print("Elija el Programa   ");
    lcd.setCursor(0, 1);
    lcd.print("    de Revelado");
    lcd.setCursor(0, 3);
    lcd.print(programa[prog]);
    //subpaso ++; // no hay nada de codigo por ahora
    BotonElegirPrograma(); // llama a cambiar el programa actual elegido según se pulse + o -
  }
  if (subpaso == 2) {
    lcd.setCursor(0, 2);
    lcd.print("Confirma Iniciar?    ");
    lcd.setCursor(0, 3);
    lcd.print(programa[prog]);
    //subpaso ++; // 
  }
  if (subpaso == 3) {
    LimpiaLcd();
    lcd.setCursor(0, 0);
    lcd.print("Verificando Sensores ...           ");
    if ((tempquim > paso1[prog])&& (programa[prog] != "Test Prototipo")){
      lcd3 = "Temp.Quimico " + String(tempquim);
      lcd4 = "Temp.Progr.  " + String(paso1[prog]);
      paso = 10;  // error de temperatura en el quimico
      }else{
        paso ++;
        subpaso = 1; // se avanza al siguiente paso
    }
    LimpiaLcd();
    // cuando se termina este paso se setea a cero el reloj global para informar el tiempo total del proceso de revelado
    setTime(00, 00, 00, 29, 9, 1985); //Formato: hora, minutos, segundos, días, mes, año (Primera emisión del programa MacGyver: 29 de septiembre de 1985)
  }

}

void Paso1() {
  //Paso 1 - temperatura de quimicos

  if (subpaso == 1) {

    if ((tempquim > paso1[prog]) && (programa[prog] != "Test Prototipo")) {
      lcd3 = "Temp.Quimico " + String(tempquim);
      lcd4 = "Temp.Progr.  " + String(paso1[prog]);
      paso = 10;  // error de temperatura en el quimico
      }else{
         //si la tempquim es menor a la del programa y la bomba esta apagada y el calentador esta apagado encenderlos y pasar al subpaso 2
         if (digitalRead(bombacalentador) == releoff){
             digitalWrite(bombacalentador, releon);   //prende la bomba que circula el agua del calentador
          }
          if (digitalRead(calentador) == releoff){
             digitalWrite(calentador, releon);        //prende el calentador
          }
        subpaso ++; // se avanza al siguiente subpaso
    }
  }

  if (subpaso == 2) {
    //si la tempquim/agua es mayor o igual a la del programa(testear umbral diferencia de temp agua/quim) (apagar si esta el calentador) y pasar al sub paso 3

      if ((tempquim >= paso1[prog] - ((tempagua - tempquim)/2))||(tempquim >= paso1[prog]) || (programa[prog] =="Test Prototipo")){
             digitalWrite(calentador, releoff);        //apaga el calentador
             delay(100);
             subpaso ++; // se avanza al siguiente subpaso
          }
  }

  if (subpaso == 3) {
    //si la tempquim es mayor o igual a la del programa (aca no juega el umbral) se apaga la bomba de circulacion y se pasa al siguiente paso 2 subpaso 1 se comienza el proceso 
          if ((tempquim >= paso1[prog]) || (programa[prog] =="Test Prototipo")){
             digitalWrite(bombacalentador, releoff);        //apaga la bombacalentador
             delay(100);
             tInicio = now();
             subpaso = 1; // se avanza al siguiente paso
             paso ++;
          }
  }
}

void Paso2() {
  //Paso 2 - primer lavado

  if (subpaso == 1) {
    LlenarTanqueRevelado(0);
  }
  if (subpaso == 2) {
    AgitarTanqueRevelado(paso2[prog]);
  }
  if (subpaso == 3) {
    VaciarTanqueRevelado(0);
  }
  if (subpaso == 4) {
    LlenarTanqueRevelado(0);
  }
  if (subpaso == 5) {
    AgitarTanqueRevelado(paso2[prog]);
  }
  if (subpaso == 6) {
    VaciarTanqueRevelado(0);
  }
}

void Paso3() {
  //Paso 3 - tanque 1 (revelador)

  if (subpaso == 1) {
    LlenarTanqueRevelado(1);
  }
  if (subpaso == 2) {
    AgitarTanqueRevelado(paso3[prog]);
  }
  if (subpaso == 3) {
    VaciarTanqueRevelado(1);
  }
}

void Paso4() {
  //Paso 4 - segundo lavado

  if (subpaso == 1) {
    LlenarTanqueRevelado(0);
  }
  if (subpaso == 2) {
    AgitarTanqueRevelado(paso4[prog]);
  }
  if (subpaso == 3) {
    VaciarTanqueRevelado(0);
  }
    if (subpaso == 4) {
    LlenarTanqueRevelado(0);
  }
  if (subpaso == 5) {
    AgitarTanqueRevelado(paso4[prog]);
  }
  if (subpaso == 6) {
    VaciarTanqueRevelado(0);
  }
}

void Paso5() {
  //Paso 5 - tanque 2 (blanqueador - baño de paro)

  if (subpaso == 1) {
    LlenarTanqueRevelado(2);
  }
  if (subpaso == 2) {
    AgitarTanqueRevelado(paso5[prog]);
  }
  if (subpaso == 3) {
    VaciarTanqueRevelado(2);
  }
}

void Paso6() {
  //Paso 6 - tercer lavado
  if (subpaso == 1) {
    LlenarTanqueRevelado(0);
  }
  if (subpaso == 2) {
    AgitarTanqueRevelado(paso6[prog]);
  }
  if (subpaso == 3) {
    VaciarTanqueRevelado(0);
  }
    if (subpaso == 4) {
    LlenarTanqueRevelado(0);
  }
  if (subpaso == 5) {
    AgitarTanqueRevelado(paso6[prog]);
  }
  if (subpaso == 6) {
    VaciarTanqueRevelado(0);
  }
}

void Paso7() {
  //Paso 7 - tanque 3 (fijador)

  if (subpaso == 1) {
    LlenarTanqueRevelado(3);
  }
  if (subpaso == 2) {
    AgitarTanqueRevelado(paso7[prog]);
  }
  if (subpaso == 3) {
    VaciarTanqueRevelado(3);
  }
}

void Paso8() {
  //Paso 8 - lavado final

  if (subpaso == 1) {
    LlenarTanqueRevelado(0);
  }
  if (subpaso == 2) {
    AgitarTanqueRevelado(paso8[prog]);
  }
  if (subpaso == 3) {
    VaciarTanqueRevelado(0);
  }
    if (subpaso == 4) {
    LlenarTanqueRevelado(0);
  }
  if (subpaso == 5) {
    AgitarTanqueRevelado(paso8[prog]);
  }
  if (subpaso == 6) {
    VaciarTanqueRevelado(0);
  }
}

// funciones de cada subpaso ---------------------------------------------------------------------------
void AgitarTanqueRevelado(int duracion) {


  if ((subpaso == 2) || (subpaso == 5)) {
    // solo si se esta en SubPaso 2
    // recibe como parametro el tiempo para el paso actual segun el programa elegido
    // se circula el reloj del arduino IMPORTANTE: actualizar el tiempoactual
    // durante el primer minuto si en paso 1 ... luego cada 10 segundos chequear/definir esto

    tActual = now(); // actualiza el tiempo actual para eventuales controles del tiempo transcurrido
    infoTsubpaso = "(" + String(duracion - controltiempo(tActual, tInicio)) + " seg.)   " ;

    if (controltiempo(tActual, tInicio) <=  duracion) { //y agitador apagado
      // encender agitador
      // si es un paso de lavado se debe agitar siempre
      // si controltiempo(tActual, tInicio) es menorigual a 60 ó controltiempo(tActual, tInicio) es mayor (((controltiempo(tActual, tInicio)/ 60)*60)+50) y menor (((controltiempo(tActual, tInicio)/ 60)*60)+60)

      if (((paso == 2) || (paso == 4) || (paso == 6) || (paso == 8)) || (controltiempo(tActual, tInicio)<= 60) || (controltiempo(tActual, tInicio) >=  (((int)(controltiempo(tActual, tInicio)/ 60)*60)+50) && controltiempo(tActual, tInicio) <= (((int)(controltiempo(tActual, tInicio)/ 60)*60)+60))){
         // agitar
         Agitado();
      } else {
         // detener agitado
         DetenerAgitado();     
      }
    }     
    if (controltiempo(tActual, tInicio) > duracion) { //y agitador encendido
      //detener agitado
      DetenerAgitado();
      ApagarValvulas();
      tInicio = now(); // actualiza el tiempo de inicio del vaciado
      subpaso ++; //se pasa al siguiente subpaso (vaciar tanque)      
    }
  }
}

void LlenarTanqueRevelado(int tanque) {
  tActual = now();
  infoTsubpaso = "cargando...";
  if ((subpaso == 1) || (subpaso == 4)) {
    // solo si esta en SubPaso 1
    // recibe como parametro desde que tanque se llena (falta definir las valvulas de cada tanque)

    if (controltiempo(tActual, tInicio) <=  tiempollenadotanque) { //y valvula cerrada
      //encender valvula y bomba
      if (tanque == 0) {
        digitalWrite(valvula4, releon);     //abre la valvula 4 (agua para lavado)
      }
      if (tanque == 1) {
        digitalWrite(valvula1, releon);     //abre la valvula 1
      }
      if (tanque == 2) {
        digitalWrite(valvula2, releon);     //abre la valvula 2
      }
       if (tanque == 3) {
        digitalWrite(valvula3, releon);     //abre la valvula3
      }
      digitalWrite(bombaentrada, HIGH); //activa la bomba de entrada
    }
    if (controltiempo(tActual, tInicio) >   tiempollenadotanque) { //y valvula cerrada  {
      // cerrar valvula y apagar bomba
      if (tanque == 0) {
        digitalWrite(valvula4, releoff);     //abre la valvula 4 (agua para lavado)
      }
      if (tanque == 1) {
        digitalWrite(valvula1, releoff);     //cierra la valvula 1
      }
      if (tanque == 2) {
        digitalWrite(valvula2, releoff);     //cierra la valvula 2
      }
      if (tanque == 3) {
        digitalWrite(valvula3, releoff);     //cierra la valvula 3    
      }
      digitalWrite(bombaentrada, LOW); //apaga la bomba de entrada
      ApagarValvulas();
      tInicio = now(); // actualiza el tiempo de inicio 
      subpaso ++;      // se pasa al siguiente subpaso (2 o 5)
    }
    
  }
}


void VaciarTanqueRevelado(int tanque) {
  tActual = now();
  infoTsubpaso = "vaciando...";
  if ((subpaso == 3) || (subpaso == 6)){
    // solo si esta en SubPaso 3
    // recibe como parametro hacia que tanque se vacía (falta definir las valvulas de cada tanque)

    if (controltiempo(tActual, tInicio) <=  (tiempovaciadotanque)) { //y valvula cerrada
      //encender valvula y bomba
      if ((tanque == 0) && (digitalRead(valvula5) == releoff)) {
        digitalWrite(valvula5, releon);      //abre la la valvula (agua desecho)
      }
      if ((tanque == 1) && (digitalRead(valvula1) == releoff)) {
        digitalWrite(valvula1, releon);      //abre la la valvula
      }
      if ((tanque == 2) && (digitalRead(valvula2) == releoff)) {
        digitalWrite(valvula2, releon);      //abre la la valvula
      }
      if ((tanque == 3) && (digitalRead(valvula3) == releoff)) {
        digitalWrite(valvula3, releon);      //abre la la valvula
      }
      digitalWrite(bombasalida, HIGH);   //activa la bomba de salida
    }
    if (controltiempo(tActual, tInicio) >   tiempovaciadotanque) { //y valvula cerrada  {
      // cerrar valvula y apagar bomba
      if ((tanque == 0) && (digitalRead(valvula5) == releon)) {
        digitalWrite(valvula5, releoff);   //cierra la la valvula (agua desecho)
      }
      if ((tanque == 1) && (digitalRead(valvula1) == releon)) {
        digitalWrite(valvula1, releoff);   //cierra la la valvula
      }
      if ((tanque == 2) && (digitalRead(valvula2) == releon)) {
        digitalWrite(valvula2, releoff);   //cierra la la valvula
      }
      if ((tanque == 3) && (digitalRead(valvula3) == releon)) {
        digitalWrite(valvula3, releoff);   //cierra la la valvula
      }
      digitalWrite(bombasalida, LOW); //apaga la bomba de salida
      ApagarValvulas();
      tInicio = now(); // actualiza el tiempo de inicio del vaciado
      if (subpaso == 3){
        if ((paso == 2) || (paso == 4) || (paso == 6) || (paso == 8)){ //si es un lavado
           subpaso ++;
        }else{
           subpaso = 1;
           paso ++; //se pasa al siguiente paso
        }
      }     
      if (subpaso == 6){
        subpaso = 1;
        paso ++; //se pasa al siguiente paso
      }
      

    }
  }
}

void Agitado() {
        digitalWrite(valvula6, releon);   //abre la valvula 6
        digitalWrite(bombaentrada, HIGH); //activa la bomba de entrada
}    
void DetenerAgitado() {
        digitalWrite(valvula6, releoff);  //cierra la valvula 6
        digitalWrite(bombaentrada, LOW);  //apaga la bomba de entrada
} 
