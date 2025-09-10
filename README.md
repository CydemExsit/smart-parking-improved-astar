# Smart Parking — Improved A* with Dynamic Re-Routing
> C++ simulation for path planning in a parking lot with real-time incident handling and wait-time–aware costs.（中文說明見下）

[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](#license)

## TL;DR
- **核心貢獻**：在 A* 評估函數中加入「等待成本/封閉格剩餘阻塞時間」，支援**動態重規劃**與**突發事件**（臨時封路、車故障）處理。
- **成品**：可重現論文實驗（效能統計、突發事件案例），輸出 CSV 與可視化圖表。

---

## Repo 結構
