#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "astar.hpp"

// 讀取簡易 CSV 地圖：0=走道, 1=牆
Grid load_csv_map(const std::string& path){
    Grid g;
    std::ifstream fin(path);
    if (!fin) {
        std::cerr << "Failed to open map: " << path << "\n";
        return g;
    }
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        std::vector<int> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            // 容忍空白
            if (cell == " " || cell == "") continue;
            row.push_back(std::stoi(cell));
        }
        if (!row.empty()) g.cell.push_back(std::move(row));
    }
    return g;
}

int main(int argc, char** argv){
    std::string map_path = "data/maps/demo_map.csv"; // 預設跑示例
    for (int i=1;i<argc;i++){
        std::string a = argv[i];
        if (a=="--map" && i+1<argc) { map_path = argv[++i]; }
    }

    Grid g = load_csv_map(map_path);
    if (g.R()==0 || g.C()==0){
        std::cerr << "Map is empty or invalid.\n";
        return 1;
    }

    std::pair<int,int> S = {0,0};
    std::pair<int,int> T = {g.R()-1, g.C()-1};

    if (!g.walkable(S.first,S.second) || !g.walkable(T.first,T.second)){
        std::cerr << "Start or Target not walkable.\n";
        return 1;
    }

    GridAStar astar;
    auto path = astar.find_path(g, S, T);
    if (path.empty()){
        std::cout << "No path found.\n";
        return 0;
    }
    std::cout << "Path length: " << path.size() << "\n";
    for (auto [r,c] : path){
        std::cout << "(" << r << "," << c << ") ";
    }
    std::cout << "\n";
    return 0;
}
