#include "portal.h"
#include "config.h"
#include "display.h"
#include "led.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

static WebServer *s_web = nullptr;
static DNSServer *s_dns = nullptr;
static Preferences s_prefs;
static volatile unsigned long s_exitAt = 0;   // 配网成功后延迟关门户（让响应发完）

// ---------------- 配网页面（与时钟主页同款风格） ----------------
static const char PORTAL_HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="zh-CN"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>ESP32 时钟 · 配网</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{min-height:100vh;display:flex;align-items:center;justify-content:center;
  font-family:"PingFang SC","Microsoft YaHei",system-ui,sans-serif;
  background:linear-gradient(160deg,#0b1526 0%,#101f38 55%,#0a1120 100%);color:#e8eefc;padding:20px}
.card{width:100%;max-width:420px;background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.12);
  border-radius:20px;padding:28px 24px;backdrop-filter:blur(14px);box-shadow:0 20px 60px rgba(0,0,0,.45)}
h1{font-size:22px;font-weight:600;text-align:center;letter-spacing:1px}
h1 span{font-size:26px}
.sub{text-align:center;color:rgba(232,238,252,.55);font-size:13px;margin:8px 0 20px}
.bar{display:flex;gap:10px;margin-bottom:14px}
.bar input{flex:1;background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.14);border-radius:12px;
  padding:12px 14px;color:#e8eefc;font-size:15px;outline:none}
.bar input:focus{border-color:#4da3ff}
.bar button,.big{border:none;border-radius:12px;padding:12px 18px;font-size:15px;font-weight:600;cursor:pointer;
  background:linear-gradient(135deg,#4da3ff,#3b82f6);color:#06121f;white-space:nowrap}
.bar button:active,.big:active{opacity:.8}
.big{width:100%;padding:14px;margin-top:6px}
#nets{max-height:230px;overflow-y:auto;border:1px solid rgba(255,255,255,.1);border-radius:14px;margin-bottom:14px}
.net{display:flex;justify-content:space-between;align-items:center;padding:12px 14px;cursor:pointer;
  border-bottom:1px solid rgba(255,255,255,.06);font-size:15px}
.net:last-child{border-bottom:none}
.net:hover{background:rgba(77,163,255,.12)}
.net.sel{background:rgba(77,163,255,.22)}
.net .r{color:rgba(232,238,252,.5);font-size:12px;margin-left:10px}
#pw{display:none;margin-bottom:14px}
#pw input{width:100%;background:rgba(255,255,255,.08);border:1px solid rgba(255,255,255,.14);border-radius:12px;
  padding:13px 14px;color:#e8eefc;font-size:15px;outline:none}
.plabel{display:flex;align-items:center;gap:6px;color:rgba(232,238,252,.6);font-size:13px;margin-top:8px}
#msg{text-align:center;font-size:14px;margin-top:14px;min-height:20px;color:#8ec5ff}
#msg.err{color:#ff8e8e}
#msg.ok{color:#7ce8a4}
.spin{display:inline-block;width:14px;height:14px;border:2px solid rgba(142,197,255,.3);
  border-top-color:#8ec5ff;border-radius:50%;animation:r 0.8s linear infinite;vertical-align:-2px;margin-right:6px}
@keyframes r{to{transform:rotate(360deg)}}
.tip{text-align:center;color:rgba(232,238,252,.35);font-size:12px;margin-top:18px}
</style></head><body>
<div class="card">
  <h1><span>&#127470;&#127475;</span> ESP32 智能时钟</h1>
  <div class="sub">首次使用 · 选择家中 WiFi 完成配网</div>
  <div class="bar">
    <input id="ssid" placeholder="也可直接输入 WiFi 名称">
    <button onclick="scan()">扫描</button>
  </div>
  <div id="nets"></div>
  <div id="pw">
    <input id="pass" type="password" placeholder="输入所选 WiFi 的密码">
    <label class="plabel"><input type="checkbox" onclick="var p=document.getElementById('pass');p.type=this.checked?'text':'password'"> 显示密码</label>
  </div>
  <button class="big" id="go" onclick="save()">连接网络</button>
  <div id="msg"></div>
  <div class="tip">热点：ESP32-Clock-Setup　密码：12345678</div>
</div>
<script>
var sel='';
function $(i){return document.getElementById(i)}
function scan(){
  $('msg').className='';$('msg').innerHTML='<span class="spin"></span>正在扫描附近的 WiFi…';
  fetch('/scan').then(r=>r.json()).then(d=>{
    if(d.scanning){setTimeout(scan,1200);return}
    var seen={},html='';
    (d.nets||[]).forEach(function(n){
      if(!n.ssid||seen[n.ssid])return;seen[n.ssid]=1;
      var bar=n.rssi>-55?'▂▄▆█':n.rssi>-65?'▂▄▆_':n.rssi>-75?'▂▄__':'▂___';
      html+='<div class="net" onclick="pick(this,\''+n.ssid.replace(/'/g,"\\'")+'\')"><span>'+n.ssid+'</span><span class="r">'+bar+' '+n.rssi+'dBm</span></div>';
    });
    $('nets').innerHTML=html||'<div class="net">未发现网络，点「扫描」重试</div>';
    $('msg').textContent='';
  }).catch(function(){$('msg').className='err';$('msg').textContent='扫描失败，请重试'});
}
function pick(el,ssid){
  document.querySelectorAll('.net').forEach(function(x){x.classList.remove('sel')});
  el.classList.add('sel');sel=ssid;$('ssid').value=ssid;$('pw').style.display='block';$('pass').focus();
}
function save(){
  var ssid=$('ssid').value.trim(),pass=$('pass').value;
  if(!ssid){$('msg').className='err';$('msg').textContent='请选择或输入 WiFi 名称';return}
  $('go').disabled=true;
  $('msg').className='';$('msg').innerHTML='<span class="spin"></span>正在连接，请稍候（约 20 秒内）…';
  var body='ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass);
  fetch('/save',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body})
    .then(r=>r.json()).then(d=>{
      if(d.ok){
        $('msg').className='ok';
        $('msg').textContent='连接成功！设备重启中，之后用浏览器访问 http://'+d.ip;
      }else{
        $('go').disabled=false;
        $('msg').className='err';$('msg').textContent='连接失败：'+(d.msg||'请检查密码');
      }
    }).catch(function(){$('go').disabled=false;$('msg').className='err';$('msg').textContent='网络异常，请重试'});
}
scan();
</script></body></html>)html";

// ---------------- 门户内部 ----------------
static void startPortal();
static void teardownPortal();

static void handleScan() {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING || n == WIFI_SCAN_FAILED) {
        if (n == WIFI_SCAN_FAILED) WiFi.scanNetworks(true);
        s_web->send(200, "application/json", "{\"scanning\":true}");
        return;
    }
    JsonDocument doc;
    JsonArray arr = doc["nets"].to<JsonArray>();
    for (int i = 0; i < n && i < 30; i++) {
        String ssid = WiFi.SSID(i);
        if (!ssid.length()) continue;
        JsonObject o = arr.add<JsonObject>();
        o["ssid"] = ssid;
        o["rssi"] = WiFi.RSSI(i);
    }
    WiFi.scanDelete();
    WiFi.scanNetworks(true);   // 继续后台补扫
    String out;
    serializeJson(doc, out);
    s_web->send(200, "application/json", out);
}

static void handleSave() {
    String ssid = s_web->arg("ssid");
    String pass = s_web->arg("pass");
    ssid.trim();
    JsonDocument doc;
    String out;
    if (!ssid.length()) {
        doc["ok"] = false; doc["msg"] = "WiFi 名称不能为空";
        serializeJson(doc, out); s_web->send(200, "application/json", out);
        return;
    }
    Serial.printf("[配网] 尝试连接: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) delay(100);

    if (WiFi.status() == WL_CONNECTED) {
        s_prefs.begin("clock", false);
        s_prefs.putString("ssid", ssid);
        s_prefs.putString("pass", pass);
        s_prefs.end();
        doc["ok"] = true;
        doc["ip"] = WiFi.localIP().toString();
        serializeJson(doc, out);
        s_web->send(200, "application/json", out);
        Serial.printf("[配网] 成功！IP: %s，即将退出配网\n", WiFi.localIP().toString().c_str());
        s_exitAt = millis() + 1500;    // 留时间把响应发完
    } else {
        wl_status_t st = WiFi.status();
        String msg = "密码可能不正确";
        if (st == WL_NO_SSID_AVAIL)      msg = "找不到该网络（信号太弱？）";
        else if (st == WL_CONNECT_FAILED) msg = "连接被拒绝";
        WiFi.disconnect();
        WiFi.scanNetworks(true);
        doc["ok"] = false; doc["msg"] = msg;
        serializeJson(doc, out);
        s_web->send(200, "application/json", out);
        Serial.printf("[配网] 失败: %s (%d)\n", msg.c_str(), st);
    }
}

static void handleRoot() {
    s_web->send_P(200, "text/html", PORTAL_HTML);
}

static void teardownPortal() {
    if (s_dns) { s_dns->stop(); delete s_dns; s_dns = nullptr; }
    if (s_web) { s_web->stop(); delete s_web; s_web = nullptr; }
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.reconnect();
}

static void startPortal() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.setHostname(HOSTNAME);
    WiFi.softAP(WIFI_AP_NAME, WIFI_AP_PASSWORD);
    IPAddress ip = WiFi.softAPIP();

    s_dns = new DNSServer();
    s_dns->start(53, "*", ip);            // 劫持所有域名 → 弹出配网页

    s_web = new WebServer(80);
    s_web->on("/", handleRoot);
    s_web->on("/scan", handleScan);
    s_web->on("/save", HTTP_POST, handleSave);
    s_web->onNotFound([]() {              // 其余全部重定向（ captive portal 探测路径）
        s_web->sendHeader("Location", String("http://") + ip.toString(), true);
        s_web->send(302, "text/plain", "");
    });
    s_web->begin();
    WiFi.scanNetworks(true);

    Serial.printf("[配网] 热点 %s（密码 %s）已开启，配网页 http://%s\n",
                  WIFI_AP_NAME, WIFI_AP_PASSWORD, ip.toString().c_str());
    displayShowMsg("等待配网", "热点 ESP32-Clock-Setup");

    for (;;) {
        s_dns->processNextRequest();
        s_web->handleClient();
        ledLoop();                        // 配网中红灯慢闪
        if (s_exitAt && millis() > s_exitAt) {
            teardownPortal();
            return;
        }
        delay(2);
    }
}

bool portalEnsureConnected() {
    s_prefs.begin("clock", true);
    String ssid = s_prefs.getString("ssid", "");
    String pass = s_prefs.getString("pass", "");
    s_prefs.end();

    if (ssid.length()) {
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(HOSTNAME);
        WiFi.begin(ssid.c_str(), pass.c_str());
        Serial.printf("[WiFi] 使用已保存凭证连接: %s\n", ssid.c_str());
        unsigned long t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) delay(100);
        if (WiFi.status() == WL_CONNECTED) return true;
        Serial.println("[WiFi] 连接失败，进入配网门户");
    }
    startPortal();
    return WiFi.status() == WL_CONNECTED;
}
