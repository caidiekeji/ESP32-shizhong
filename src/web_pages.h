#pragma once
// 浏览器时钟页面（PROGMEM 内嵌，无需上传文件系统）
#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 智能时钟</title>
<style>
:root{
  --bg1:#0b1026; --bg2:#1a2456; --orb1:#5b6cff; --orb2:#9333ea; --orb3:#0ea5e9;
  --glass:rgba(255,255,255,.07); --border:rgba(255,255,255,.14);
  --text:#f1f5ff; --muted:#94a3c8; --accent:#7dd3fc;
}
body.day{
  --bg1:#87b7f0; --bg2:#dfe9f3; --orb1:#fbbf24; --orb2:#60a5fa; --orb3:#f472b6;
  --glass:rgba(255,255,255,.45); --border:rgba(255,255,255,.6);
  --text:#16233f; --muted:#3b4a68; --accent:#0284c7;
}
*{margin:0;padding:0;box-sizing:border-box}
html,body{height:100%}
body{
  font-family:"Segoe UI","PingFang SC","Microsoft YaHei",system-ui,sans-serif;
  background:linear-gradient(160deg,var(--bg1),var(--bg2));
  color:var(--text); min-height:100vh; overflow-x:hidden;
  transition:background 1.5s ease;
}
.bg-orb{position:fixed;border-radius:50%;filter:blur(110px);opacity:.5;pointer-events:none;z-index:0;animation:float 16s ease-in-out infinite}
.orb1{width:44vw;height:44vw;background:var(--orb1);top:-12vw;left:-10vw}
.orb2{width:36vw;height:36vw;background:var(--orb2);bottom:-8vw;right:-6vw;animation-delay:-6s}
.orb3{width:26vw;height:26vw;background:var(--orb3);top:40%;left:55%;animation-delay:-11s;opacity:.35}
@keyframes float{0%,100%{transform:translate(0,0) scale(1)}50%{transform:translate(4vw,-3vw) scale(1.08)}}
.app{position:relative;z-index:1;max-width:980px;margin:0 auto;padding:28px 22px 40px}

.topbar{display:flex;justify-content:space-between;align-items:center;margin-bottom:22px;flex-wrap:wrap;gap:10px}
.brand{display:flex;align-items:center;gap:10px;font-size:15px;letter-spacing:.12em;color:var(--muted)}
.dot{width:9px;height:9px;border-radius:50%;background:#f87171;transition:background .4s}
.dot.on{background:#4ade80;box-shadow:0 0 10px #4ade80;animation:pulse 2s infinite}
@keyframes pulse{50%{opacity:.5}}
.meta{font-size:13px;color:var(--muted)}

.glass{background:var(--glass);border:1px solid var(--border);border-radius:26px;
  backdrop-filter:blur(22px);-webkit-backdrop-filter:blur(22px);
  box-shadow:0 12px 40px rgba(0,0,0,.18)}

.clock-card{text-align:center;padding:44px 20px 36px;margin-bottom:20px}
.clock{font-size:clamp(72px,15vw,168px);font-weight:200;line-height:1;
  font-variant-numeric:tabular-nums;letter-spacing:.02em;text-shadow:0 4px 30px rgba(0,0,0,.25)}
.clock .colon{animation:blink 2s step-end infinite}
@keyframes blink{50%{opacity:.25}}
.clock .sec{font-size:.28em;font-weight:400;color:var(--muted);margin-left:.15em;vertical-align:.9em}
.date-line{margin-top:18px;font-size:clamp(15px,2.4vw,20px);letter-spacing:.35em;color:var(--muted)}

.grid{display:grid;grid-template-columns:1.25fr 1fr;gap:20px;margin-bottom:20px}
@media(max-width:720px){.grid{grid-template-columns:1fr}}

.card{padding:24px 26px}
.card h3{font-size:13px;font-weight:600;letter-spacing:.25em;color:var(--muted);margin-bottom:16px}

.wx-main{display:flex;align-items:center;gap:22px}
.wx-icon{font-size:64px;line-height:1;filter:drop-shadow(0 4px 12px rgba(0,0,0,.25))}
.wx-temp{font-size:52px;font-weight:300;font-variant-numeric:tabular-nums}
.wx-temp small{font-size:.4em;color:var(--muted);font-weight:400}
.wx-text{font-size:19px;margin:4px 0 6px}
.wx-sub{font-size:13.5px;color:var(--muted);line-height:1.7}

.metrics{display:flex;justify-content:space-between;text-align:center;gap:8px}
.metric{flex:1}
.metric .val{font-size:clamp(26px,4vw,34px);font-weight:300;font-variant-numeric:tabular-nums}
.metric .lab{font-size:12px;color:var(--muted);margin-top:6px;letter-spacing:.1em}

.fc-row{display:grid;grid-template-columns:repeat(3,1fr);gap:12px;text-align:center}
.fc-day{padding:14px 6px;border-radius:16px;background:rgba(255,255,255,.05)}
body.day .fc-day{background:rgba(255,255,255,.35)}
.fc-name{font-size:13px;color:var(--muted);margin-bottom:8px}
.fc-icon{font-size:30px;margin-bottom:8px}
.fc-text{font-size:13.5px;margin-bottom:8px}
.fc-temp{font-size:16px;font-variant-numeric:tabular-nums}
.fc-temp b{font-weight:600}
.fc-temp span{color:var(--muted)}

.statusbar{display:flex;justify-content:space-between;align-items:center;gap:12px;
  padding:16px 22px;font-size:13px;color:var(--muted);flex-wrap:wrap}
.city-form{display:flex;gap:8px;align-items:center}
.city-form input{background:rgba(255,255,255,.1);border:1px solid var(--border);color:var(--text);
  border-radius:10px;padding:7px 12px;font-size:13px;width:110px;outline:none}
body.day .city-form input{background:rgba(255,255,255,.55)}
.city-form input::placeholder{color:var(--muted)}
.city-form button{background:var(--accent);color:#08101f;border:none;border-radius:10px;
  padding:8px 16px;font-size:13px;font-weight:600;cursor:pointer;transition:opacity .2s}
.city-form button:hover{opacity:.85}
</style>
</head>
<body>
<div class="bg-orb orb1"></div><div class="bg-orb orb2"></div><div class="bg-orb orb3"></div>
<main class="app">
  <header class="topbar">
    <div class="brand"><span class="dot" id="dot"></span> ESP32 智能时钟</div>
    <div class="meta"><span id="city">--</span> · 天气更新于 <span id="wx-time">--</span></div>
  </header>

  <section class="clock-card glass">
    <div class="clock"><span id="hh">--</span><span class="colon">:</span><span id="mm">--</span><span class="sec" id="ss">--</span></div>
    <div class="date-line" id="date">正在同步时间…</div>
  </section>

  <div class="grid">
    <section class="glass card wx-main">
      <div class="wx-icon" id="wx-icon">🌤️</div>
      <div class="wx-info">
        <div class="wx-temp"><span id="wx-temp">--</span><small>°C</small></div>
        <div class="wx-text" id="wx-text">等待天气数据…</div>
        <div class="wx-sub"><span id="wx-feels"></span> · <span id="wx-wind"></span> · 湿度 <span id="wx-hum">--</span>%</div>
      </div>
    </section>
    <section class="glass card">
      <h3>室内环境</h3>
      <div class="metrics">
        <div class="metric"><div class="val" id="in-t">--</div><div class="lab">温度 ℃</div></div>
        <div class="metric"><div class="val" id="in-h">--</div><div class="lab">湿度 %</div></div>
        <div class="metric"><div class="val" id="in-p">--</div><div class="lab">气压 hPa</div></div>
      </div>
    </section>
  </div>

  <section class="glass card" style="margin-bottom:20px">
    <h3>未来三天</h3>
    <div class="fc-row" id="fc"><div class="fc-name">等待数据…</div></div>
  </section>

  <footer class="glass statusbar">
    <span id="status">连接中…</span>
    <span class="city-form">
      <input id="cityInput" placeholder="城市，如 上海" maxlength="20">
      <button onclick="setCity('auto')">自动</button>
      <button onclick="setCity()">设置</button>
    </span>
  </footer>
</main>

<script>
const $=id=>document.getElementById(id);
const pad=n=>n<10?'0'+n:''+n;
let off=0;

function render(){
  const d=new Date(Date.now()+off);
  $('hh').textContent=pad(d.getHours());
  $('mm').textContent=pad(d.getMinutes());
  $('ss').textContent=pad(d.getSeconds());
  $('date').textContent=d.getFullYear()+'年'+(d.getMonth()+1)+'月'+d.getDate()+'日 星期'+'日一二三四五六'[d.getDay()];
  const h=d.getHours();
  document.body.classList.toggle('day',h>=6&&h<18);
}
setInterval(render,1000);render();

function wxEmoji(c){
  c=+c;
  if(c===100||c===150)return '☀️';
  if(c===101||c===102||c===103)return '⛅';
  if(c===104)return '☁️';
  if(c>=300&&c<399)return (c===302||c===303||c===304)?'⛈️':'🌧️';
  if(c>=400&&c<499)return '🌨️';
  if(c>=500&&c<516)return '🌫️';
  if(c===507||c===508)return '🌬️';
  return '🌤️';
}

function fmtWxTime(ts){
  const d=new Date(ts*1000);
  return pad(d.getHours())+':'+pad(d.getMinutes());
}

function renderFc(fc){
  if(!fc||!fc.length){$('fc').innerHTML='<div class="fc-name">暂无预报</div>';return}
  const names=['明天','后天','大后天'];
  $('fc').innerHTML=fc.map((f,i)=>{
    let name=names[i]||f.date;
    const wd=new Date(f.date+'T00:00:00');
    if(i>2||isNaN(wd))name=f.date;
    return '<div class="fc-day"><div class="fc-name">'+name+'</div>'+
      '<div class="fc-icon">'+wxEmoji(f.icon)+'</div>'+
      '<div class="fc-text">'+f.text+'</div>'+
      '<div class="fc-temp"><b>'+f.max+'°</b> <span>'+f.min+'°</span></div></div>';
  }).join('');
}

async function poll(){
  try{
    const r=await fetch('/api/data',{cache:'no-store'});
    const d=await r.json();
    off=Math.round(d.time*1000)-Date.now();
    $('city').textContent=d.city||'--';

    const wx=d.wx||{};
    if(wx.valid){
      $('wx-icon').textContent=wxEmoji(wx.icon);
      $('wx-temp').textContent=wx.temp;
      $('wx-text').textContent=wx.text;
      $('wx-feels').textContent='体感 '+wx.feels+'°C';
      $('wx-wind').textContent=wx.wind;
      $('wx-hum').textContent=wx.hum;
      $('wx-time').textContent=fmtWxTime(wx.updated);
      $('city').textContent=(d.city||'')+' · '+wx.text;
    }else{
      $('wx-text').textContent='天气获取中…';
    }
    renderFc(d.fc);

    const ind=d.indoor||{};
    $('in-t').textContent=ind.t!=null&&!isNaN(ind.t)?ind.t.toFixed(1):'--';
    $('in-h').textContent=ind.h!=null&&!isNaN(ind.h)?ind.h.toFixed(0):'--';
    $('in-p').textContent=ind.p!=null&&!isNaN(ind.p)?ind.p.toFixed(0):'--';

    const s=d.sys||{};
    $('status').textContent='已连接 · '+(d.autoLoc?'IP自动定位 · ':'')+'信号 '+s.rssi+'dBm · 内存 '+(s.heap/1024).toFixed(0)+'KB · 运行 '+(s.up/3600).toFixed(1)+'h';
    $('dot').classList.add('on');
  }catch(e){
    $('dot').classList.remove('on');
    $('status').textContent='连接断开，自动重试中…';
  }
}
setInterval(poll,5000);poll();

async function setCity(v){
  const val=v||$('cityInput').value.trim();
  if(!val)return;
  $('cityInput').value='';
  try{await fetch('/api/setcity?city='+encodeURIComponent(val));poll()}catch(e){}
}
$('cityInput').addEventListener('keydown',e=>{if(e.key==='Enter')setCity()});
</script>
</body>
</html>)HTML";
