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
#include <algorithm>
#include <condition_variable>
#include <map>
#include <functional> // 新增此行以使用 std::function

using namespace std;
using namespace std::chrono;

enum CellType { ENTRANCE, AISLE, WALL, PARKING_SPACE, VEHICLE, CLOSED_AISLE };

struct Cell {
    int row, col;
    CellType type;
    int waitTime = 0;
    char vehicleID;
    bool isMoving = true;
    Cell(int r = 0, int c = 0, CellType t = AISLE) : row(r), col(c), type(t) {}
};

struct VehicleTime {
    char vehicleID;
    long long time;
    VehicleTime(char id, long long t) : vehicleID(id), time(t) {}
};

class ParkingLot {
public:
    static const int MAX_ROWS = 17;
    static const int MAX_COLS = 24;

    // 全域事件相關
    static atomic<bool> eventTriggered;
    static int closedCellRow;
    static int closedCellCol;
    static mutex replanMtx;

    struct AffectedVehicleInfo {
        char vehicleID;
        vector<pair<int,int>> remainingPath;
        int remainingLen;
        pair<int,int> currentPos; 
        int endRow;
        int endCol;

        AffectedVehicleInfo(char v, const vector<pair<int,int>>& p, int l, pair<int,int> c, int er, int ec)
            : vehicleID(v), remainingPath(p), remainingLen(l), currentPos(c), endRow(er), endCol(ec) {}
    };

    static vector<AffectedVehicleInfo> affectedVehicles;
    static condition_variable replanCV;

private:
    Cell parkingLot[MAX_ROWS][MAX_COLS];
    vector<VehicleTime> vehicleTimes;
    mutex mtx;
    atomic<long long> lastDisplayTime;
    map<char, pair<int,int>> vehicleDestinations;

    // isCellValid需修改，以配合CLOSED_AISLE不可通行
    bool isCellValid(int row, int col) const {
        return row >= 0 && row < MAX_ROWS && col >= 0 && col < MAX_COLS &&
               (parkingLot[row][col].type == AISLE ||
                (parkingLot[row][col].type == VEHICLE && parkingLot[row][col].isMoving) ||
                parkingLot[row][col].type == ENTRANCE);
    }

    void moveVehicleImpl(vector<pair<int, int>>& path, char vehicleID) {
        if (path.empty()) return;
        auto startTime = steady_clock::now();

        parkingLot[path.front().first][path.front().second].isMoving = true;

        parkingLot[path[0].first][path[0].second].type = VEHICLE;
        parkingLot[path[0].first][path[0].second].vehicleID = vehicleID;
        parkingLot[path[0].first][path[0].second].isMoving = true;
        int wtSum = 0;
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            if(parkingLot[path[i].first][path[i].second].waitTime != 0){
                int adj = parkingLot[path[i].first][path[i].second].waitTime - (int)i;
                wtSum += std::max(adj, 0);
            }
        }
        parkingLot[path.back().first][path.back().second].waitTime = (int)path.size() + 9 + wtSum;

        for (size_t i = 1; i < path.size();) {
            if (parkingLot[path[i].first][path[i].second].isMoving) {
                mtx.lock();
                parkingLot[path[i - 1].first][path[i - 1].second].vehicleID = ' ';
                parkingLot[path[i].first][path[i].second].vehicleID = vehicleID;
                parkingLot[path[i - 1].first][path[i - 1].second].type = AISLE;
                parkingLot[path[i - 1].first][path[i - 1].second].isMoving = true;
                parkingLot[path[i].first][path[i].second].type = VEHICLE;
                parkingLot[path[i].first][path[i].second].isMoving = true;
                mtx.unlock();
                path.erase(path.begin());
            }
            else{
                parkingLot[path[i-1].first][path[i-1].second].isMoving = false;
            }

            int wtSum = 0;
            for (size_t k = 0; k + 1 < path.size(); ++k) {
                if(parkingLot[path[k].first][path[k].second].waitTime != 0){
                int adj = parkingLot[path[k].first][path[k].second].waitTime - (int)k;
                wtSum += std::max(adj, 0);
                }
            }
            parkingLot[path.back().first][path.back().second].waitTime = (int)path.size() + 9 + wtSum;

            this_thread::sleep_for(chrono::seconds(1));
            displayStatus();

            // 事件觸發檢查
            if (eventTriggered.load()) {
                // 檢查是否受影響
                if (isVehicleAffectedByClosedCell(path)) {
                    lock_guard<mutex> lk(replanMtx);
                    // 在 moveVehicleImpl 偵測到事件並受影響處:
                    auto it = vehicleDestinations.find(vehicleID);
                    if (it != vehicleDestinations.end()) {
                        int originalEndRow = it->second.first;
                        int originalEndCol = it->second.second;
                        affectedVehicles.emplace_back(vehicleID, path, (int)path.size(), path[0], originalEndRow, originalEndCol);
                    } else {
                        // 找不到目標位置的錯誤處理
                    }
                    return; // 中斷moveVehicle
                }
            }

            if (path.size() == 1){
                for (int j = 9; j >= 0; j--){
                    parkingLot[path.back().first][path.back().second].waitTime--;
                    parkingLot[path.back().first][path.back().second].isMoving = false;
                    if (j == 0 || islower(vehicleID)){
                        j = 0;
                        parkingLot[path.back().first][path.back().second].type = AISLE;
                        parkingLot[path.back().first][path.back().second].waitTime = 0;
                        //parkingLot[path.back().first][path.back().second].vehicleID = ' ';
                        parkingLot[path.back().first][path.back().second].isMoving = true;
                    }
                    this_thread::sleep_for(chrono::seconds(1));
                    displayStatus();
                }
            }
        }

        lock_guard<mutex> lock(mtx);
        auto endTime = steady_clock::now();
        auto duration = duration_cast<seconds>(endTime - startTime).count();
        vehicleTimes.emplace_back(vehicleID, duration);
    }

    // 原本的aStar改用std::function作為參數
    bool aStarWithReturn(int startRow, int startCol, int endRow, int endCol, char vehicleID,
                         pair<int,int> noGoCell, bool allowUturn,
                         std::function<void(vector<pair<int,int>>&, char)> moveVehicleCallback) {

        struct Node {
            int row, col, g, h;
            vector<pair<int,int>> path;
            int f() const { return g + h; }
            bool operator>(const Node& other) const { return f() > other.f(); }
        };

        auto heuristic = [&](int sr, int sc, int er, int ec) {
            return abs(er - sr) + abs(ec - sc);
        };

        priority_queue<Node, vector<Node>, greater<Node>> pq;
        vector<vector<int>> cost(MAX_ROWS, vector<int>(MAX_COLS, INT_MAX));
        cost[startRow][startCol] = 0;
        int hh = heuristic(startRow, startCol, endRow, endCol);

        {
            Node startNode{startRow, startCol, 0, hh};
            startNode.path.emplace_back(startRow, startCol);
            pq.push(startNode);
        }

        while (!pq.empty()) {
            Node current = pq.top();
            pq.pop();

            if (current.row == endRow && current.col == endCol) {
                moveVehicleCallback(current.path, vehicleID);
                return true;
            }

            const int dr[] = {-1, 1, 0, 0};
            const int dc[] = {0, 0, -1, 1};

            for (int i = 0; i < 4; ++i) {
                int newRow = current.row + dr[i];
                int newCol = current.col + dc[i];

                if (newRow == noGoCell.first && newCol == noGoCell.second && !allowUturn) {
                    continue;
                }

                if (newRow >=0 && newRow < MAX_ROWS && newCol>=0 && newCol < MAX_COLS) {
                    CellType t = parkingLot[newRow][newCol].type;
                    if (t == CLOSED_AISLE || t == WALL || t == PARKING_SPACE) continue;
                    //if (!isCellValid(newRow, newCol)) continue;
                    int baseG = current.g + 1;
                    int extra = 0;
                    if (parkingLot[newRow][newCol].waitTime > 0 && isupper((unsigned char)vehicleID)) {
                        extra = std::max(parkingLot[newRow][newCol].waitTime - baseG, 0);
                    }
                    int newG = baseG + extra;                    
                    if (newG < cost[newRow][newCol]) {
                        cost[newRow][newCol] = newG;
                        int hVal = heuristic(newRow, newCol, endRow, endCol);
                        Node newNode{newRow, newCol, newG, hVal};
                        newNode.path = current.path;
                        newNode.path.emplace_back(newRow, newCol);
                        pq.push(newNode);
                    }
                }
            }
        }
        cout << "No valid path found.\n";
        return false;
    }

public:
    ParkingLot() {
        for (int i = 0; i < MAX_ROWS; ++i) {
            for (int j = 0; j < MAX_COLS; ++j) {
                parkingLot[i][j] = Cell(i, j, AISLE);
            }
        }
        lastDisplayTime.store(0);
    }

    void setCellType(int r, int c, CellType t) {
        lock_guard<mutex> lk(mtx);
        parkingLot[r][c].type = t;
    }

    void addCell(int row, int col, CellType type) {
        parkingLot[row][col].type = type;
    }

    const Cell (*getParkingLot() const)[MAX_COLS] {
        return parkingLot;
    }

    vector<VehicleTime> getVehicleTimes() const {
        return vehicleTimes;
    }

    bool addVehicle(int row, int col, char vehicleID) {
        if (parkingLot[row][col].type == PARKING_SPACE) {
            parkingLot[row][col].type = VEHICLE;
            parkingLot[row][col].vehicleID = vehicleID;
            parkingLot[row][col].isMoving = false;

            // 將該車輛的目標位置記錄下來 (row,col)為此車的最終停車位置
            

            int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (auto &dir : directions) {
                int newRow = row + dir[0], newCol = col + dir[1];
                if (isCellValid(newRow, newCol)) {
                    vehicleDestinations[vehicleID] = {newRow, newCol};
                    auto mvCallback = [&](vector<pair<int,int>>& p, char vID){ moveVehicleImpl(p,vID); };
                    bool res = aStarWithReturn(0, 8, newRow, newCol, vehicleID, {-1,-1}, true, mvCallback);
                    return res;
                }
            }
            cout << "No valid aisle adjacent to the parking space.\n";
            return false;
        } else {
            cout << "This is not a parking space.\n";
            return false;
        }
    }

    bool removeVehicle(int row, int col, char vehicleID) {
        if (parkingLot[row][col].type == VEHICLE) {
            parkingLot[row][col].type = PARKING_SPACE;

            // 離開停車場的目標是(0,8)
            vehicleDestinations[vehicleID] = {0, 8};

            int directions[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (auto &dir : directions) {
                int newRow = row + dir[0], newCol = col + dir[1];
                if (isCellValid(newRow, newCol)) {
                    auto mvCallback = [&](vector<pair<int,int>>& p, char vID){ moveVehicleImpl(p,vID); };
                    bool res = aStarWithReturn(newRow, newCol, 0, 8, vehicleID, {-1,-1}, true, mvCallback);
                    return res;
                }
            }
        } else {
            cout << "No vehicle at this location.\n";
            return false;
        }
        return false; // 補上return避免警告
    }


    pair<int, int> getRandomParkingSpace() {
        srand((unsigned)time(nullptr));
        vector<pair<int, int>> availableSpaces;

        for (int i = 0; i < MAX_ROWS; ++i) {
            for (int j = 0; j < MAX_COLS; ++j) {
                if (parkingLot[i][j].type == PARKING_SPACE) {
                    availableSpaces.emplace_back(i, j);
                }
            }
        }

        return !availableSpaces.empty() ? availableSpaces[rand() % availableSpaces.size()] : make_pair(-1, -1);
    }

    void displayStatus() {
        static std::atomic_flag isDisplaying = ATOMIC_FLAG_INIT;

        if (isDisplaying.test_and_set()) {
            return;
        }

        try {
            auto now = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
            long long timeSinceLastDisplay = now - lastDisplayTime.load();

            if (timeSinceLastDisplay >= 1000) {
                lastDisplayTime.store(now);

                system("cls");

                cout << "  ";
                for (int i = 0; i < MAX_COLS; ++i) cout << i % 10 << " ";
                cout << "\n";
                for (int i = 0; i < MAX_ROWS; ++i) {
                    cout << i % 10 << " ";
                    for (int j = 0; j < MAX_COLS; ++j) {
                        char displayChar;

                        if (parkingLot[i][j].waitTime > 0) {
                            displayChar = '0' + (parkingLot[i][j].waitTime % 10);
                        } else {
                            switch (parkingLot[i][j].type) {
                                case ENTRANCE: displayChar = ' '; break;
                                case AISLE: displayChar = ' '; break;
                                case WALL: displayChar = '+'; break;
                                case PARKING_SPACE: displayChar = '-'; break;
                                case VEHICLE: displayChar = parkingLot[i][j].vehicleID; break;
                                case CLOSED_AISLE: displayChar = '#'; break;
                            }
                        }

                        cout << displayChar << " ";
                    }

                    cout << "\n";
                }
                cout << "\n";
            }
        } catch (...) {
            isDisplaying.clear();
            throw;
        }

        isDisplaying.clear();
    }

    bool isVehicleAffectedByClosedCell(const vector<pair<int,int>>& path) {
        for (auto &p : path) {
            if (p.first == closedCellRow && p.second == closedCellCol) {
                return true;
            }
        }
        return false;
    }

    bool replanForVehicleWithReturn(AffectedVehicleInfo &avi) {
        int startRow = avi.remainingPath[0].first;
        int startCol = avi.remainingPath[0].second;
        int endRow = avi.endRow;
        int endCol = avi.endCol;
        char vID = avi.vehicleID;
        pair<int,int> noGoCell = avi.currentPos;

        // 如果 endRow,endCol是停車位或不可通行的格，重新找出相鄰AISLE作為新終點
        if (parkingLot[endRow][endCol].type == PARKING_SPACE) {
            bool foundAisle = false;
            int directions[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
            for (auto &dir : directions) {
                int newRow = endRow + dir[0];
                int newCol = endCol + dir[1];
                if (isCellValid(newRow, newCol)) {
                    endRow = newRow;
                    endCol = newCol;
                    foundAisle = true;
                    break;
                }
            }
            if (!foundAisle) {
                cout << "No valid aisle adjacent to the parking space end target.\n";
                return false;
            }
        }

        auto mvCallback = [&](vector<pair<int,int>>& p, char vID){ moveVehicleImpl(p,vID); };

        // 第一次嘗試，不允許迴轉
        bool success = aStarWithReturn(startRow, startCol, endRow, endCol, vID, noGoCell, false, mvCallback);
        if (!success) {
            // 第二次嘗試，允許迴轉
            success = aStarWithReturn(startRow, startCol, endRow, endCol, vID, noGoCell, true, mvCallback);
        }
        return success;
    }



    // 重新規劃的邏輯由外部replanVehicles函式呼叫本函式
};

// 靜態成員初始化
atomic<bool> ParkingLot::eventTriggered(false);
int ParkingLot::closedCellRow = -1;
int ParkingLot::closedCellCol = -1;
mutex ParkingLot::replanMtx;
vector<ParkingLot::AffectedVehicleInfo> ParkingLot::affectedVehicles;
condition_variable ParkingLot::replanCV;

void removeRandomVehicle(ParkingLot& parkingLot) {
    srand((unsigned)time(nullptr));
    vector<pair<int, int>> vehiclePositions;

    // 使用getParkingLot()存取
    auto lot = parkingLot.getParkingLot();
    for (int i = 0; i < ParkingLot::MAX_ROWS; ++i) {
        for (int j = 0; j < ParkingLot::MAX_COLS; ++j) {
            if (lot[i][j].type == VEHICLE && isalpha(lot[i][j].vehicleID)) {
                vehiclePositions.emplace_back(i, j);
            }
        }
    }

    if (!vehiclePositions.empty()) {
        auto vehiclePos = vehiclePositions[rand() % vehiclePositions.size()];
        char vehicleID = lot[vehiclePos.first][vehiclePos.second].vehicleID;
        cout << "Vehicle " << vehicleID << " is leaving the parking lot.\n";
        parkingLot.removeVehicle(vehiclePos.first, vehiclePos.second, vehicleID);
    }
}

void addVehicleWithRandomSpace(ParkingLot& parkingLot, char vehicleID) {
    pair<int, int> parkingSpace = parkingLot.getRandomParkingSpace();
    if (parkingSpace.first != -1) {
        cout << "Vehicle " << vehicleID << " is entering the parking lot.\n";
        parkingLot.addVehicle(parkingSpace.first, parkingSpace.second, vehicleID);
    }
}

void addRandomVehicle(ParkingLot& parkingLot, char vehicleID) {
    addVehicleWithRandomSpace(parkingLot, vehicleID);
}
/*
void triggerEvent(ParkingLot &parkingLot) {
    this_thread::sleep_for(chrono::seconds(20));
    vector<pair<int,int>> candidates;

    auto lot = parkingLot.getParkingLot();
    for (int i = 0; i < ParkingLot::MAX_ROWS; ++i) {
        for (int j = 0; j < ParkingLot::MAX_COLS; ++j) {
            if (lot[i][j].type == AISLE && lot[i][j].waitTime == 0) {
                candidates.emplace_back(i,j);
            }
        }
    }

    if (!candidates.empty()) {
        srand((unsigned)time(nullptr));
        auto chosen = candidates[rand() % candidates.size()];


        // 使用公開方法來修改CellType
        parkingLot.setCellType(chosen.first, chosen.second, CLOSED_AISLE);

        ParkingLot::closedCellRow = chosen.first;
        ParkingLot::closedCellCol = chosen.second;

        ParkingLot::eventTriggered.store(true);
        cout << "Event triggered! Cell (" << chosen.first << "," << chosen.second << ") is now CLOSED_AISLE.\n";
    }
}
*/
void triggerEvent(ParkingLot &parkingLot) {
    this_thread::sleep_for(chrono::seconds(5));
    int chosenRow = 1; // 手動指定列
    int chosenCol = 10; // 手動指定行

    parkingLot.setCellType(chosenRow, chosenCol, CLOSED_AISLE);

    ParkingLot::closedCellRow = chosenRow;
    ParkingLot::closedCellCol = chosenCol;

    ParkingLot::eventTriggered.store(true);
    cout << "Event triggered! Cell (" << chosenRow << "," << chosenCol << ") is now CLOSED_AISLE.\n";
}


void replanVehicles(ParkingLot &parkingLot) {
    while (true) {
        this_thread::sleep_for(chrono::seconds(1));
        if (ParkingLot::eventTriggered.load()) {
            lock_guard<mutex> lk(ParkingLot::replanMtx);
            if (!ParkingLot::affectedVehicles.empty()) {
                sort(ParkingLot::affectedVehicles.begin(), ParkingLot::affectedVehicles.end(), 
                     [](const ParkingLot::AffectedVehicleInfo &a, const ParkingLot::AffectedVehicleInfo &b){
                         return a.remainingLen < b.remainingLen;
                     });
                while (!ParkingLot::affectedVehicles.empty()) {
                    auto avi = ParkingLot::affectedVehicles.front();
                    ParkingLot::affectedVehicles.erase(ParkingLot::affectedVehicles.begin());
                    cout << "Replanning for vehicle " << avi.vehicleID << "...\n";
                    bool success = parkingLot.replanForVehicleWithReturn(avi);
                    if (!success) {
                        cout << "Vehicle " << avi.vehicleID << " could not find a path even after allowing U-turn.\n";
                    }

                }
            }
        }
    }
}

int main() {
    ParkingLot parkingLot;

    parkingLot.addCell(0, 0, WALL);
    parkingLot.addCell(0, 23, WALL);
    parkingLot.addCell(16, 0, WALL);
    parkingLot.addCell(16, 23, WALL);

    for (int i = 1; i <= 22; ++i) {
        parkingLot.addCell(0, i, PARKING_SPACE);
        parkingLot.addCell(16, i, PARKING_SPACE);
    }
    for (int i = 1; i <= 15; ++i) {
        parkingLot.addCell(i, 0, PARKING_SPACE);
        parkingLot.addCell(i, 23, PARKING_SPACE);
    }
    parkingLot.addCell(0, 8, AISLE);

    for (int i = 2; i <= 14; i++ ) {
        if(i==2 || i==7 || i==9 || i==14){
            for (int j = 2; j <= 20; j += 3) {
                parkingLot.addCell(i, j, WALL);
                parkingLot.addCell(i, j + 1, WALL);
            }
        }
    }

    for (int i = 3; i <= 6; i++) {
        for (int j = 2; j <= 20; j += 3) {
            parkingLot.addCell(i, j, PARKING_SPACE);
            parkingLot.addCell(i, j + 1, PARKING_SPACE);
        }
    }

    for (int i = 10; i <= 13; i++) {
        for (int j = 2; j <= 20; j += 3) {
            parkingLot.addCell(i, j, PARKING_SPACE);
            parkingLot.addCell(i, j + 1, PARKING_SPACE);
        }
    }

    parkingLot.displayStatus();

    srand((unsigned)time(nullptr));
    unordered_set<char> usedIDs;
    vector<thread> threads;

    int vehicleCount = rand() % 6 + 15;

    thread eventThread(triggerEvent, ref(parkingLot));
    thread replanThread(replanVehicles, ref(parkingLot));

    for (int i = 0; i < vehicleCount; ++i) {
        char vehicleID;
        do {
            vehicleID = 'A' + (char)(rand() % 26);
        } while (usedIDs.find(vehicleID) != usedIDs.end());
        usedIDs.insert(vehicleID);
        int action = 0; 
        if (action < 1) {
            threads.emplace_back(addRandomVehicle, ref(parkingLot), vehicleID);
        } else {
            threads.emplace_back(removeRandomVehicle, ref(parkingLot));
        }
        int interval = rand() % 2 + 2;
        this_thread::sleep_for(chrono::seconds(interval));
    }

    for (auto& th : threads) {
        th.join();
    }

    vector<VehicleTime> times = parkingLot.getVehicleTimes();
    for (const auto& vt : times) {
        cout << "Vehicle " << vt.vehicleID << " move time: " << vt.time << " seconds" << endl;
    }

    // eventThread, replanThread在此示範不特別join或結束，實務可加條件中斷。

    return 0;
}
