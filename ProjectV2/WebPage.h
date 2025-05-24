// WebPage.h
#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

// 主頁面 HTML（包含 CSS/JS）並保留 %HISTORY% 佔位符
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>電子衣櫃管理</title>
  <style>
    body { font-family: sans-serif; text-align: center; }
    table { margin: auto; }
  </style>
</head>
<body>
  <h1>電子衣櫃系統</h1>
  <button onclick="startScan()">掃描新卡</button>
  <button id="toggleViewBtn" onclick="toggleView()">切換顯示模式：顯示虛擬衣櫃</button>
  <div id="contentView"></div>

  <h2>未來數小時天氣預報及穿搭建議</h2>
  <div id="recommendation"></div>

  <h2>過去穿衣紀錄</h2>
  <div id="history">%HISTORY%</div>

  <script>
    let viewMode = 'list';
    function toggleView(){
      if(viewMode === 'list'){
        viewMode = 'grid';
        document.getElementById('toggleViewBtn').innerText = '切換顯示模式：顯示所有衣物';
      } else {
        viewMode = 'list';
        document.getElementById('toggleViewBtn').innerText = '切換顯示模式：顯示虛擬衣櫃';
      }
      getCards();
    }

    function getCards(){
      fetch('/cards').then(r=>r.json()).then(data=>{
        viewMode==='list'?renderListView(data):renderGridView(data);
      });
    }

    function renderListView(data){
      let grouped = {};
      data.forEach(card => {
        let color = (card.features&&card.features.color)?card.features.color:'未知顏色';
        grouped[color] = grouped[color]||[];
        grouped[color].push(card);
      });
      let html = '';
      for(let color in grouped){
        html += `<h3>${color}</h3><ul>`;
        grouped[color].forEach(card=>{
          let style = (card.features&&card.features.style)?card.features.style:'未知樣式';
          let pos = (card.features&&card.features.position)?card.features.position:'未設定位置';
          html += `<li>${style} (位置: ${pos})
            <button onclick="onRename('${card.uid}','${style}','${color}','${pos}')">修改</button>
            <button onclick="onDelete('${card.uid}')">刪除</button>
          </li>`;
        });
        html += '</ul>';
      }
      document.getElementById('contentView').innerHTML = html;
    }

    function renderGridView(data){
      let posArr=['A1','A2','B1','B2'];
      let grid='<table border="1" style="width:50%;text-align:center;"><tr>';
      for(let i=0;i<2;i++){
        let p=posArr[i]; let cnt=data.filter(c=>c.features&&c.features.position===p).length;
        grid+=`<td>${p}<br><button onclick="showSpaceItems('${p}')">查看(${cnt}件)</button></td>`;
      }
      grid+='</tr><tr>';
      for(let i=2;i<4;i++){
        let p=posArr[i]; let cnt=data.filter(c=>c.features&&c.features.position===p).length;
        grid+=`<td>${p}<br><button onclick="showSpaceItems('${p}')">查看(${cnt}件)</button></td>`;
      }
      grid+='</tr></table>';
      document.getElementById('contentView').innerHTML = grid;
    }

    function showSpaceItems(pos){
      fetch('/cards').then(r=>r.json()).then(data=>{
        let items=data.filter(c=>c.features&&c.features.position===pos);
        if(!items.length) alert(pos+' 空無衣物');
        else{
          let w=window.open('','','width=400,height=300');
          w.document.write(`<h3>${pos} 的衣物清單</h3><table border="1"><tr><th>顏色</th><th>樣式</th></tr>`);
          items.forEach(it=>{
            let st=(it.features&&it.features.style)?it.features.style:'未知樣式';
            let col=(it.features&&it.features.color)?it.features.color:'未知顏色';
            w.document.write(`<tr><td>${col}</td><td>${st}</td></tr>`);
          });
          w.document.write('</table>'); w.document.close();
        }
      });
    }

    function startScan(){ fetch('/startScan',{method:'POST'}).then(r=>r.text()).then(t=>alert(t)); }

    setInterval(()=>{
      fetch('/scanStatus').then(r=>r.json()).then(d=>{
        if(d.pendingUID){
          if(d.pendingExists) alert('此卡已登記 (UID='+d.pendingUID+')，如需修改可在清單中操作');
          else{
            alert('讀到新卡 UID: '+d.pendingUID+'，將儲存至資料庫');
            let style=prompt('輸入衣服樣式','');
            let color=prompt('輸入衣服顏色','');
            let pos=prompt('輸入衣櫃位置','');
            fetch('/cards',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({uid:d.pendingUID,features:{style, color, position:pos}})}).then(_=>getCards());
          }
        }
      });
    },1000);

    function onRename(uid,os,oc,op){
      let ns=prompt('目前樣式:'+os,os)||os;
      let nc=prompt('目前顏色:'+oc,oc)||oc;
      let np=prompt('目前位置:'+op,op)||op;
      fetch('/cards?uid='+uid,{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify({features:{style:ns,color:nc,position:np}})}).then(_=>getCards());
    }
    function onDelete(uid){ if(confirm('確定刪除?')) fetch('/cards?uid='+uid,{method:'DELETE'}).then(_=>getCards()); }

    function getRecommend(){ fetch('/recommend').then(r=>r.text()).then(h=>document.getElementById('recommendation').innerHTML=h); }

    window.onload=function(){ getRecommend(); getCards(); };
  </script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
