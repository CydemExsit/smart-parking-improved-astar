# Smart Parking — Improved A* (Final Thesis Code)

本倉庫包含我論文的兩支最終程式（皆**無需外部輸入、直接執行**），以及論文用數據檔：
- `250919repath.cpp` → 動態重規劃展示（Improved A* + 事件阻塞 + 等待成本）
- `250604statisticlog.cpp` → 批次統計（Traditional A* vs Improved A*）
- `results/results_cleaned_forPAPER.csv` → 論文實際使用的 5,000 筆清洗樣本
  > 完整 50,000 筆：`resultsFinal.csv`（將於 Releases 提供，或依需求加入 repo）

## 1. TL;DR
- 平均等待時間改善 **62.02%**（摘自《Final_Thesis.pdf》：Improved A* 相較傳統 A* 的整體平均等待時間由 3.600 秒降至 1.368 秒）
- 動態重規劃成功率 **100%**（摘自《Final_Thesis.pdf》：動態重規劃機制在受阻時仍能維持穩定即時性與成功率）

## 2. Quickstart
```bash
scripts/reproduce.ps1          # Windows
bash scripts/reproduce.sh      # Linux / Git Bash
```

## 3. Reproduce
執行上述腳本後會：

* 以 CMake 自動建置兩支主程式
* 依序執行 `250919repath` 與 `250604statisticlog`
* 讀取 `results/results_cleaned_forPAPER.csv`，輸出統計圖於 `docs/figs/`
* 產出論文中使用的數據與圖表，可直接對照最終論文結果

## Data

* `results/results_cleaned_forPAPER.csv`：論文採用之 5k 清洗樣本（由 50k 全量隨機擷取）
* `resultsFinal.csv`（50k 全量）將放在 GitHub Releases；需要時可另外提供
* 論文 PDF：`docs/Final_Thesis.pdf`

## Repo 結構

```text
smart-parking-improved-astar/
├─ src/
│  ├─ 250919repath.cpp
│  └─ 250604statisticlog.cpp
├─ results/
│  └─ results_cleaned_forPAPER.csv
├─ docs/
│  ├─ Final_Thesis.pdf
│  └─ figs/
├─ scripts/
│  ├─ reproduce.ps1
│  ├─ reproduce.sh
│  └─ plot_results.py
├─ CMakeLists.txt
└─ README.md
```

## Citation

* Final thesis: **Path Planning and Efficiency Optimization in Smart Parking Systems Using an Improved A***（Tamkang University, 2025）
