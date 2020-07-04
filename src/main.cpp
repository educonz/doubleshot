#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>

// Pinos sensores infravermelhos
#define ANALOG_PIN_0 A6 // cabo branco
#define ANALOG_PIN_1 A5 // cabo cinza
#define ANALOG_PIN_2 A4 // cabo roxo

const String WIFI_SID = "DOUBLE_SHOT";
const String WIFI_PASS = "double91234";

// Pinos leds
const uint16_t PixelCount = 71;
const uint8_t PixelPin = 17;
const RgbColor CylonEyeColor(HtmlColor(0xF39120));

// Instancias LED
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(2);
uint16_t lastPixel = 0;
int8_t moveDir = 1;
AnimEaseFunction moveEase = NeoEase::CircularInOut;
RgbColor white(128);
RgbColor red(200, 10, 0);
RgbColor yellow(255, 100, 0);
RgbColor black(0);

HslColor hslWhite(white);
HslColor hslBlack(black);
HslColor hslYellow(yellow);

// Instancias WIFI
WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig Config;

// Configurações aplicação
bool traningValues = true;
uint16_t valorComparacaoSensor1 = 500;
uint16_t valorComparacaoSensor3 = 2900;
uint16_t valorMinComparacaoSensor2 = 2000;
uint16_t valorMaxComparacaoSensor2 = 3100;
uint16_t sensor1 = 0;
uint16_t sensor2 = 0;
uint16_t sensor3 = 0;
uint16_t valoresTreinamentosSensor1[50];
uint16_t valoresTreinamentosSensor2[50];
uint16_t valoresTreinamentosSensor3[50];
uint8_t indexTreinamento = 0;

uint8_t winner = 0;
uint8_t quantidadeEfeitos = 0;
RgbColor colorEfeito = black;

uint16_t i, j, vencedor1 = 0, vencedor2 = 0, vencedor3 = 0;

// ==== > Animação LED treinamento
void FadeAll(uint8_t darkenBy)
{
  RgbColor color;
  for (uint16_t indexPixel = 0; indexPixel < strip.PixelCount(); indexPixel++)
  {
    color = strip.GetPixelColor(indexPixel);
    color.Darken(darkenBy);
    strip.SetPixelColor(indexPixel, color);
  }
}

void FadeAnimUpdate(const AnimationParam &param)
{
  if (param.state == AnimationState_Completed)
  {
    FadeAll(10);
    animations.RestartAnimation(param.index);
  }
}

void MoveAnimUpdate(const AnimationParam &param)
{
  float progress = moveEase(param.progress);
  uint16_t nextPixel;
  if (moveDir > 0)
  {
    nextPixel = progress * PixelCount;
  }
  else
  {
    nextPixel = (1.0f - progress) * PixelCount;
  }

  if (lastPixel != nextPixel)
  {
    for (uint16_t i = lastPixel + moveDir; i != nextPixel; i += moveDir)
    {
      strip.SetPixelColor(i, CylonEyeColor);
    }
  }
  strip.SetPixelColor(nextPixel, CylonEyeColor);

  lastPixel = nextPixel;

  if (param.state == AnimationState_Completed)
  {
    moveDir *= -1;
    animations.RestartAnimation(param.index);
  }
}

void SetupAnimations()
{
  animations.StartAnimation(0, 5, FadeAnimUpdate);
  animations.StartAnimation(1, 2000, MoveAnimUpdate);
}

void SetRandomSeed()
{
  uint32_t seed;
  seed = analogRead(0);
  delay(1);

  for (int shifts = 3; shifts < 31; shifts += 3)
  {
    seed ^= analogRead(0) << shifts;
    delay(1);
  }
  randomSeed(seed);
}

void SetupLed()
{
  pinMode(PixelPin, OUTPUT);
  strip.Begin();
  strip.Show();
  SetupAnimations();
}
// <====  Animação LED treinamento

// >>> Animação LED executando
RgbColor Wheel(byte WheelPos)
{
  if (WheelPos < 85)
  {
    RgbColor color1(WheelPos * 3, 255 - WheelPos * 3, 0);
    return color1;
  }
  else if (WheelPos < 170)
  {
    WheelPos -= 85;
    RgbColor color2(255 - WheelPos * 3, 0, WheelPos * 3);
    return color2;
  }
  else
  {
    WheelPos -= 170;
    RgbColor color3(0, WheelPos * 3, 255 - WheelPos * 3);
    return color3;
  }
}
// <<< Animação LED executando

// ==== > Métodos de funcionamento dos SENSORES
void SetupSensores()
{
  pinMode(ANALOG_PIN_0, INPUT);
  pinMode(ANALOG_PIN_1, INPUT);
  pinMode(ANALOG_PIN_2, INPUT);
}

void LimparEfeitoSensor()
{
  for (int i = 0; i < PixelCount; i++)
  {
    strip.SetPixelColor(i, black);
  }
  strip.Show();
}
// <====  Métodos de funcionamento dos SENSORES

// ==== > Métodos de treinamento dos SENSORES
float Average(uint16_t *array, uint16_t len)
{
  long sum = 0L;
  for (int i = 0; i < len; i++)
    sum += array[i];
  return ((float)sum) / len;
}

void EfeitoSensor()
{
  for (int i = 0; i < PixelCount; i++)
  {
    strip.SetPixelColor(i, colorEfeito);
  }
  strip.Show();
}

void TreinandoValoresSensores()
{
  animations.UpdateAnimations();
  strip.Show();

  if (indexTreinamento < 50)
  {
    valoresTreinamentosSensor1[indexTreinamento] = analogRead(ANALOG_PIN_0);
    valoresTreinamentosSensor2[indexTreinamento] = analogRead(ANALOG_PIN_1);
    valoresTreinamentosSensor3[indexTreinamento] = analogRead(ANALOG_PIN_2);
    indexTreinamento++;
  }
  else
  {
    uint16_t avarageSensor;

    avarageSensor = Average(valoresTreinamentosSensor1, 50);
    valorComparacaoSensor1 = ((uint16_t)avarageSensor) + 150;
    Serial.print("Valor sensor 1: ");
    Serial.println(avarageSensor);

    avarageSensor = Average(valoresTreinamentosSensor2, 50);
    valorMaxComparacaoSensor2 = ((uint16_t)avarageSensor) + 400;
    valorMinComparacaoSensor2 = ((uint16_t)avarageSensor) - 200;

    Serial.print("Valor sensor 2: ");
    Serial.print(valorMinComparacaoSensor2);
    Serial.print(" - ");
    Serial.println(valorMaxComparacaoSensor2);

    avarageSensor = Average(valoresTreinamentosSensor3, 50);
    valorComparacaoSensor3 = ((uint16_t)avarageSensor) + 200;

    Serial.print("Valor sensor 3: ");
    Serial.println(valorComparacaoSensor3);

    memset(valoresTreinamentosSensor1, 0, sizeof(valoresTreinamentosSensor1));
    memset(valoresTreinamentosSensor2, 0, sizeof(valoresTreinamentosSensor2));
    memset(valoresTreinamentosSensor3, 0, sizeof(valoresTreinamentosSensor3));
    traningValues = false;
    LimparEfeitoSensor();
    Serial.println("Valores aprendidos!");
    delay(500);
  }
  delay(100);
}
// < ====  Métodos de treinamento dos SENSORES

// ===>  Métodos de execução
void AnimacaoStand()
{
  if (winner == 0)
  {
    if (j >= 256)
    {
      j = 0;
    }
    for (i = 0; i < PixelCount; i++)
    {
      strip.SetPixelColor(i, Wheel((i * 1 + j) & 255));
    }
    strip.Show();
    j++;
  }
}

void EfeitosVencedor()
{
  EfeitoSensor();
  delay(100);
  LimparEfeitoSensor();
  delay(100);
  if (quantidadeEfeitos == 20)
  {
    winner = 0;
    quantidadeEfeitos = 0;
    colorEfeito = black;
  }
  else
  {
    quantidadeEfeitos++;
  }
}

void VerificarSeHaVencedor()
{
  sensor1 = analogRead(ANALOG_PIN_0);
  if (winner == 0 && sensor1 >= valorComparacaoSensor1)
  {
    Serial.println(sensor1);
    winner = 1;
    colorEfeito = yellow;
    vencedor1++;
  }

  sensor2 = analogRead(ANALOG_PIN_1);
  if (winner == 0 && (sensor2 <= valorMinComparacaoSensor2 || sensor2 > valorMaxComparacaoSensor2))
  {
    winner = 2;
    colorEfeito = white;
    Serial.println(sensor2);
    vencedor2++;
  }

  sensor3 = analogRead(ANALOG_PIN_2);
  if (winner == 0 && sensor3 >= valorComparacaoSensor3)
  {
    Serial.println(sensor3);
    winner = 3;
    colorEfeito = red;
    vencedor3++;
  }
}
// < ==== Métodos de execução

// ===>  Métodos WIFI
void rootPage()
{
  char content[] PROGMEM = R"rawliteral(
    <!DOCTYPE HTML>
<html>
 <head>
    <meta charset="UTF-8">
   <meta name="viewport" content="width=device-width, initial-scale=1">
   <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
   <style>
     html {
      font-family: Arial;
      display: inline-block;
      margin: 0px auto;
      text-align: center;
     }
     h2 { font-size: 3.0rem; }
     p { font-size: 3.0rem; }
     .units { font-size: 1.2rem; }
     .dht-labels{
       font-size: 1.5rem;
       vertical-align:middle;
       padding-bottom: 15px;
     }
   </style>
 </head>
 <body>
   <h2>DOUBLE SHOT</h2>
   <p>
     <i class="fas fa-trophy" style="color:#daa520;"></i>
     <span class="dht-labels">1 posição</span>
     <span id="pos1">%POS1%</span>
   </p>
   <p>
     <i class="fas fa-trophy " style="color:#c0c0c0;"></i>
     <span class="dht-labels">2 posição</span>
     <span id="pos2">%POS2%</span>
   </p>
   <p>
     <i class="fas fa-trophy " style="color:#b01d0b;"></i>
     <span class="dht-labels">3 posição</span>
     <span id="pos3">%POS3%</span>
   </p>
 </body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      if (!!this.responseText) {
          const positions = this.responseText.split(';')
          document.getElementById("pos1").innerHTML = positions[0] || 0;
          document.getElementById("pos2").innerHTML = positions[1] || 0;
          document.getElementById("pos3").innerHTML = positions[2] || 0;
        }
    }
  };
  xhttp.open("GET", `${window.location.href}winners`, true);
  xhttp.send();
}, 1000);
</script>
 </html>)rawliteral";

  Server.send(200, "text/html", content);
}

void ReadWinners()
{
  String retorno = "";
  retorno += vencedor1;
  retorno += ";";
  retorno += vencedor2;
  retorno += ";";
  retorno += vencedor3;
  Server.send(200, "text/plain", retorno);
}

void SetupWifi()
{
  Config.apid = WIFI_SID;
  Config.psk = WIFI_PASS;
  Config.autoReconnect = true;
  Config.portalTimeout = 60000;

  Server.on("/", HTTP_GET, rootPage);
  Server.on("/winners", HTTP_GET, ReadWinners);

  Portal.config(Config);
  if (Portal.begin())
  {
    Serial.println("HTTP server:" + WiFi.localIP().toString());
  }
  else {
    Portal.end();
  }
}

// < ==== Métodos WIFI

void setup()
{
  Serial.begin(115200);

  SetupSensores();
  SetupLed();
  SetRandomSeed();
  
  delay(1000); // give me time to bring up serial monitor
  Serial.println("Iniciando double shot");
  Serial.println("Aprendendo valores de comparação");
  
  SetupWifi();
  delay(1000);
}

void loop()
{
  Portal.handleClient();
  if (traningValues)
  {
    TreinandoValoresSensores();
  }
  else if (winner > 0)
  {
    EfeitosVencedor();
  }
  else
  {
    VerificarSeHaVencedor();
    AnimacaoStand();
    delay(5);
  }
}