#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>   // Library: "WebSockets" by Markus Sattler

#define IN1 4
#define IN2 5
#define IN3 6
#define IN4 7
#define ENA 15
#define ENB 16
#define FAN 17

#define FRONT_LED1 21
#define FRONT_LED2 9
#define REAR_LED1 36
#define REAR_LED2 37

const char* ssid = "ESP32-CAR";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);

int currentCommand = 0;
int requestedSpeed = 180;
int modeIdx = 3;
int modeCap = 255;
bool boost = false;
bool fanState = false;
bool frontLedState = false;
bool rearLedState = false;
unsigned long lastCommandTime = 0;
unsigned long lastStatusTime = 0;

const int modeCaps[4] = {120, 170, 215, 255};
const unsigned long motorTimeout = 400;   // failsafe

const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<meta name="theme-color" content="#f4f3fb">
<title>SMART CAR</title>
<style>
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent;user-select:none;-webkit-user-select:none;-webkit-touch-callout:none}
body{margin:0;min-height:100vh;display:flex;justify-content:center;padding:14px 14px 120px;color:#1e1b2e;
 font-family:system-ui,-apple-system,'Segoe UI',Roboto,Arial,sans-serif;background:#f4f3fb;touch-action:manipulation}
.wrap{width:100%;max-width:440px}
.top{display:flex;justify-content:space-between;align-items:center;font-size:12px;color:#6b6b85}
.pill{padding:5px 12px;border-radius:99px;background:#fff;border:1px solid #ece8fb;color:#16a34a;font-weight:700}
.pill.off{color:#e11d48}
.brand{text-align:center;letter-spacing:.45em;font-weight:700;color:#6d28d9;font-size:18px;margin:10px 0 14px;padding-left:.45em}
.card{background:#fff;border-radius:24px;padding:16px;margin-bottom:14px;border:1px solid #ece8fb}
.gauge{position:relative;width:250px;height:250px;margin:0 auto}
.gauge svg{width:100%;height:100%}
.readout{position:absolute;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center}
.num{font-size:62px;font-weight:700;line-height:1}
.unit{font-size:12px;color:#8a8aa3;margin-top:2px}
.mname{font-size:12px;font-weight:700;color:#6d28d9;margin-top:6px;letter-spacing:1px}
.gear{font-size:22px;font-weight:700;margin-top:2px}
.carbox{display:flex;justify-content:center;margin-top:-6px}
.beam{fill:#fcd34d;opacity:0}
.hl{fill:#c4b5fd}
.tl{fill:#fca5a5}
.f .beam{opacity:.5}
.f .hl{fill:#facc15}
.r .tl{fill:#ef4444}
.seg{display:flex;background:#f1ebff;border-radius:99px;padding:4px;gap:2px}
.seg button{flex:1;border:0;background:none;padding:10px 0;border-radius:99px;font-size:12px;font-weight:700;color:#6b6b85;cursor:pointer}
.seg button.on{background:#6d28d9;color:#fff}
.title{font-size:12px;font-weight:700;letter-spacing:1px;color:#8a8aa3;margin:0 0 12px}
.stick{position:relative;width:230px;height:230px;margin:14px auto 4px;border-radius:50%;touch-action:none;
 background:radial-gradient(circle,#fff 35%,#f1ebff);border:1px solid #ddd5fa}
.stick::before{content:"";position:absolute;inset:34px;border-radius:50%;border:1px dashed #cdbff7}
.stick::after{content:"";position:absolute;inset:0;border-radius:50%;
 background:linear-gradient(#ddd5fa,#ddd5fa) center/1px 100% no-repeat,linear-gradient(#ddd5fa,#ddd5fa) center/100% 1px no-repeat}
.knob{position:absolute;left:50%;top:50%;width:78px;height:78px;margin:-39px 0 0 -39px;border-radius:50%;z-index:2;
 background:linear-gradient(145deg,#8b5cf6,#5b21b6);will-change:transform;pointer-events:none}
.keys{display:none;grid-template-columns:repeat(3,1fr);gap:10px;margin-top:14px}
.keys button{height:62px;border-radius:18px;border:1px solid #ddd5fa;background:#faf8ff;font-size:22px;color:#6d28d9;touch-action:none;cursor:pointer}
.keys button.act{background:#6d28d9;color:#fff}
.keys button.s{background:#fee2e2;border-color:#fecaca;color:#e11d48;font-size:14px;font-weight:700}
.row{display:flex;align-items:center;gap:10px;margin-top:13px;font-size:13px}
.row span{width:88px;color:#6b6b85}
.bar{flex:1;height:6px;border-radius:9px;background:#ece8fb;overflow:hidden}
.bar i{display:block;height:100%;width:0;border-radius:9px;background:#6d28d9}
.row em{font-style:normal;font-weight:700;min-width:54px;text-align:right}
.sub{display:flex;justify-content:space-between;font-size:12px;color:#6b6b85;margin-top:16px}
.sp{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:12px}
.sp b{font-size:26px}
.boost{width:100%;margin-top:16px;padding:13px;border:1px solid #ddd5fa;border-radius:99px;background:#faf8ff;color:#6d28d9;font-weight:700;font-size:13px;touch-action:none;cursor:pointer}
.boost.on{background:#6d28d9;color:#fff}
input[type=range]{width:100%;height:6px;appearance:none;-webkit-appearance:none;border-radius:9px;outline:none;background:linear-gradient(90deg,#c4b5fd,#6d28d9)}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:26px;height:26px;border-radius:50%;background:#fff;border:3px solid #6d28d9}
input[type=range]::-moz-range-thumb{width:20px;height:20px;border-radius:50%;background:#fff;border:3px solid #6d28d9}
.dock{position:fixed;left:50%;bottom:12px;transform:translateX(-50%);width:calc(100% - 24px);max-width:416px;
 display:grid;grid-template-columns:1fr 1fr 84px 1fr;align-items:center;background:#fff;border-radius:34px;padding:8px 6px;
 border:1px solid #ece8fb;box-shadow:0 8px 20px rgba(109,40,217,.15)}
.dk{background:none;border:0;display:flex;flex-direction:column;align-items:center;gap:3px;font-size:11px;font-weight:600;color:#6b6b85;padding:7px 0;border-radius:18px;touch-action:none;cursor:pointer}
.dk b{font-size:20px;font-weight:400}
.dk.on{color:#6d28d9;background:#f1ebff}
.sos{width:76px;height:76px;margin:-34px auto 0;border-radius:50%;border:5px solid #f4f3fb;color:#fff;font-weight:700;font-size:13px;cursor:pointer;
 background:linear-gradient(145deg,#8b5cf6,#5b21b6)}
.sos:active{transform:scale(.94)}
@media(max-width:350px){.gauge{width:210px;height:210px}.num{font-size:50px}.stick{width:190px;height:190px}}
</style>
</head>
<body>
<div class="wrap">

<div class="top"><span class="pill off" id="status">● Offline</span><span>Linked devices: <b id="clients">1</b></span></div>
<div class="brand">SMART IOT CAR</div>

<div class="card" id="hero">
 <div class="gauge">
  <svg viewBox="0 0 200 200">
   <defs><linearGradient id="g" x1="0" y1="1" x2="1" y2="0"><stop offset="0" stop-color="#c4b5fd"/><stop offset="1" stop-color="#6d28d9"/></linearGradient></defs>
   <g id="ticks" stroke="#cfc6ee" stroke-width="1.5"></g>
   <circle cx="100" cy="100" r="76" fill="none" stroke="#ece8fb" stroke-width="10" stroke-linecap="round" stroke-dasharray="358.1 477.5" transform="rotate(135 100 100)"/>
   <circle id="arc" cx="100" cy="100" r="76" fill="none" stroke="url(#g)" stroke-width="10" stroke-linecap="round" stroke-dasharray="0.1 477.5" transform="rotate(135 100 100)"/>
  </svg>
  <div class="readout">
   <div class="num" id="num">0</div>
   <div class="unit">% power</div>
   <div class="mname" id="mname">SPORT PLUS</div>
   <div class="gear" id="gear">P</div>
  </div>
 </div>
 <div class="carbox">
  <svg viewBox="0 -24 100 124" width="86">
   <polygon class="beam" points="34,10 6,-22 44,-22"/><polygon class="beam" points="66,10 56,-22 94,-22"/>
   <rect x="20" y="18" width="10" height="22" rx="3" fill="#2b2540"/><rect x="70" y="18" width="10" height="22" rx="3" fill="#2b2540"/>
   <rect x="20" y="62" width="10" height="22" rx="3" fill="#2b2540"/><rect x="70" y="62" width="10" height="22" rx="3" fill="#2b2540"/>
   <rect x="27" y="4" width="46" height="92" rx="18" fill="#fff" stroke="#6d28d9" stroke-width="2"/>
   <path d="M36 30H64L60 48H40Z" fill="#cfc3fb"/><path d="M38 72H62L60 83H40Z" fill="#cfc3fb"/>
   <circle class="hl" cx="37" cy="12" r="3.5"/><circle class="hl" cx="63" cy="12" r="3.5"/>
   <circle class="tl" cx="37" cy="89" r="3.5"/><circle class="tl" cx="63" cy="89" r="3.5"/>
  </svg>
 </div>
</div>

<div class="card">
 <p class="title">DRIVE MODE</p>
 <div class="seg" id="modes">
  <button data-mode="0">Wet</button><button data-mode="1">Normal</button><button data-mode="2">Sport</button><button data-mode="3">Sport +</button>
 </div>
</div>

<div class="card">
 <div class="seg" id="views"><button data-view="stick" class="on">Joystick</button><button data-view="keys">Buttons</button></div>
 <div class="stick" id="stick"><div class="knob" id="knob"></div></div>
 <div class="keys" id="keys">
  <button data-cmd="FL">&#9700;</button><button data-cmd="F">&#9650;</button><button data-cmd="FR">&#9701;</button>
  <button data-cmd="L">&#9664;</button><button class="s" id="keyStop">STOP</button><button data-cmd="R">&#9654;</button>
  <button data-cmd="BL">&#9699;</button><button data-cmd="B">&#9660;</button><button data-cmd="BR">&#9698;</button>
 </div>
</div>

<div class="card">
 <div class="sp"><p class="title" style="margin:0">MOTOR SPEED</p><div><b id="spdVal">180</b> <small>PWM</small></div></div>
 <input type="range" id="speed" min="60" max="255" value="180">
 <button class="boost" id="bBoost">&#9889; Hold for full power</button>
</div>

<div class="card">
 <p class="title">VEHICLE STATUS</p>
 <div class="row"><span>Power</span><div class="bar"><i id="bPow"></i></div><em id="ePow">0%</em></div>
 <div class="row"><span>Mode limit</span><div class="bar"><i id="bCap"></i></div><em id="eCap">255</em></div>
 <div class="row"><span>Cooling fan</span><div class="bar"><i id="bFan"></i></div><em id="eFan">OFF</em></div>
 <div class="sub"><span>Front: <b id="eF">OFF</b> &nbsp; Rear: <b id="eR">OFF</b></span><span>Uptime: <b id="up">00:00</b></span></div>
</div>

</div>

<div class="dock">
 <button class="dk" id="bFront"><b>&#128161;</b>Front</button>
 <button class="dk" id="bRear"><b>&#128308;</b>Rear</button>
 <button class="sos" id="sos">STOP</button>
 <button class="dk" id="bFanBtn"><b>&#10052;</b>Fan</button>
</div>

<script>
var $=function(i){return document.getElementById(i)};
var caps=[120,170,215,255],names=["WET","NORMAL","SPORT","SPORT PLUS"];
var act=null,spd=180,mode=3,fanOn=false,frontOn=false,rearOn=false,boost=false,first=true;
var modeBtns=document.querySelectorAll("[data-mode]");

(function(){
 var s="";
 for(var i=0;i<28;i++){
  var a=(135+i*270/27)*Math.PI/180;
  s+='<line x1="'+(100+88*Math.cos(a))+'" y1="'+(100+88*Math.sin(a))+'" x2="'+(100+94*Math.cos(a))+'" y2="'+(100+94*Math.sin(a))+'"/>';
 }
 $("ticks").innerHTML=s;
})();

/* ---------- WebSocket ---------- */
var ws=null;
function online(ok){$("status").textContent=ok?"● Online":"● Offline";$("status").className="pill"+(ok?"":" off")}
function tx(m){if(ws&&ws.readyState===1)ws.send(m)}
function connect(){
 ws=new WebSocket("ws://"+location.hostname+":81/");
 ws.onopen=function(){online(true);tx("V:"+spd);tx("D:"+mode);if(act)tx("M:"+act)};
 ws.onclose=function(){online(false);setTimeout(connect,300)};
 ws.onerror=function(){try{ws.close()}catch(e){}};
 ws.onmessage=function(ev){
  var d;try{d=JSON.parse(ev.data)}catch(e){return}
  fanOn=d.fan;frontOn=d.front;rearOn=d.rear;mode=d.mode;
  $("clients").textContent=d.clients;
  var m=Math.floor(d.up/60),s=d.up%60;
  $("up").textContent=(m<10?"0":"")+m+":"+(s<10?"0":"")+s;
  if(first){first=false;spd=d.speed;$("speed").value=spd;$("spdVal").textContent=spd}
  render();
 };
}
connect();

function power(){return act?(boost?255:Math.min(spd,caps[mode])):0}

function render(){
 var p=power(),pct=Math.round(p/255*100);
 $("arc").setAttribute("stroke-dasharray",Math.max(p/255*358.1,.1)+" 477.5");
 $("num").textContent=pct;
 $("gear").textContent=act?(act.charAt(0)==="B"?"R":"D"):"P";
 $("mname").textContent=names[mode];
 $("bPow").style.width=pct+"%";$("ePow").textContent=pct+"%";
 $("bCap").style.width=(caps[mode]/255*100)+"%";$("eCap").textContent=caps[mode];
 $("bFan").style.width=fanOn?"100%":"0";$("eFan").textContent=fanOn?"ON":"OFF";
 $("eF").textContent=frontOn?"ON":"OFF";$("eR").textContent=rearOn?"ON":"OFF";
 $("hero").className="card"+(frontOn?" f":"")+(rearOn?" r":"");
 $("bFront").className="dk"+(frontOn?" on":"");
 $("bRear").className="dk"+(rearOn?" on":"");
 $("bFanBtn").className="dk"+(fanOn?" on":"");
 $("bBoost").className="boost"+(boost?" on":"");
 for(var i=0;i<modeBtns.length;i++)modeBtns[i].className=(+modeBtns[i].dataset.mode===mode)?"on":"";
}

/* send first, draw after */
function move(c){
 if(c===act)return;
 act=c;
 tx(c?"M:"+c:"X");
 render();
}
function stopAll(){act=null;tx("X");render()}
setInterval(function(){if(act)tx("M:"+act)},100);

var stick=$("stick"),knob=$("knob"),down=false,dirs=["R","FR","F","FL","L","BL","B","BR"];
function drag(e){
 var r=stick.getBoundingClientRect(),R=r.width/2,dx=e.clientX-r.left-R,dy=e.clientY-r.top-R,d=Math.hypot(dx,dy),k=Math.min(d,R*.6)/(d||1);
 var c=null;
 if(d>=R*.22){
  var a=Math.atan2(-dy,dx)*180/Math.PI;
  c=dirs[(Math.round(a/45)+8)%8];
 }
 move(c);
 knob.style.transform="translate("+dx*k+"px,"+dy*k+"px)";
}
stick.addEventListener("pointerdown",function(e){e.preventDefault();down=true;stick.setPointerCapture(e.pointerId);drag(e)});
stick.addEventListener("pointermove",function(e){if(down)drag(e)});
function release(){down=false;knob.style.transform="";move(null);tx("X")}
stick.addEventListener("pointerup",release);
stick.addEventListener("pointercancel",release);

document.querySelectorAll(".keys [data-cmd]").forEach(function(b){
 b.addEventListener("pointerdown",function(e){e.preventDefault();b.classList.add("act");move(b.dataset.cmd)});
 var up=function(){b.classList.remove("act");if(act===b.dataset.cmd){move(null);tx("X")}};
 b.addEventListener("pointerup",up);b.addEventListener("pointercancel",up);b.addEventListener("pointerleave",up);
});

document.querySelectorAll("[data-view]").forEach(function(b){
 b.addEventListener("click",function(){
  var k=b.dataset.view==="keys";
  $("stick").style.display=k?"none":"block";
  $("keys").style.display=k?"grid":"none";
  document.querySelectorAll("[data-view]").forEach(function(x){x.className=(x===b)?"on":""});
  stopAll();
 });
});

modeBtns.forEach(function(b){
 b.addEventListener("pointerdown",function(){mode=+b.dataset.mode;tx("D:"+mode);render()});
});

$("speed").addEventListener("input",function(){
 spd=+this.value;tx("V:"+spd);$("spdVal").textContent=spd;render();
});

["sos","keyStop"].forEach(function(id){
 $(id).addEventListener("pointerdown",function(e){e.preventDefault();knob.style.transform="";stopAll()});
});

$("bFront").addEventListener("pointerdown",function(){frontOn=!frontOn;tx("L:"+(frontOn?1:0));render()});
$("bRear").addEventListener("pointerdown",function(){rearOn=!rearOn;tx("R:"+(rearOn?1:0));render()});
$("bFanBtn").addEventListener("pointerdown",function(){fanOn=!fanOn;tx("F:"+(fanOn?1:0));render()});

function hold(id,on,off){
 var b=$(id);
 b.addEventListener("pointerdown",function(e){e.preventDefault();on()});
 ["pointerup","pointercancel","pointerleave"].forEach(function(n){b.addEventListener(n,off)});
}
hold("bBoost",function(){boost=true;tx("B:1");render()},
              function(){if(boost){boost=false;tx("B:0");render()}});

document.addEventListener("visibilitychange",function(){
 if(document.hidden){boost=false;tx("B:0");stopAll()}
});

render();
</script>
</body>
</html>
)rawliteral";

/* ---------------- Motor control ---------------- */

void setMotors(int a, int b, int c, int d, bool leftOn, bool rightOn) {
  int spd = boost ? 255 : (requestedSpeed < modeCap ? requestedSpeed : modeCap);

  digitalWrite(IN1, a);
  digitalWrite(IN2, b);
  digitalWrite(IN3, c);
  digitalWrite(IN4, d);

  ledcWrite(ENA, leftOn ? spd : 0);
  ledcWrite(ENB, rightOn ? spd : 0);
}

void stopMotor() {
  setMotors(LOW, LOW, LOW, LOW, false, false);
}

// Applies the current command to the motors right now
void applyCommand() {
  switch (currentCommand) {
    case 1: setMotors(HIGH, LOW, HIGH, LOW, true, true); break;   // forward
    case 2: setMotors(LOW, HIGH, LOW, HIGH, true, true); break;   // backward
    case 3: setMotors(LOW, HIGH, HIGH, LOW, true, true); break;   // left
    case 4: setMotors(HIGH, LOW, LOW, HIGH, true, true); break;   // right
    case 5: setMotors(LOW, LOW, HIGH, LOW, false, true); break;   // forward-left
    case 6: setMotors(HIGH, LOW, LOW, LOW, true, false); break;   // forward-right
    case 7: setMotors(LOW, LOW, LOW, HIGH, false, true); break;   // backward-left
    case 8: setMotors(LOW, HIGH, LOW, LOW, true, false); break;   // backward-right
    default: stopMotor(); break;
  }
}

/* ---------------- Status ---------------- */

void sendStatus() {
  String json = "{";
  json += "\"speed\":" + String(requestedSpeed);
  json += ",\"mode\":" + String(modeIdx);
  json += ",\"fan\":" + String(fanState ? "true" : "false");
  json += ",\"front\":" + String(frontLedState ? "true" : "false");
  json += ",\"rear\":" + String(rearLedState ? "true" : "false");
  json += ",\"clients\":" + String(WiFi.softAPgetStationNum());
  json += ",\"up\":" + String(millis() / 1000);
  json += "}";
  webSocket.broadcastTXT(json);
}

int commandFromString(const String &c) {
  if (c == "F") return 1;
  if (c == "B") return 2;
  if (c == "L") return 3;
  if (c == "R") return 4;
  if (c == "FL") return 5;
  if (c == "FR") return 6;
  if (c == "BL") return 7;
  if (c == "BR") return 8;
  return 0;
}

/* ---------------- WebSocket handler ---------------- */

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      currentCommand = 0;
      boost = false;
      stopMotor();
      break;

    case WStype_CONNECTED:
      sendStatus();
      break;

    case WStype_TEXT: {
      if (length == 0) return;
      char k = (char)payload[0];

      if (k == 'X') {                         // stop
        currentCommand = 0;
        lastCommandTime = millis();
        stopMotor();
        return;
      }

      if (length < 3 || payload[1] != ':') return;
      String v = String((char *)payload + 2);

      switch (k) {
        case 'M': {                           // move
          int cmd = commandFromString(v);
          lastCommandTime = millis();
          if (cmd != currentCommand) {        // only touch pins on change
            currentCommand = cmd;
            applyCommand();
          }
          break;
        }

        case 'V': {                           // speed
          int value = v.toInt();
          if (value < 60) value = 60;
          if (value > 255) value = 255;
          requestedSpeed = value;
          if (currentCommand) applyCommand();
          break;
        }

        case 'D': {                           // drive mode
          int m = v.toInt();
          if (m >= 0 && m <= 3) {
            modeIdx = m;
            modeCap = modeCaps[m];
            if (currentCommand) applyCommand();
          }
          break;
        }

        case 'B':                             // boost
          boost = (v == "1");
          lastCommandTime = millis();
          if (currentCommand) applyCommand();
          break;

        case 'F':
          fanState = (v == "1");
          digitalWrite(FAN, fanState ? HIGH : LOW);
          break;

        case 'L':
          frontLedState = (v == "1");
          digitalWrite(FRONT_LED1, frontLedState ? HIGH : LOW);
          digitalWrite(FRONT_LED2, frontLedState ? HIGH : LOW);
          break;

        case 'R':
          rearLedState = (v == "1");
          digitalWrite(REAR_LED1, rearLedState ? HIGH : LOW);
          digitalWrite(REAR_LED2, rearLedState ? HIGH : LOW);
          break;
      }
      break;
    }

    default:
      break;
  }
}

/* ---------------- Setup / loop ---------------- */

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(FAN, OUTPUT);
  pinMode(FRONT_LED1, OUTPUT);
  pinMode(FRONT_LED2, OUTPUT);
  pinMode(REAR_LED1, OUTPUT);
  pinMode(REAR_LED2, OUTPUT);

  digitalWrite(FAN, LOW);
  digitalWrite(FRONT_LED1, LOW);
  digitalWrite(FRONT_LED2, LOW);
  digitalWrite(REAR_LED1, LOW);
  digitalWrite(REAR_LED2, LOW);

  ledcAttach(ENA, 1000, 8);
  ledcAttach(ENB, 1000, 8);
  stopMotor();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, 1, 0, 4);   // channel 1, visible, max 4 clients
  WiFi.setSleep(false);                   // no power-save = lowest latency
  WiFi.setTxPower(WIFI_POWER_19_5dBm);

  Serial.println();
  Serial.print("WiFi: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() {
    server.send_P(200, "text/html; charset=utf-8", MAIN_page);
  });
  server.begin();

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("Server started");
}

void loop() {
  webSocket.loop();
  server.handleClient();

  // Failsafe: stop if commands stop arriving
  if (currentCommand != 0 && millis() - lastCommandTime > motorTimeout) {
    currentCommand = 0;
    stopMotor();
  }

  // Status broadcast (uptime / linked devices)
  if (millis() - lastStatusTime > 2000) {
    lastStatusTime = millis();
    sendStatus();
  }
}