// WebPage.h
#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-Hant">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>智慧衣櫃管理系統</title>
  <link href="https://fonts.googleapis.com/css2?family=Noto+Sans+TC:wght@400;500;700&display=swap" rel="stylesheet">
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.1/dist/chart.umd.min.js"></script>
  <style>
    :root {
      --black: #000000;
      --navy: #1B264F;
      --orange: #FF9F29;
      --light-gray: #F5F5F5;
      
      --primary: var(--navy);
      --secondary: var(--orange);
      --background: var(--light-gray);
      --text: var(--black);
      --border: #dcdde1;
      --error: #e74c3c;
      --success: #2ecc71;
    }

    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }

    body {
      font-family: 'Noto Sans TC', sans-serif;
      background: var(--background);
      color: var(--text);
      line-height: 1.6;
      padding: 20px;
    }

    .container {
      max-width: 1200px;
      margin: 0 auto;
    }

    .header {
      text-align: center;
      margin-bottom: 2rem;
      padding: 1rem;
      background: white;
      border-radius: 10px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }

    h1, h2, h3 {
      color: var(--primary);
      margin-bottom: 1rem;
    }

    .stats-container {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 1rem;
      margin-bottom: 1rem;
    }

    .stat-card {
      background: white;
      padding: 1rem;
      border-radius: 8px;
      text-align: center;
      box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }

    .stat-number {
      font-size: 2rem;
      font-weight: bold;
      color: var(--primary);
    }

    .stat-label {
      color: var(--text);
      font-size: 0.9rem;
    }

    .button-group {
      display: flex;
      gap: 1rem;
      justify-content: center;
      margin-bottom: 2rem;
      flex-wrap: wrap;
    }

    button {
      background: var(--primary);
      color: white;
      border: none;
      padding: 10px 20px;
      border-radius: 5px;
      cursor: pointer;
      transition: all 0.3s ease;
      font-size: 1rem;
    }

    button:hover {
      background: #357abd;
      transform: translateY(-2px);
    }

    button.secondary {
      background: var(--secondary);
    }

    button.danger {
      background: var(--error);
    }

    .grid {
      display: flex;
      flex-direction: column;
      gap: 1.5rem;
      margin-bottom: 2rem;
    }

    .card {
      background: white;
      border-radius: 10px;
      padding: 1.5rem;
      box-shadow: 0 4px 6px rgba(0, 0, 0, 0.1);
      width: 100%;
      transition: transform 0.3s ease, box-shadow 0.3s ease;
    }

    .card:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 12px rgba(0, 0, 0, 0.15);
    }

    .weather-card {
      background: linear-gradient(135deg, var(--navy), var(--orange));
      color: white;
    }

    .weather-card h2 {
      color: white;
    }

    table {
      width: 100%;
      border-collapse: collapse;
      margin: 1rem 0;
      background: white;
      border-radius: 10px;
      overflow: hidden;
      font-size: 0.9rem;
      box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
      transition: box-shadow 0.3s ease;
    }

    table:hover {
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
    }

    th, td {
      padding: 8px 12px;
      text-align: left;
      border: 1px solid var(--border);
      min-width: 80px;
    }

    .weather-card table {
      background: transparent;
      color: white;
      table-layout: fixed;
      width: 100%;
    }

    .weather-card th {
      background: rgba(0, 0, 0, 0.2);
      color: white;
      font-weight: 500;
      white-space: nowrap;
      font-size: 0.85rem;
      padding: 6px 8px;
    }

    .weather-card td {
      border-color: rgba(255, 255, 255, 0.2);
      white-space: normal;
      font-size: 0.85rem;
      padding: 6px 8px;
    }

    .weather-card tr:hover {
      background: rgba(255, 255, 255, 0.1);
    }

    .weather-card th:first-child,
    .weather-card td:first-child {
      width: 30%;
    }

    .weather-card th:not(:first-child),
    .weather-card td:not(:first-child) {
      width: 14%;
      text-align: center;
    }

    .clothes-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 1rem;
    }

    .clothes-space {
      background: white;
      padding: 1.5rem;
      border-radius: 10px;
      text-align: center;
      cursor: pointer;
      transition: all 0.3s ease;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
    }

    .clothes-space:hover {
      transform: translateY(-5px);
      box-shadow: 0 5px 15px rgba(0,0,0,0.1);
    }

    .space-icon {
      font-size: 2rem;
      margin-bottom: 0.5rem;
    }

    .modal {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: rgba(0,0,0,0.5);
      justify-content: center;
      align-items: center;
      z-index: 1000;
    }

    .modal-content {
      background: white;
      padding: 2rem;
      border-radius: 12px;
      max-width: 90%;
      width: 1000px;
      max-height: 90vh;
      overflow-y: auto;
      box-shadow: 0 10px 25px rgba(0, 0, 0, 0.2);
    }

    .modal-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 1rem;
      padding-bottom: 1rem;
      border-bottom: 1px solid var(--border);
    }

    .modal-filters {
      display: flex;
      gap: 1rem;
      margin-bottom: 1rem;
      flex-wrap: wrap;
    }

    .modal-filters select {
      padding: 0.5rem;
      border: 1px solid var(--border);
      border-radius: 5px;
      min-width: 150px;
    }

    .modal-close {
      background: none;
      border: none;
      font-size: 1.5rem;
      cursor: pointer;
      padding: 0.5rem;
    }

    .modal-close:hover {
      opacity: 0.7;
    }

    .chart-container {
      height: 300px;
      margin: 1rem 0;
    }

    .history-item {
      background: white;
      padding: 1rem;
      border-radius: 8px;
      margin-bottom: 1rem;
      box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }

    .history-date {
      color: var(--primary);
      font-weight: bold;
    }

    .history-details {
      margin-top: 0.5rem;
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 0.5rem;
    }

    .history-detail {
      padding: 0.5rem;
      background: #f8f9fa;
      border-radius: 4px;
    }

    @media (max-width: 768px) {
      .grid {
        flex-direction: column;
      }
      
      .button-group {
        flex-direction: column;
      }
      
      .clothes-grid {
        grid-template-columns: 1fr;
      }

      .stats-container {
        grid-template-columns: repeat(2, 1fr);
      }

      .weather-card {
        overflow-x: auto;
        padding: 1rem;
      }
      
      .weather-card table {
        min-width: auto;
        width: 100%;
      }
      
      .weather-card th,
      .weather-card td {
        font-size: 0.8rem;
        padding: 4px 6px;
      }

      .weather-card th:first-child,
      .weather-card td:first-child {
        width: auto;
        min-width: 120px;
      }
    }

    .notification {
      position: fixed;
      top: 20px;
      right: 20px;
      padding: 1rem 1.5rem;
      border-radius: 8px;
      color: white;
      font-weight: 500;
      opacity: 0;
      transform: translateX(100%);
      transition: all 0.3s ease;
      z-index: 1000;
    }

    .notification.show {
      opacity: 1;
      transform: translateX(0);
    }

    .notification.success {
      background: var(--success);
    }

    .notification.error {
      background: var(--error);
    }

    .notification.info {
      background: var(--primary);
    }

    .loading-overlay {
      position: fixed;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background: rgba(255, 255, 255, 0.8);
      display: flex;
      justify-content: center;
      align-items: center;
      z-index: 1000;
    }

    .loading-spinner {
      width: 50px;
      height: 50px;
      border: 4px solid var(--border);
      border-top-color: var(--primary);
      border-radius: 50%;
      animation: spin 1s linear infinite;
    }

    @keyframes spin {
      to {
        transform: rotate(360deg);
      }
    }

    .empty-state {
      text-align: center;
      padding: 2rem;
      color: #666;
    }

    .empty-state-icon {
      font-size: 3rem;
      margin-bottom: 1rem;
    }

    .filter-container {
      display: flex;
      gap: 1rem;
      margin-bottom: 1rem;
      flex-wrap: wrap;
    }

    .filter-container select {
      padding: 0.5rem;
      border: 1px solid var(--border);
      border-radius: 5px;
      background: white;
      min-width: 150px;
    }

    .edit-btn {
      background: var(--primary);
      color: white;
      border: none;
      padding: 4px 8px;
      border-radius: 4px;
      cursor: pointer;
      margin-right: 4px;
    }

    .delete-btn {
      background: var(--error);
      color: white;
      border: none;
      padding: 4px 8px;
      border-radius: 4px;
      cursor: pointer;
    }

    .edit-btn:hover, .delete-btn:hover {
      opacity: 0.9;
    }

    .list-filters {
      display: flex;
      gap: 1rem;
      margin-bottom: 1.5rem;
      flex-wrap: wrap;
    }

    .list-filters select {
      padding: 8px 12px;
      border: 1px solid var(--border);
      border-radius: 6px;
      background: white;
      min-width: 150px;
      cursor: pointer;
      transition: all 0.3s ease;
    }

    .list-filters select:hover {
      border-color: var(--primary);
    }

    .list-filters select:focus {
      outline: none;
      border-color: var(--primary);
      box-shadow: 0 0 0 2px rgba(27, 38, 79, 0.1);
    }

    .no-results {
      text-align: center;
      padding: 2rem;
      color: var(--text);
      background: rgba(0, 0, 0, 0.05);
      border-radius: 8px;
      margin: 1rem 0;
    }

    button.secondary {
      background: var(--secondary);
      margin-right: 0.5rem;
    }

    button.danger {
      background: var(--error);
    }

    button.secondary:hover,
    button.danger:hover {
      filter: brightness(1.1);
      transform: translateY(-1px);
    }

    tr {
      transition: background-color 0.3s ease;
    }

    tr:hover {
      background-color: rgba(0, 0, 0, 0.02);
    }

    #listTableContainer {
      background: white;
      padding: 1rem;
      border-radius: 8px;
      box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>智慧衣櫃管理系統</h1>
      <div class="button-group">
        <button onclick="startScan()">掃描新衣物</button>
        <button id="toggleViewBtn" onclick="toggleView()">切換顯示模式</button>
        <button onclick="refreshData()" class="secondary">重新整理資料</button>
      </div>
    </div>

    <div class="stats-container">
      <div class="stat-card">
        <div class="stat-number" id="totalClothes">0</div>
        <div class="stat-label">總衣物數量</div>
      </div>
      <div class="stat-card">
        <div class="stat-number" id="todayUsage">0</div>
        <div class="stat-label">今日使用次數</div>
      </div>
      <div class="stat-card">
        <div class="stat-number" id="avgTemp">-</div>
        <div class="stat-label">平均溫度</div>
      </div>
      <div class="stat-card">
        <div class="stat-number" id="mostUsed">-</div>
        <div class="stat-label">最常使用位置</div>
      </div>
    </div>

    <div class="grid">
      <div class="card weather-card">
        <h2>天氣資訊與穿搭建議</h2>
        <div id="recommendation"></div>
      </div>
      
      <div class="card">
        <h2>衣物管理</h2>
        <div id="contentView"></div>
      </div>
    </div>

    <div class="card">
      <h2>歷史穿搭分析</h2>
      <div class="filter-container">
        <select id="topFilter" onchange="filterHistory()">
          <option value="">選擇上衣</option>
        </select>
        <select id="topColorFilter" onchange="filterHistory()">
          <option value="">選擇上衣顏色</option>
        </select>
        <select id="topStyleFilter" onchange="filterHistory()">
          <option value="">選擇上衣款式</option>
        </select>
        <select id="bottomFilter" onchange="filterHistory()">
          <option value="">選擇下身</option>
        </select>
      </div>
      <div id="similarHistory"></div>
    </div>
  </div>

  <div id="modal" class="modal">
    <div class="modal-content">
      <div id="modalContent"></div>
    </div>
  </div>

  <div id="loading" class="loading">
    <div class="loading-content">處理中...</div>
  </div>
  <div id="notification" class="notification"></div>

  <script>
    let viewMode = 'grid';
    let clothesData = [];
    let historyData = [];
    let chart = null;

    function showLoading() {
      document.getElementById('loading').style.display = 'block';
    }

    function hideLoading() {
      document.getElementById('loading').style.display = 'none';
    }

    function showNotification(message, type = 'info') {
      const notification = document.createElement('div');
      notification.className = `notification ${type}`;
      notification.textContent = message;
      
      document.body.appendChild(notification);
      
      setTimeout(() => {
        notification.classList.add('show');
      }, 100);

      setTimeout(() => {
        notification.classList.remove('show');
        setTimeout(() => {
          notification.remove();
        }, 300);
      }, 3000);
    }

    function toggleView() {
      viewMode = viewMode === 'list' ? 'grid' : 'list';
      document.getElementById('toggleViewBtn').textContent = 
        `切換至${viewMode === 'list' ? '格狀' : '列表'}檢視`;
      renderClothes();
    }

    async function refreshData() {
      try {
        showLoading();
        await Promise.all([
          getRecommend(),
          getCards(),
          getSimilarHistory()
        ]);
        updateStatistics();
        hideLoading();
      } catch (error) {
        console.error('數據刷新失敗:', error);
        showNotification('數據刷新失敗，請稍後重試', 'error');
        hideLoading();
      }
    }

    function updateStatistics() {
      // 更新統計資訊
      document.getElementById('totalClothes').textContent = clothesData.length || 0;
      
      // 計算今日使用次數
      const today = new Date().toISOString().split('T')[0];
      let todayUsage = 0;
      
      if (Array.isArray(historyData) && historyData.length > 0) {
        // 跳過標題行，從索引 1 開始
        todayUsage = historyData.slice(1).filter(row => {
          if (!Array.isArray(row) || row.length === 0) return false;
          const date = row[0];
          return date && date.startsWith && date.startsWith(today);
        }).length;
      }
      
      document.getElementById('todayUsage').textContent = todayUsage;

      // 從天氣資訊中獲取當前溫度範圍並計算平均值
      fetch('/recommend')
        .then(response => response.json())
        .then(data => {
          if (data && data.maxTemp && data.minTemp) {
            const maxTemp = parseFloat(data.maxTemp);
            const minTemp = parseFloat(data.minTemp);
            const avgTemp = (maxTemp + minTemp) / 2;
            document.getElementById('avgTemp').textContent = avgTemp.toFixed(1) + '°C';
          } else {
            document.getElementById('avgTemp').textContent = '-';
          }
        })
        .catch(() => {
          document.getElementById('avgTemp').textContent = '-';
        });

      // 計算最常使用位置
      if (Array.isArray(clothesData) && clothesData.length > 0) {
        const positionCounts = clothesData.reduce((acc, item) => {
          if (item && item.features && item.features.position) {
            const pos = item.features.position;
            acc[pos] = (acc[pos] || 0) + 1;
          }
          return acc;
        }, {});
        
        const positions = Object.entries(positionCounts);
        if (positions.length > 0) {
          const mostUsed = positions.sort((a, b) => b[1] - a[1])[0];
          document.getElementById('mostUsed').textContent = mostUsed[0];
        } else {
          document.getElementById('mostUsed').textContent = '-';
        }
      } else {
        document.getElementById('mostUsed').textContent = '-';
      }
    }

    function updateHistoryChart() {
      const ctx = document.getElementById('historyChart').getContext('2d');
      
      // 如果已經有圖表，先銷毀
      if (chart) {
        chart.destroy();
      }

      // 準備資料
      const last7Days = Array.from({length: 7}, (_, i) => {
        const date = new Date();
        date.setDate(date.getDate() - i);
        return date.toISOString().split('T')[0];
      }).reverse();

      const usageData = last7Days.map(date => {
        return historyData.filter(h => h.date.startsWith(date)).length;
      });

      // 創建新圖表
      chart = new Chart(ctx, {
        type: 'line',
        data: {
          labels: last7Days,
          datasets: [{
            label: '每日使用次數',
            data: usageData,
            borderColor: '#4a90e2',
            backgroundColor: 'rgba(74, 144, 226, 0.1)',
            tension: 0.4,
            fill: true
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          plugins: {
            legend: {
              display: false
            }
          },
          scales: {
            y: {
              beginAtZero: true,
              ticks: {
                stepSize: 1
              }
            }
          }
        }
      });
    }

    async function getCards() {
      try {
        const response = await fetch('/cards');
        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        const data = await response.json();
        
        if (!Array.isArray(data)) {
          throw new Error('返回數據格式錯誤');
        }

        clothesData = data.map(item => ({
          ...item,
          features: item.features || {}
        }));

        updateFilterOptions(clothesData);
        const container = document.getElementById('contentView');
        if (viewMode === 'list') {
          renderListView(container);
        } else {
          renderGridView(container);
        }
      } catch (error) {
        console.error('獲取衣物數據失敗:', error);
        showNotification('獲取衣物數據失敗，請稍後重試', 'error');
      }
    }

    function renderClothes() {
      const contentView = document.getElementById('contentView');
      if (!clothesData || clothesData.length === 0) {
        contentView.innerHTML = `
          <div class="empty-state">
            <div class="empty-state-icon">📦</div>
            <p>目前沒有衣物資料</p>
            <button onclick="startScan()">新增衣物</button>
          </div>
        `;
        return;
      }

      if (viewMode === 'list') {
        renderListView(contentView);
      } else {
        renderGridView(contentView);
      }
    }

    function renderListView(container) {
      // 準備篩選選單的選項
      const tops = new Set();
      const models = new Set();
      const colors = new Set();
      const styles = new Set();
      const positions = new Set();

      clothesData.forEach(item => {
        if (!item.features) return;
        if (item.cloth) tops.add(item.cloth);
        if (item.model) models.add(item.model);
        if (item.features.color) colors.add(item.features.color);
        if (item.features.style) styles.add(item.features.style);
        if (item.features.position) positions.add(item.features.position);
      });

      let html = `
        <div class="list-filters">
          <select id="topFilter" onchange="filterListView()">
            <option value="">選擇衣服</option>
            ${Array.from(tops).sort().map(t => `<option value="${t}">${t}</option>`).join('')}
          </select>
          <select id="modelFilter" onchange="filterListView()">
            <option value="">選擇款式</option>
            ${Array.from(models).sort().map(m => `<option value="${m}">${m}</option>`).join('')}
          </select>
          <select id="colorFilter" onchange="filterListView()">
            <option value="">選擇顏色</option>
            ${Array.from(colors).sort().map(c => `<option value="${c}">${c}</option>`).join('')}
          </select>
          <select id="styleFilter" onchange="filterListView()">
            <option value="">選擇樣式</option>
            ${Array.from(styles).sort().map(s => `<option value="${s}">${s}</option>`).join('')}
          </select>
          <select id="positionFilter" onchange="filterListView()">
            <option value="">選擇位置</option>
            ${Array.from(positions).sort().map(p => `<option value="${p}">${p}</option>`).join('')}
          </select>
        </div>
        <div id="listTableContainer">
          <table>
            <thead>
              <tr>
                <th>衣物名稱</th>
                <th>款式</th>
                <th>顏色</th>
                <th>樣式</th>
                <th>位置</th>
                <th>操作</th>
              </tr>
            </thead>
            <tbody>
      `;

      clothesData.forEach(item => {
        if (!item.features) return;
        
        html += `
          <tr>
            <td>${item.cloth || '-'}</td>
            <td>${item.model || '-'}</td>
            <td>${item.features.color || '-'}</td>
            <td>${item.features.style || '-'}</td>
            <td>${item.features.position || '-'}</td>
            <td>
              <button onclick="editItem('${item.uid}')" class="edit-btn">編輯</button>
              <button onclick="deleteItem('${item.uid}')" class="delete-btn">刪除</button>
            </td>
          </tr>
        `;
      });

      html += '</tbody></table></div>';
      container.innerHTML = html;
    }

    function filterListView() {
      const selectedTop = document.getElementById('topFilter').value;
      const selectedModel = document.getElementById('modelFilter').value;
      const selectedColor = document.getElementById('colorFilter').value;
      const selectedStyle = document.getElementById('styleFilter').value;
      const selectedPosition = document.getElementById('positionFilter').value;

      const filteredClothes = clothesData.filter(item => {
        if (!item.features) return false;
        return (!selectedTop || item.cloth === selectedTop) &&
               (!selectedModel || item.model === selectedModel) &&
               (!selectedColor || item.features.color === selectedColor) &&
               (!selectedStyle || item.features.style === selectedStyle) &&
               (!selectedPosition || item.features.position === selectedPosition);
      });

      const container = document.getElementById('listTableContainer');
      if (filteredClothes.length === 0) {
        container.innerHTML = '<div class="no-results">沒有符合條件的衣物</div>';
        return;
      }

      let html = `
        <table>
          <thead>
            <tr>
              <th>衣物名稱</th>
              <th>款式</th>
              <th>顏色</th>
              <th>樣式</th>
              <th>位置</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
            ${filteredClothes.map(item => `
              <tr>
                <td>${item.cloth || '-'}</td>
                <td>${item.model || '-'}</td>
                <td>${item.features.color || '-'}</td>
                <td>${item.features.style || '-'}</td>
                <td>${item.features.position || '-'}</td>
                <td>
                  <button onclick="editItem('${item.uid}')" class="edit-btn">編輯</button>
                  <button onclick="deleteItem('${item.uid}')" class="delete-btn">刪除</button>
                </td>
              </tr>
            `).join('')}
          </tbody>
        </table>
      `;
      container.innerHTML = html;
    }

    function renderGridView(container) {
      const positions = ['A1', 'A2', 'B1', 'B2'];
      let html = '<div class="clothes-grid">';
      
      positions.forEach(pos => {
        const items = clothesData.filter(item => item.features && item.features.position === pos);
        html += `
          <div class="clothes-space" onclick="showSpaceDetails('${pos}')">
            <div class="space-icon">📍</div>
            <h3>${pos}</h3>
            <p>${items.length} 件衣物</p>
          </div>
        `;
      });

      html += '</div>';
      container.innerHTML = html;
    }

    function showSpaceDetails(pos) {
      console.log('Opening modal for position:', pos);
      const items = clothesData.filter(item => item.features && item.features.position === pos);
      const modalContent = document.getElementById('modalContent');
      
      // 準備篩選選單的選項
      const tops = new Set();
      const models = new Set();
      const colors = new Set();
      const styles = new Set();

      items.forEach(item => {
        if (item.cloth) tops.add(item.cloth);
        if (item.model) models.add(item.model);
        if (item.features && item.features.color) colors.add(item.features.color);
        if (item.features && item.features.style) styles.add(item.features.style);
      });

      let html = `
        <div class="modal-header">
          <h2>${pos} 位置衣物</h2>
          <button onclick="closeModal()" class="modal-close">&times;</button>
        </div>
        <div class="modal-filters">
          <select id="modalTopFilter" onchange="filterModalClothes('${pos}')">
            <option value="">選擇衣服</option>
            ${Array.from(tops).sort().map(t => `<option value="${t}">${t}</option>`).join('')}
          </select>
          <select id="modalModelFilter" onchange="filterModalClothes('${pos}')">
            <option value="">選擇款式</option>
            ${Array.from(models).sort().map(m => `<option value="${m}">${m}</option>`).join('')}
          </select>
          <select id="modalColorFilter" onchange="filterModalClothes('${pos}')">
            <option value="">選擇顏色</option>
            ${Array.from(colors).sort().map(c => `<option value="${c}">${c}</option>`).join('')}
          </select>
          <select id="modalStyleFilter" onchange="filterModalClothes('${pos}')">
            <option value="">選擇樣式</option>
            ${Array.from(styles).sort().map(s => `<option value="${s}">${s}</option>`).join('')}
          </select>
        </div>
        <div id="modalClothesContent">
      `;

      if (items.length === 0) {
        html += `
          <div class="empty-state">
            <div class="empty-state-icon">📦</div>
            <p>此位置目前沒有衣物</p>
          </div>
        `;
      } else {
        html += `
          <table>
            <thead>
              <tr>
                <th>衣物名稱</th>
                <th>款式</th>
                <th>顏色</th>
                <th>樣式</th>
                <th>操作</th>
              </tr>
            </thead>
            <tbody>
        `;

        items.forEach(item => {
          html += `
            <tr>
              <td>${item.cloth || '-'}</td>
              <td>${item.model || '-'}</td>
              <td>${item.features ? (item.features.color || '-') : '-'}</td>
              <td>${item.features ? (item.features.style || '-') : '-'}</td>
              <td>
                <button onclick="editItem('${item.uid}')" class="edit-btn">編輯</button>
                <button onclick="deleteItem('${item.uid}')" class="delete-btn">刪除</button>
              </td>
            </tr>
          `;
        });

        html += '</tbody></table>';
      }

      html += '</div>';
      modalContent.innerHTML = html;
      
      const modal = document.getElementById('modal');
      modal.style.display = 'flex';
      
      // 點擊模態框外部時關閉
      modal.onclick = function(event) {
        if (event.target === modal) {
          closeModal();
        }
      };
    }

    function filterModalClothes(pos) {
      const top = document.getElementById('modalTopFilter').value;
      const model = document.getElementById('modalModelFilter').value;
      const color = document.getElementById('modalColorFilter').value;
      const style = document.getElementById('modalStyleFilter').value;

      const filteredItems = clothesData.filter(item => {
        if (!item.features || item.features.position !== pos) return false;
        
        return (!top || item.cloth === top) &&
               (!model || item.model === model) &&
               (!color || item.features.color === color) &&
               (!style || item.features.style === style);
      });

      const container = document.getElementById('modalClothesContent');
      if (filteredItems.length === 0) {
        container.innerHTML = '<div class="no-results">沒有符合條件的衣物</div>';
        return;
      }

      let html = `
        <table>
          <thead>
            <tr>
              <th>衣物名稱</th>
              <th>款式</th>
              <th>顏色</th>
              <th>樣式</th>
              <th>操作</th>
            </tr>
          </thead>
          <tbody>
      `;

      filteredItems.forEach(item => {
        html += `
          <tr>
            <td>${item.cloth || '-'}</td>
            <td>${item.model || '-'}</td>
            <td>${item.features ? (item.features.color || '-') : '-'}</td>
            <td>${item.features ? (item.features.style || '-') : '-'}</td>
            <td>
              <button onclick="editItem('${item.uid}')" class="edit-btn">編輯</button>
              <button onclick="deleteItem('${item.uid}')" class="delete-btn">刪除</button>
            </td>
          </tr>
        `;
      });

      html += '</tbody></table>';
      container.innerHTML = html;
    }

    function closeModal() {
      document.getElementById('modal').style.display = 'none';
    }

    async function startScan() {
      showLoading();
      try {
        const response = await fetch('/startScan', { method: 'POST' });
        const message = await response.text();
        showNotification(message, 'success');
      } catch (error) {
        showNotification('掃描失敗', 'error');
      }
      hideLoading();
    }

    async function editItem(uid) {
      const item = clothesData.find(i => i.uid === uid);
      if (!item) return;

      const clothesList = ["上衣", "褲子", "裙子", "牛仔褲", "連身裙", "襯衫", "毛衣", "T恤", "外套"];
      const styleList = ["素色", "格紋", "條紋", "直條紋", "花紋", "蕾絲"];
      const colorList = ["白色", "黑色", "紅色", "藍色", "灰色", "綠色", "黃色", "粉色", "紫色", "咖啡色"];
      const positionList = ["A1", "A2", "B1", "B2", "C1", "C2", "D1", "D2"];

      const modalContent = document.getElementById('modalContent');
      modalContent.innerHTML = `
        <div class="modal-header">
          <h2>編輯衣物資訊</h2>
          <button onclick="closeModal()" class="modal-close">&times;</button>
        </div>
        <div class="modal-body" style="padding: 20px;">
          <div class="form-group">
            <label for="editClothes">衣物類型：</label>
            <select id="editClothes" class="edit-select">
              <option value="">請選擇衣物類型</option>
              ${clothesList.map(c => `<option value="${c}" ${item.cloth === c ? 'selected' : ''}>${c}</option>`).join('')}
            </select>
          </div>
          <div class="form-group">
            <label for="editModel">款式描述：</label>
            <input type="text" id="editModel" class="edit-input" value="${item.model || ''}" placeholder="請輸入款式描述">
          </div>
          <div class="form-group">
            <label for="editColor">顏色：</label>
            <select id="editColor" class="edit-select">
              <option value="">請選擇顏色</option>
              ${colorList.map(c => `<option value="${c}" ${item.features?.color === c ? 'selected' : ''}>${c}</option>`).join('')}
            </select>
          </div>
          <div class="form-group">
            <label for="editStyle">樣式：</label>
            <select id="editStyle" class="edit-select">
              <option value="">請選擇樣式</option>
              ${styleList.map(s => `<option value="${s}" ${item.features?.style === s ? 'selected' : ''}>${s}</option>`).join('')}
            </select>
          </div>
          <div class="form-group">
            <label for="editPosition">位置：</label>
            <select id="editPosition" class="edit-select" required>
              <option value="">請選擇位置</option>
              ${positionList.map(p => `<option value="${p}" ${item.features?.position === p ? 'selected' : ''}>${p}</option>`).join('')}
            </select>
          </div>
          <div class="form-actions">
            <button onclick="closeModal()" class="btn-secondary">取消</button>
            <button onclick="saveEdit('${uid}')" class="btn-primary">儲存</button>
          </div>
        </div>
      `;

      // 更新樣式
      const style = document.createElement('style');
      style.textContent = `
        .form-group {
          margin-bottom: 20px;
        }
        .form-group label {
          display: block;
          margin-bottom: 8px;
          color: var(--primary);
          font-weight: 500;
        }
        .edit-select, .edit-input {
          width: 100%;
          padding: 10px;
          border: 1px solid var(--border);
          border-radius: 5px;
          font-size: 16px;
          background-color: white;
        }
        .edit-select:focus, .edit-input:focus {
          outline: none;
          border-color: var(--primary);
          box-shadow: 0 0 0 2px rgba(27, 38, 79, 0.1);
        }
        .form-actions {
          display: flex;
          justify-content: flex-end;
          gap: 10px;
          margin-top: 30px;
        }
        .btn-primary, .btn-secondary {
          padding: 10px 20px;
          border-radius: 5px;
          font-size: 16px;
          cursor: pointer;
          transition: all 0.3s ease;
        }
        .btn-primary {
          background: var(--primary);
          color: white;
          border: none;
        }
        .btn-secondary {
          background: white;
          color: var(--primary);
          border: 1px solid var(--primary);
        }
        .btn-primary:hover {
          background: #2a3a6d;
        }
        .btn-secondary:hover {
          background: #f5f5f5;
        }
      `;
      modalContent.appendChild(style);

      document.getElementById('modal').style.display = 'flex';
    }

    async function saveEdit(uid) {
      const clothes = document.getElementById('editClothes').value;
      const model = document.getElementById('editModel').value;
      const style = document.getElementById('editStyle').value;
      const color = document.getElementById('editColor').value;
      const position = document.getElementById('editPosition').value;

      showLoading();
      try {
        // 構建更新資料
        const updateData = {
          cloth: clothes,
          model: model,
          features: {
            style: style,
            color: color,
            position: position
          }
        };

        console.log('Sending update:', updateData);  // 除錯用

        const response = await fetch(`/cards?uid=${uid}`, {
          method: 'PUT',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(updateData)
        });

        const responseData = await response.text();
        console.log('Update response:', responseData);  // 除錯用

        if (response.ok) {
          showNotification('更新成功', 'success');
          // 重新載入資料
          await refreshData();
          closeModal();
        } else {
          showNotification(`更新失敗: ${responseData}`, 'error');
        }
      } catch (error) {
        console.error('更新失敗:', error);
        showNotification(`更新失敗: ${error.message}`, 'error');
      } finally {
        hideLoading();
      }
    }

    async function deleteItem(uid) {
      try {
        if (!confirm('確定要刪除這件衣物嗎？')) {
          return;
        }

        const response = await fetch(`/cards?uid=${uid}`, {
          method: 'DELETE'
        });

        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }

        showNotification('衣物刪除成功', 'success');
        await getCards();
      } catch (error) {
        console.error('刪除衣物失敗:', error);
        showNotification('刪除衣物失敗，請稍後重試', 'error');
      }
    }

    async function getRecommend() {
      try {
        const response = await fetch('/recommend');
        const html = await response.text();
        // 處理時間格式
        const processedHtml = html.replace(
          /(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}) ~ (\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})/g,
          (match, start, end) => {
            const startDate = new Date(start);
            const endDate = new Date(end);
            return `${startDate.getMonth()+1}/${startDate.getDate()} ${startDate.getHours()}:00 ~ ${endDate.getHours()}:00`;
          }
        );
        document.getElementById('recommendation').innerHTML = processedHtml;
      } catch (error) {
        showNotification('獲取天氣資訊失敗', 'error');
      }
    }

    async function getSimilarHistory() {
      try {
        const response = await fetch('/similarHistory');
        const data = await response.json();
        
        // 確保資料是陣列格式
        if (!Array.isArray(data)) {
          console.error('歷史資料格式錯誤:', data);
          showNotification('歷史資料格式錯誤', 'error');
          return;
        }
        
        historyData = data;
        console.log('歷史資料載入成功，筆數:', historyData.length);
        
        // 更新篩選選單選項
        if (historyData.length > 0) {
          updateFilterOptions(historyData);
          // 顯示所有資料
          filterHistory();
        } else {
          document.getElementById('similarHistory').innerHTML = '<div class="no-results">目前沒有歷史資料</div>';
        }
      } catch (error) {
        console.error('獲取歷史資料失敗:', error);
        showNotification('獲取歷史資料失敗', 'error');
      }
    }

    function updateFilterOptions(data) {
      if (!Array.isArray(data) || data.length <= 1) {
        console.error('歷史資料格式錯誤或沒有資料');
        return;
      }

      const tops = new Set();
      const topColors = new Set();
      const topStyles = new Set();
      const bottoms = new Set();

      // 從第二筆資料開始處理（跳過標題行）
      for (let i = 1; i < data.length; i++) {
        const row = data[i];
        if (Array.isArray(row)) {
          // 只加入非空值且不是 '-' 的選項
          if (row[6] && row[6] !== '-') tops.add(row[6]);
          if (row[7] && row[7] !== '-') topColors.add(row[7]);
          if (row[8] && row[8] !== '-') topStyles.add(row[8]);
          if (row[9] && row[9] !== '-') bottoms.add(row[9]);
        }
      }

      console.log('篩選選項統計:', {
        '上衣數量': tops.size,
        '上衣顏色數量': topColors.size,
        '上衣款式數量': topStyles.size,
        '下身數量': bottoms.size
      });

      // 更新選單選項
      updateSelect('topFilter', tops);
      updateSelect('topColorFilter', topColors);
      updateSelect('topStyleFilter', topStyles);
      updateSelect('bottomFilter', bottoms);
    }

    function updateSelect(id, options) {
      const select = document.getElementById(id);
      if (!select) {
        console.error(`找不到選單元素: ${id}`);
        return;
      }

      // 保留第一個預設選項
      const defaultOption = select.options[0];
      select.innerHTML = '';
      select.appendChild(defaultOption);
      
      // 添加新選項
      Array.from(options).sort().forEach(option => {
        if (option && option.trim()) {
          const opt = document.createElement('option');
          opt.value = option;
          opt.textContent = option;
          select.appendChild(opt);
        }
      });
    }

    async function filterHistory() {
      try {
        showLoading();
        
        const top = document.getElementById('topFilter').value;
        const topColor = document.getElementById('topColorFilter').value;
        const topStyle = document.getElementById('topStyleFilter').value;
        const bottom = document.getElementById('bottomFilter').value;

        console.log('篩選條件:', { top, topColor, topStyle, bottom });

        if (!Array.isArray(historyData) || historyData.length <= 1) {
          document.getElementById('similarHistory').innerHTML = '<div class="no-results">沒有歷史資料可供篩選</div>';
          return;
        }

        // 篩選資料
        const filteredData = historyData.filter((row, index) => {
          if (index === 0) return false; // 跳過標題行
          if (!Array.isArray(row)) return false;

          const matchTop = !top || row[6] === top;
          const matchTopColor = !topColor || row[7] === topColor;
          const matchTopStyle = !topStyle || row[8] === topStyle;
          const matchBottom = !bottom || row[9] === bottom;

          return matchTop && matchTopColor && matchTopStyle && matchBottom;
        });

        console.log('篩選結果:', {
          '總資料筆數': historyData.length,
          '篩選後筆數': filteredData.length
        });

        if (filteredData.length === 0) {
          document.getElementById('similarHistory').innerHTML = '<div class="no-results">沒有符合條件的資料</div>';
        } else {
          let html = `<div class="table-container"><table>
            <caption>符合條件的歷史紀錄 (共 ${filteredData.length} 筆)</caption>
            <thead><tr>
              <th>時間</th><th>天氣</th><th>最高溫</th><th>最低溫</th>
              <th>舒適度</th><th>降雨機率</th>
              <th>上衣</th><th>上衣顏色</th><th>上衣款式</th>
              <th>下身</th><th>下身顏色</th><th>下身款式</th>
            </tr></thead><tbody>`;

          filteredData.forEach(row => {
            if (Array.isArray(row)) {
              html += `<tr>
                ${row.map(cell => `<td>${cell || '-'}</td>`).join('')}
              </tr>`;
            }
          });

          html += '</tbody></table></div>';
          document.getElementById('similarHistory').innerHTML = html;
        }
      } catch (error) {
        console.error('篩選資料時發生錯誤:', error);
        showNotification('篩選資料時發生錯誤', 'error');
      } finally {
        hideLoading();
      }
    }

    // 定期檢查掃描狀態
    setInterval(async () => {
      try {
        const response = await fetch('/scanStatus');
        const data = await response.json();
        
        if (data.pendingUID) {
          if (data.pendingExists) {
            showNotification(`此卡已登記: ${data.pendingUID}`, 'error');
          } else {
            const style = prompt('輸入樣式:');
            if (style === null) return;

            const color = prompt('輸入顏色:');
            if (color === null) return;

            const position = prompt('輸入位置 (A1/A2/B1/B2):');
            if (position === null) return;

            showLoading();
            try {
              const response = await fetch('/cards', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                  uid: data.pendingUID,
                  features: { style, color, position }
                })
              });

              if (response.ok) {
                showNotification('新增成功', 'success');
                await getCards();
              } else {
                showNotification('新增失敗', 'error');
              }
            } catch (error) {
              showNotification('新增失敗', 'error');
            }
            hideLoading();
          }
        }
      } catch (error) {
        console.error('檢查掃描狀態失敗:', error);
      }
    }, 1000);

    // 頁面載入時初始化
    window.onload = async function() {
      try {
        // 先獲取衣物資料
        const response = await fetch('/cards');
        clothesData = await response.json();
        
        // 然後渲染衣物管理視圖
        renderClothes();
        
        // 最後刷新其他資料
        await refreshData();
      } catch (error) {
        showNotification('初始化失敗', 'error');
      }
    };
  </script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H
