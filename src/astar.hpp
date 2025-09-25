#pragma once
#include <vector>
#include <queue>
#include <utility>
#include <limits>
#include <unordered_map>
#include <functional>

struct Node {
    int r, c;
    double g = std::numeric_limits<double>::infinity(); // cost so far
    double f = std::numeric_limits<double>::infinity(); // g + h
    int pr = -1, pc = -1; // parent
};

struct Grid {
    // 0 = walkable, 1 = wall/blocked
    std::vector<std::vector<int>> cell; // [row][col]
    int R() const { return (int)cell.size(); }
    int C() const { return cell.empty() ? 0 : (int)cell[0].size(); }
    bool in(int r, int c) const { return r>=0 && r<R() && c>=0 && c<C(); }
    bool walkable(int r, int c) const { return in(r,c) && cell[r][c]==0; }
};

// 等待成本（之後你可接「封閉格剩餘阻塞時間」邏輯）
// 先回傳 0，確保能跑通
inline double wait_cost(int /*r*/, int /*c*/, double /*t*/) {
    return 0.0;
}

class GridAStar {
public:
    using Path = std::vector<std::pair<int,int>>;

    // 簡單的 4 鄰接 A*，帶可插拔等待成本
    Path find_path(const Grid& g, std::pair<int,int> s, std::pair<int,int> t) {
        auto H = [&](int r, int c) {
            // 曼哈頓啟發式
            return (double) (std::abs(r - t.first) + std::abs(c - t.second));
        };

        auto key = [&](int r, int c) { return (long long)r<<32 | (unsigned int)c; };

        std::unordered_map<long long, Node> nodes;
        auto& sn = nodes[key(s.first, s.second)];
        sn.r = s.first; sn.c = s.second; sn.g = 0; sn.f = H(s.first, s.second);

        using QN = std::pair<double, std::pair<int,int>>; // (f, (r,c))
        std::priority_queue<QN, std::vector<QN>, std::greater<QN>> pq;
        pq.push({sn.f, s});

        const int dr[4] = {-1,1,0,0};
        const int dc[4] = {0,0,-1,1};

        std::unordered_map<long long, bool> closed;

        while (!pq.empty()) {
            auto [fcur, pos] = pq.top(); pq.pop();
            int r = pos.first, c = pos.second;
            auto k = key(r,c);
            if (closed[k]) continue;
            closed[k] = true;

            if (r==t.first && c==t.second) {
                // 回溯路徑
                Path path;
                long long curk = k;
                while (true) {
                    const auto& nd = nodes[curk];
                    path.push_back({nd.r, nd.c});
                    if (nd.pr==-1 && nd.pc==-1) break;
                    curk = key(nd.pr, nd.pc);
                }
                std::reverse(path.begin(), path.end());
                return path;
            }

            for (int i=0;i<4;i++){
                int nr = r + dr[i], nc = c + dc[i];
                if (!g.walkable(nr,nc)) continue;
                auto nk = key(nr,nc);

                double move_cost = 1.0;           // 基礎移動成本
                double wcost = wait_cost(nr,nc,0); // 等待成本（目前先 0）
                double ng = nodes[k].g + move_cost + wcost;
                auto& nn = nodes[nk];
                if (ng < nn.g) {
                    nn.r = nr; nn.c = nc;
                    nn.g = ng;
                    nn.f = ng + H(nr,nc);
                    nn.pr = r; nn.pc = c;
                    pq.push({nn.f, {nr,nc}});
                }
            }
        }
        return {}; // 空 = 找不到
    }
};
