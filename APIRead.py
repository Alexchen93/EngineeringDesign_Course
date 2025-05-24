import requests
import json

# 你的 API URL
url = "API"

# 發送 GET 請求
response = requests.get(url)

# 確認回傳狀態
if response.status_code == 200:
    # 轉換回傳內容為 JSON 格式
    data = response.json()
    
    # 印出回傳的 JSON 資料（整個內容）
    print(json.dumps(data, ensure_ascii=False, indent=4))
    
    # 或只取部分內容，例如天氣預報
    weather_elements = data['records']['location'][0]['weatherElement']
    for element in weather_elements:
        print(f"{element['elementName']}: {element['time'][0]['parameter']['parameterName']}")
else:
    print(f"請求失敗，狀態碼: {response.status_code}")
