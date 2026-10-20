#pragma once
#include <Arduino.h>

// Cette page est servie directement depuis la flash (pas de SPIFFS/LittleFS).
static const char WEB_TERMINAL_HTML[] PROGMEM = R"WEBHTML(<!DOCTYPE html>
<html lang="fr"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 | Console Wi-Fi</title>
<style>
:root{color-scheme:dark;font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
*{box-sizing:border-box}body{margin:0;background:#0a1020;color:#e2e8f0}
main{max-width:1050px;padding:24px;margin:auto}
header{display:flex;align-items:center;justify-content:space-between;gap:15px;flex-wrap:wrap;margin-bottom:18px}
h1{margin:0;font-size:23px;letter-spacing:-.5px}
small{display:block;margin-top:5px;color:#94a3b8}
#indicator{padding:7px 13px;border:1px solid #334155;border-radius:30px;font-size:12px;color:#fda4af}
#indicator.online{color:#86efac;border-color:#166534}
.panel{background:#111b2c;border:1px solid #29374d;border-radius:12px;overflow:hidden}
.tools{display:flex;gap:9px;padding:12px;border-bottom:1px solid #29374d;flex-wrap:wrap}
button{border:1px solid #344760;background:#1d2c43;color:#e2e8f0;border-radius:7px;padding:9px 15px;cursor:pointer;font-size:14px}
button:hover{background:#2b415e}button.primary{background:#2563eb;border-color:#2563eb}
button.primary:hover{background:#1d4ed8}
#out{margin:0;padding:18px;height:min(65vh,650px);min-height:330px;overflow:auto;white-space:pre-wrap;overflow-wrap:anywhere;font:13px/1.6 ui-monospace,SFMono-Regular,Consolas,monospace;color:#c0f0c8;background:#0b1321}
form{display:flex;gap:10px;padding:12px;border-top:1px solid #29374d}
input{min-width:0;flex:1;background:#0b1321;border:1px solid #344760;border-radius:7px;padding:10px 12px;color:#fff;font:14px ui-monospace,monospace}
footer{color:#94a3b8;font-size:13px;margin-top:12px;line-height:1.5}
</style></head><body><main>
<header><div><h1>Console ESP32</h1><small>Terminal Wi-Fi · OTA actif</small></div><span id="indicator">Connexion...</span></header>
<div class="panel">
<div class="tools"><button type="button" onclick="sendCommand('help')">Aide</button><button type="button" onclick="sendCommand('status')">Etat</button><button type="button" onclick="sendCommand('clear')">Effacer</button><button type="button" onclick="reboot()">Redemarrer</button></div>
<pre id="out">Connexion a l'ESP32...</pre>
<form id="commandForm"><input id="command" placeholder="Commande (help, status, ip, heap...)" autocomplete="off" maxlength="64" aria-label="Commande"><button class="primary" type="submit">Envoyer</button></form>
</div><footer>Les journaux sont rafraichis toutes les secondes. En cas de perte du Wi-Fi, la page tente automatiquement de se reconnecter. Les 8 derniers Kio de messages sont conserves en memoire vive.</footer>
</main><script>
'use strict';
const output=document.getElementById('out');
const indicator=document.getElementById('indicator');
const command=document.getElementById('command');
let revision=-1;
function connected(ok){indicator.className=ok?'online':'';indicator.textContent=ok?'Connecte':'Deconnecte · nouvel essai';}
async function poll(){
  try{
    const response=await fetch('/api/logs?rev='+revision,{cache:'no-store'});
    if(response.status===204){connected(true);return;}
    if(!response.ok)throw new Error('HTTP '+response.status);
    const text=await response.text();
    revision=Number(response.headers.get('X-Log-Revision') || 0);
    const atBottom=output.scrollTop+output.clientHeight>=output.scrollHeight-40;
    output.textContent=text;
    if(atBottom)output.scrollTop=output.scrollHeight;
    connected(true);
  }catch(e){connected(false);}finally{setTimeout(poll,1000);}
}
async function sendCommand(cmd){
  if(!cmd)return;
  try{
    const response=await fetch('/api/command',{method:'POST',cache:'no-store',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({cmd})});
    if(!response.ok)throw new Error('HTTP '+response.status);
    revision=-1;
  }catch(e){connected(false);}
}
function reboot(){if(confirm('Redemarrer l\'ESP32 ?'))sendCommand('restart');}
document.getElementById('commandForm').addEventListener('submit',e=>{e.preventDefault();const cmd=command.value.trim();command.value='';sendCommand(cmd);});
command.focus();poll();
</script></body></html>)WEBHTML";
