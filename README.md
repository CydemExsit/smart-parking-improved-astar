# Smart Parking — Improved A* (Final Thesis Code)

本倉庫包含我論文的兩支最終程式（皆**無需外部輸入、直接執行**），以及論文用數據檔：
- `250919repath.cpp` → 動態重規劃展示（Improved A* + 事件阻塞 + 等待成本）
- `250604statisticlog.cpp` → 批次統計（Traditional A* vs Improved A*）
- `results/results_cleaned_forPAPER.csv` → 論文實際使用的 5,000 筆清洗樣本  
  > 完整 50,000 筆：`resultsFinal.csv`（將於 Releases 提供，或依需求加入 repo）

## TL;DR
- 改良 A*：在評估中引入等待成本 / 封閉格剩餘時間；支援突發事件下的動態重規劃
- 論文結論（摘要）：平均等待時間顯著下降；動態事件情境下重規劃成功率高（詳見論文）

---

## Build
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run (no external input)

```
# 動態重規劃展示
./build/250919repath            # Windows: .\build\250919repath.exe

# 批次統計（Traditional vs Improved）
./build/250604statisticlog      # Windows: .\build\250604statisticlog.exe
```

> 保存主控台輸出：
>
> ```bash
> ./build/250604statisticlog > results_console.txt
> ```

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
├─ CMakeLists.txt
└─ README.md
```


## Citation

* Final thesis: **Path Planning and Efficiency Optimization in Smart Parking Systems Using an Improved A***（Tamkang University, 2025）
