#include <iostream>
#include <vector>
#include <queue>
#include <climits>
#include <cmath>
#include <stack>
#include <thread>
#include <chrono>
#include <mutex>
#include <cstdlib>
#include <ctime>
#include <unordered_set>
#include <atomic>
#include <fstream>
#include <algorithm> // for std::shuffle
#include <random>    // for std::default_random_engine

using namespace std;
using namespace std::chrono;

static mutex g_logMutex;
static string g_runId;
static ofstream g_assignmentFile;
static int g_loggedCount = 0;

enum CellType
{
    ENTRANCE,
    AISLE,
    WALL,
    PARKING_SPACE,
    VEHICLE
};

// --------------------------------------------------------------------
// Cell：多了 occupiedBy 來避免多車同時改 waitTime
// --------------------------------------------------------------------
struct Cell
{
    int row, col;
    CellType type;
    int waitTime = 0;
    char vehicleID;
    bool isMoving = false;
    char occupiedBy = '\0';

    Cell(int r = 0, int c = 0, CellType t = AISLE)
        : row(r), col(c), type(t), vehicleID(' ') {}
};

// --------------------------------------------------------------------
// VehicleTime：紀錄「哪輛車(vehicleID)」「index 第幾台車」「time 統計值」
// --------------------------------------------------------------------
struct VehicleTime
{
    char vehicleID;
    int vehicleIndex; // 第幾台車
    long long time;
    VehicleTime(char id, int idx, long long t)
        : vehicleID(id), vehicleIndex(idx), time(t) {}
};

class ParkingLot
{
private:
    static const int MAX_ROWS = 13;
    static const int MAX_COLS = 12;

    vector<vector<Cell>> parkingLot;
    mutable mutex mtx;

    // 行駛時間 / 延遲時間
    vector<VehicleTime> vehicleTimes;
    vector<VehicleTime> delayTimes;

    bool useImprovedAStar = false;

    struct Node
    {
        int row, col, g, h;
        vector<pair<int, int>> path;
        Node(int r, int c, int gVal, int hVal)
            : row(r), col(c), g(gVal), h(hVal)
        {
            path.emplace_back(r, c);
        }
        int f() const { return g + h; }
        bool operator>(const Node &other) const { return f() > other.f(); }
    };

    // --------------------------------------------------------------------
    // moveVehicle：檢查 path 是否空，避免 segfault + occupant 機制 + 統計 delay
    // 加「vehicleIndex」參數，用於記錄到 vehicleTimes / delayTimes
    // --------------------------------------------------------------------
    void moveVehicle(vector<pair<int, int>> &path, char vehicleID, int vehicleIndex)
    {
        auto startTime = steady_clock::now();

        if (path.empty())
        {
            cout << "[moveVehicle] path is empty => no move.\n";
            return;
        }
        // 起點標記
        parkingLot[path[0].first][path[0].second].type = VEHICLE;
        parkingLot[path[0].first][path[0].second].vehicleID = vehicleID;
        parkingLot[path[0].first][path[0].second].isMoving = true;

        // 計算 wtSum
        int wtSum = 0;
        for (size_t i = 1; i + 1 < path.size(); i++)
        {
            int cw = parkingLot[path[i].first][path[i].second].waitTime;
            if (cw > 0)
            {
                int adj = cw - (int)i + 1;
                if (adj > 9)
                    adj = 0;
                wtSum += max(adj, 0);
            }
        }
        int initialVal = (int)path.size() + 9 + wtSum;

        // 合併最後一格 occupant
        if (!path.empty())
        {
            int rr = path.back().first;
            int cc = path.back().second;
            int oldVal = parkingLot[rr][cc].waitTime;

            int distance = (int)path.size() - 1;
            if (distance < 0)
                distance = 0;

            int offset = 0;
            if (oldVal > 0)
            {
                int tmp = oldVal - distance;
                if (tmp < 0)
                    tmp = 0;
                offset = tmp;
            }
            int myVal = initialVal + offset;
            myVal = max(myVal, oldVal);

            if (myVal > oldVal)
            {
                parkingLot[rr][cc].occupiedBy = vehicleID;
                parkingLot[rr][cc].waitTime = myVal;
            }
            else
            {
                parkingLot[rr][cc].waitTime = oldVal;
            }
        }

        // 統計延遲
        int delay = 0;

        // 移動
        for (size_t i = 1; i < path.size();)
        {
            if (path.size() <= 1)
                break; // 只剩最後一格 => break

            // 若下一格可進 => 移動
            if (!parkingLot[path[i].first][path[i].second].isMoving)
            {
                pair<int, int> oldPos = path[0];
                pair<int, int> newPos = path[i];

                {
                    lock_guard<mutex> lock(mtx);
                    parkingLot[oldPos.first][oldPos.second].vehicleID = ' ';
                    parkingLot[oldPos.first][oldPos.second].type = AISLE;
                    parkingLot[oldPos.first][oldPos.second].isMoving = false;

                    parkingLot[newPos.first][newPos.second].vehicleID = vehicleID;
                    parkingLot[newPos.first][newPos.second].type = VEHICLE;
                    parkingLot[newPos.first][newPos.second].isMoving = true;
                }
                // 移除 path.begin() => 前進
                path.erase(path.begin());
            }
            else
            {
                // 無法前進 => delay++
                delay++;
            }

            {
                int r = path.back().first;
                int c = path.back().second;
                if (parkingLot[r][c].occupiedBy == vehicleID)
                {
                    int wtSum2 = 0;
                    for (size_t k = 1; k < path.size() - 1; k++)
                    {
                        int cellWait = parkingLot[path[k].first][path[k].second].waitTime;
                        if (cellWait > 0)
                        {
                            int adj2 = cellWait - (int)k + 1;
                            if (adj2 > 9)
                                adj2 = 0;
                            wtSum2 += std::max(adj2, 0);
                        }
                    }
                    int baseVal2 = (int)path.size() + 9 + wtSum2;

                    if (parkingLot[r][c].waitTime > 0)
                    {
                        parkingLot[r][c].waitTime--;
                    }
                }
            }

            this_thread::sleep_for(std::chrono::seconds(1));
            // displayStatus(); // 大量測試時可註解

            // 再檢查
            if (path.size() <= 1)
                break;
        }

        // 倒車9秒
        if (!path.empty() && path.size() == 1)
        {
            int rr = path.back().first;
            int cc = path.back().second;
            for (int j = 9; j >= 0; j--)
            {
                parkingLot[rr][cc].waitTime = j;
                if (j == 0)
                {
                    parkingLot[rr][cc].type = AISLE;
                    parkingLot[rr][cc].waitTime = 0;
                    parkingLot[rr][cc].vehicleID = ' ';
                    parkingLot[rr][cc].isMoving = false;
                    parkingLot[rr][cc].occupiedBy = '\0';
                }
                this_thread::sleep_for(std::chrono::seconds(1));
                // displayStatus();
            }
        }

        auto endTime = steady_clock::now();
        auto duration = duration_cast<seconds>(endTime - startTime).count();

        {
            lock_guard<mutex> lock(mtx);
            // 在這裡把 vehicleIndex 也記進去
            vehicleTimes.emplace_back(vehicleID, vehicleIndex, (long long)duration);
            delayTimes.emplace_back(vehicleID, vehicleIndex, (long long)delay);
        }
    }

    //--------------------------------------------------------------------------------
    // A*： 若 useImprovedAStar==true => 把 waitTime - baseG 加到 G cost
    //      再呼叫 moveVehicle
    //--------------------------------------------------------------------------------
    bool isCellValid(int row, int col) const
    {
        return (row >= 0 && row < (int)parkingLot.size() &&
                col >= 0 && col < (int)parkingLot[0].size() &&
                (parkingLot[row][col].type == AISLE ||
                 (parkingLot[row][col].type == VEHICLE && parkingLot[row][col].isMoving)));
    }

    void aStar(int sr, int sc, int er, int ec, char vehicleID, int vehicleIndex)
    {
        if (!isCellValid(sr, sc) || !isCellValid(er, ec))
        {
            cout << "Invalid start or end pos.\n";
            return;
        }
        priority_queue<Node, vector<Node>, greater<Node>> pq;
        vector<vector<int>> cost(parkingLot.size(), vector<int>(parkingLot[0].size(), INT_MAX));

        cost[sr][sc] = 0;
        pq.emplace(sr, sc, 0, calcHeuristic(sr, sc, er, ec));

        while (!pq.empty())
        {
            Node cur = pq.top();
            pq.pop();

            if (cur.row == er && cur.col == ec)
            {
                moveVehicle(cur.path, vehicleID, vehicleIndex);
                return;
            }
            static const int DR[4] = {-1, 1, 0, 0};
            static const int DC[4] = {0, 0, -1, 1};

            for (int i = 0; i < 4; i++)
            {
                int nr = cur.row + DR[i];
                int nc = cur.col + DC[i];
                if (isCellValid(nr, nc))
                {
                    int baseG = cur.g + 1;
                    int extra = 0;
                    if (useImprovedAStar)
                    {
                        int w = parkingLot[nr][nc].waitTime;
                        int remain = w - baseG;
                        if (remain > 0)
                            extra = remain;
                    }
                    int newG = baseG + extra;

                    if (newG < cost[nr][nc])
                    {
                        cost[nr][nc] = newG;
                        Node nxt(nr, nc, newG, calcHeuristic(nr, nc, er, ec));
                        nxt.path = cur.path;
                        nxt.path.emplace_back(nr, nc);
                        pq.push(nxt);
                    }
                }
            }
        }
        cout << "No valid path found.\n";
    }

    int calcHeuristic(int r1, int c1, int r2, int c2)
    {
        return abs(r2 - r1) + abs(c2 - c1);
    }

public:
    // 小地圖 13×12
    ParkingLot()
    {
        parkingLot.resize(MAX_ROWS, vector<Cell>(MAX_COLS));
        for (int i = 0; i < MAX_ROWS; i++)
        {
            for (int j = 0; j < MAX_COLS; j++)
            {
                parkingLot[i][j] = Cell(i, j, AISLE);
            }
        }
    }

    ParkingLot(const ParkingLot &other)
    {
        this->parkingLot = other.parkingLot;
        this->useImprovedAStar = other.useImprovedAStar;
    }

    void setUseImprovedAStar(bool improved)
    {
        useImprovedAStar = improved;
    }

    const vector<vector<Cell>> &getParkingLot() const
    {
        return parkingLot;
    }

    void addCell(int r, int c, CellType t)
    {
        if (r >= 0 && r < (int)parkingLot.size() && c >= 0 && c < (int)parkingLot[0].size())
        {
            parkingLot[r][c].type = t;
        }
    }

    // 將「車輛」放在 (row,col) => 跑 aStar(0,4 => row+dir, col+dir)
    // 加了 vehicleIndex 參數
    bool addVehicle(int row, int col, char vehicleID, int vehicleIndex)
    {
        if (row < 0 || row >= (int)parkingLot.size() || col < 0 || col >= (int)parkingLot[0].size())
        {
            cout << "(addVehicle) invalid pos.\n";
            return false;
        }
        if (parkingLot[row][col].type == PARKING_SPACE)
        {
            parkingLot[row][col].type = VEHICLE;
            parkingLot[row][col].vehicleID = vehicleID;

            // 找相鄰 aisles
            int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (auto &dir : dirs)
            {
                int nr = row + dir[0];
                int nc = col + dir[1];
                if (isCellValid(nr, nc))
                {
                    aStar(0, 4, nr, nc, vehicleID, vehicleIndex);
                    return true;
                }
            }
            cout << "No valid aisle near.\n";
            return false;
        }
        else
        {
            cout << "Not a PARKING_SPACE.\n";
            return false;
        }
    }

    // 取得行駛時間
    vector<VehicleTime> getVehicleTimes() const
    {
        return vehicleTimes;
    }
    // 取得延遲時間
    vector<VehicleTime> getDelayTime() const
    {
        return delayTimes;
    }

    // 顯示地圖(可註解掉)
    void displayStatus()
    {
        static std::atomic_flag isDisp = ATOMIC_FLAG_INIT;
        if (isDisp.test_and_set())
            return;
        try
        {
            static std::atomic<long long> lastD(0);
            auto now = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
            if (now - lastD >= 1000)
            {
                lastD = now;
                system("cls");
                cout << "   ";
                for (int c = 0; c < MAX_COLS; c++)
                {
                    cout << (c % 10) << " ";
                }
                cout << "\n";
                for (int r = 0; r < MAX_ROWS; r++)
                {
                    cout << (r % 10) << " ";
                    for (int c = 0; c < MAX_COLS; c++)
                    {
                        char ch = ' ';
                        if (parkingLot[r][c].waitTime > 0)
                        {
                            ch = '0' + (parkingLot[r][c].waitTime % 10);
                        }
                        else
                        {
                            switch (parkingLot[r][c].type)
                            {
                            case ENTRANCE:
                            case AISLE:
                                ch = ' ';
                                break;
                            case WALL:
                                ch = '+';
                                break;
                            case PARKING_SPACE:
                                ch = '-';
                                break;
                            case VEHICLE:
                                ch = parkingLot[r][c].vehicleID;
                                break;
                            }
                        }
                        cout << ch << " ";
                    }
                    cout << "\n";
                }
                cout << "\n";
            }
        }
        catch (...)
        {
            isDisp.clear();
            throw;
        }
        isDisp.clear();
    }
};

void addVehicleLogged(ParkingLot &lot, char vehicleID, int row, int col, int index)
{
    if (g_loggedCount < 20)
    {
        auto now = system_clock::now();
        std::time_t tt = system_clock::to_time_t(now);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&tt));
        {
            lock_guard<mutex> lock(g_logMutex);
            g_assignmentFile << g_runId << ',' << vehicleID << ',' << row << ',' << col << ',' << buf << '\n';
        }
        ++g_loggedCount;
    }
    lot.addVehicle(row, col, vehicleID, index);
}

// ----------------------------------------------------------------------
// For Average
// ----------------------------------------------------------------------
double calcAverageOfFirstN(const vector<VehicleTime> &arr, int N)
{
    if (arr.empty())
        return 0.0;
    int limit = min((int)arr.size(), N);
    long long sum = 0;
    for (int i = 0; i < limit; i++)
    {
        sum += arr[i].time;
    }
    return (limit > 0) ? (double)sum / limit : 0.0;
}
double calcAverageOfLastN(const vector<VehicleTime> &arr, int N)
{
    if (arr.empty())
        return 0.0;
    int sz = (int)arr.size();
    int start = max(sz - N, 0);
    long long sum = 0;
    for (int i = start; i < sz; i++)
    {
        sum += arr[i].time;
    }
    int count = sz - start;
    return (count > 0) ? (double)sum / count : 0.0;
}

// 取「前10輛」 => vehicleIndex < 10
// 取「後10輛」 => vehicleIndex >= 10
// ex: "frontAvg( times, 0,10 )"
double calcAverageByIndexRange(const vector<VehicleTime> &arr, int beginIdx, int endIdx)
{
    // 針對 arr 裡 vehicleIndex 在 [beginIdx, endIdx) 區間的做平均
    if (arr.empty())
        return 0.0;
    long long sum = 0;
    int count = 0;
    for (auto &vt : arr)
    {
        if (vt.vehicleIndex >= beginIdx && vt.vehicleIndex < endIdx)
        {
            sum += vt.time;
            count++;
        }
    }
    return (count > 0) ? (double)sum / count : 0.0;
}

// ----------------------------------------------------------------------
// 在 main 中執行：
//   1) 建立 baseLot => Setting
//   2) 產生 vehicleIDs + spaces, 確保隨機位置不重複
//   3) parkingLotOriginal、parkingLotImproved
//   4) Each => addVehicle(..., index)
//   5) Print front10 / last10
// ----------------------------------------------------------------------
int main(int argc, char *argv[])
{
    srand((unsigned)time(nullptr));

    if (argc > 1)
    {
        g_runId = argv[1];
    }
    else
    {
        g_runId = to_string(time(nullptr));
    }
    g_assignmentFile.open("vehicle_assignments.csv", ios::app);

    ParkingLot baseLot;

    // 設定地圖(同你給的例子)
    baseLot.addCell(0, 0, WALL);
    baseLot.addCell(0, 11, WALL);
    baseLot.addCell(12, 0, WALL);
    baseLot.addCell(12, 11, WALL);

    for (int i = 1; i <= 10; i++)
    {
        baseLot.addCell(0, i, PARKING_SPACE);
        baseLot.addCell(12, i, PARKING_SPACE);
    }
    for (int r = 1; r <= 11; r++)
    {
        baseLot.addCell(r, 0, PARKING_SPACE);
        baseLot.addCell(r, 11, PARKING_SPACE);
    }
    // 唯一入口 (0,4)
    baseLot.addCell(0, 4, AISLE);
    baseLot.addCell(0, 3, WALL);
    baseLot.addCell(0, 5, WALL);

    for (int i = 2; i <= 10; i++)
    {
        if (i == 2 || i == 5 || i == 7 || i == 10)
        {
            for (int j = 2; j <= 8; j += 3)
            {
                baseLot.addCell(i, j, WALL);
                baseLot.addCell(i, j + 1, WALL);
            }
        }
    }
    for (int i = 3; i <= 4; i++)
    {
        for (int j = 2; j <= 8; j += 3)
        {
            baseLot.addCell(i, j, PARKING_SPACE);
            baseLot.addCell(i, j + 1, PARKING_SPACE);
        }
    }
    for (int i = 8; i <= 9; i++)
    {
        for (int j = 2; j <= 8; j += 3)
        {
            baseLot.addCell(i, j, PARKING_SPACE);
            baseLot.addCell(i, j + 1, PARKING_SPACE);
        }
    }

    // 先收集「所有 PARKING_SPACE」
    vector<pair<int, int>> allSpaces;
    {
        auto &grid = baseLot.getParkingLot();
        for (int r = 0; r < (int)grid.size(); r++)
        {
            for (int c = 0; c < (int)grid[0].size(); c++)
            {
                if (grid[r][c].type == PARKING_SPACE)
                {
                    allSpaces.emplace_back(r, c);
                }
            }
        }
    }

    // 若可用車位 < 20 => break
    int vehicleCount = 20;
    if ((int)allSpaces.size() < vehicleCount)
    {
        cout << "Not enough parking spaces => can't place 20 vehicles.\n";
        return 0;
    }

    // 打亂 => 取得前 20 個
    std::random_device rd;
    std::default_random_engine eng(rd());
    std::shuffle(allSpaces.begin(), allSpaces.end(), eng);

    vector<char> vehicleIDs(vehicleCount);
    vector<pair<int, int>> parkingSpaces(vehicleCount);

    // 產生 20 個不重複車位
    // 也產生 20 個不重複的 vehicleID
    unordered_set<char> usedIDs;
    for (int i = 0; i < vehicleCount; i++)
    {
        char vID;
        do
        {
            vID = 'A' + (rand() % 26);
        } while (usedIDs.find(vID) != usedIDs.end());
        usedIDs.insert(vID);

        vehicleIDs[i] = vID;
        parkingSpaces[i] = allSpaces[i]; // 取前20
    }

    // 建立 Original / Improved
    ParkingLot parkingLotOriginal = baseLot;
    parkingLotOriginal.setUseImprovedAStar(false);

    ParkingLot parkingLotImproved = baseLot;
    parkingLotImproved.setUseImprovedAStar(true);

    // 先執行「傳統 A*」
    cout << "=== Traditional A* Execution ===\n";
    {
        vector<thread> ths;
        for (int i = 0; i < vehicleCount; i++)
        {
            char vID = vehicleIDs[i];
            auto ps = parkingSpaces[i];
            // 執行 addVehicle( row, col, vID, i )
            ths.emplace_back([&](char id, int r, int c, int idx)
                             { addVehicleLogged(parkingLotOriginal, id, r, c, idx); }, vID, ps.first, ps.second, i);

            this_thread::sleep_for(std::chrono::seconds(2));
        }
        for (auto &th : ths)
        {
            th.join();
        }
    }

    // 取結果
    auto timesOrig = parkingLotOriginal.getVehicleTimes();
    auto delayOrig = parkingLotOriginal.getDelayTime();

    // 再執行「改良 A*」
    cout << "\n=== Improved A* Execution ===\n";
    {
        vector<thread> ths;
        for (int i = 0; i < vehicleCount; i++)
        {
            char vID = vehicleIDs[i];
            auto ps = parkingSpaces[i];
            ths.emplace_back([&](char id, int r, int c, int idx)
                             { addVehicleLogged(parkingLotImproved, id, r, c, idx); }, vID, ps.first, ps.second, i);

            this_thread::sleep_for(std::chrono::seconds(2));
        }
        for (auto &th : ths)
        {
            th.join();
        }
    }

    auto timesImpr = parkingLotImproved.getVehicleTimes();
    auto delayImpr = parkingLotImproved.getDelayTime();

    // 分別計算「前10 與 後10」
    // 這裡 "前10" => vehicleIndex < 10; "後10" => vehicleIndex>=10
    auto calcAvgByIndexRange = [&](const vector<VehicleTime> &arr, int startIndex, int endIndex)
    {
        long long sum = 0;
        int count = 0;
        for (auto &vt : arr)
        {
            if (vt.vehicleIndex >= startIndex && vt.vehicleIndex < endIndex)
            {
                sum += vt.time;
                count++;
            }
        }
        return (count > 0) ? (double)sum / count : 0.0;
    };

    double frontTimeOrig = calcAvgByIndexRange(timesOrig, 0, 10);
    double backTimeOrig = calcAvgByIndexRange(timesOrig, 10, 20);

    double frontDelayOrig = calcAvgByIndexRange(delayOrig, 0, 10);
    double backDelayOrig = calcAvgByIndexRange(delayOrig, 10, 20);

    double frontTimeImpr = calcAvgByIndexRange(timesImpr, 0, 10);
    double backTimeImpr = calcAvgByIndexRange(timesImpr, 10, 20);

    double frontDelayImpr = calcAvgByIndexRange(delayImpr, 0, 10);
    double backDelayImpr = calcAvgByIndexRange(delayImpr, 10, 20);

    cout << "\n=== Results (Front10 / Back10) ===\n";
    cout << "(Traditional A*) front10 time=" << frontTimeOrig << ", back10 time=" << backTimeOrig << "\n";
    cout << "(Traditional A*) front10 delay=" << frontDelayOrig << ", back10 delay=" << backDelayOrig << "\n\n";

    cout << "(Improved A*) front10 time=" << frontTimeImpr << ", back10 time=" << backTimeImpr << "\n";
    cout << "(Improved A*) front10 delay=" << frontDelayImpr << ", back10 delay=" << backDelayImpr << "\n";

    cin.get();

    g_assignmentFile.close();
    return 0;
}
