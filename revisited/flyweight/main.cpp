#include <iostream>
#include <cstdint>
#include <random>

enum class TileType : uint8_t { HILL, GRASS, RIVER, DESTINATION };

struct Vector3 {
    float x;
    float y;
    float z;
};

// -- flyweight structure -- //
class TerrainData {
    std::string texturePath;            // fictional
    std::string materialPath;           // fictional
    std::vector<Vector3> vertices;      // fictional
};

class Tile {
public:
    Tile(TileType type, int movementCost, const TerrainData& terrainData):
        m_terrainData(const_cast<TerrainData*>(&terrainData))
        , m_movementCost(movementCost)
        ,  m_type(type) {
    }
    
    int getMovementCost() const { return m_movementCost; }
    bool isWater() const { return m_type == TileType::RIVER; }
    TileType getType() const { return m_type; }

private:
    TerrainData* m_terrainData;
    int m_movementCost;
    TileType m_type;
};
// -- end of flyweight structure -- //

class World {
public:
    static const int GRID_WIDTH = 10;
    static const int GRID_HEIGHT = 10;

    World(std::mt19937& mt) {
        std::uniform_int_distribution<int> distGridCol(0,GRID_WIDTH-1);
        std::uniform_int_distribution<int> distGridRow(0,GRID_HEIGHT-1);

        // set 2 destination tiles
        int waterRows[2];
        int waterCols[2];
        for (int i=0; i<2; ++i) {
            waterRows[i] = distGridRow(mt);
            waterCols[i] = distGridCol(mt);
            grid[waterRows[i]][waterCols[i]] = TileType::DESTINATION;
        }

        // set river
        std::uniform_int_distribution<int> dist2(0, 1);
        if (dist2(mt)) {        // horizontal river
            int riverRow = distGridRow(mt);
            while (riverRow == waterRows[0] ||
                   riverRow == waterRows[1]) {
                riverRow = distGridRow(mt);
            }

            for (int col=0; col<GRID_WIDTH; ++col) {
                grid[riverRow][col] = TileType::RIVER;
            }
        }
        else {                  // vertical river
            int riverCol = distGridRow(mt);
            while (riverCol == waterCols[0] ||
                   riverCol == waterCols[1]) {
                riverCol = distGridRow(mt);
            }

            for (int row=0; row<GRID_HEIGHT; ++row) {
                grid[row][riverCol] = TileType::RIVER;
            }
        }

        // 25% to have hill
        // the less is for grass
        std::uniform_int_distribution<int> dist4(0, 3);
        for (int row=0; row<GRID_HEIGHT; ++row) {
            for (int col=0; col<GRID_WIDTH; ++col) {
                if (grid[row][col] != TileType::DESTINATION &&
                    grid[row][col] != TileType::RIVER) {
                    if (dist4(mt) == 0) {
                        grid[row][col] = TileType::HILL;          // ok to incur cost of casting; once, just once at startup time
                    }
                    else {      // for the 75% (less of chance) for grass
                        grid[row][col] = TileType::GRASS;
                    }
                }
            }
        }
    }

    void printGridLayout() const {
        for (int row=0; row<GRID_HEIGHT; ++row) {
            for (int col=0; col<GRID_WIDTH; ++col) {
                if (grid[row][col] == TileType::GRASS) {
                    std::cout << "+";
                }
                else if (grid[row][col] == TileType::RIVER) {
                    std::cout << "~";
                }
                else if (grid[row][col] == TileType::HILL) {
                    std::cout << "%";
                }
                else {
                    std::cout << "X";
                }
                std::cout << "|";
            }
            std::cout << "\n";
        }
    }

    const Tile& getTile(int row, int col) const {
        // trade-off using pointer to point to Tile via the following quick check of type
        // then return corresponding Tile. Avoid dereferencing of pointer.
        //
        // Anyway, we can opt-in using dereferencing by storing pointer to Tile in World, thus no
        // conditional type checking here.
        TileType type = grid[row][col];
        if (type == TileType::HILL) {
            return hillTile;
        }
        else if (type == TileType::GRASS) {
            return grassTile;
        }
        else if (type == TileType::RIVER) {
            return riverTile;
        }
        else {
            return destinationTile;
        }
    }
    
private:
    TileType grid[GRID_HEIGHT][GRID_WIDTH];

    TerrainData hillTerrainData;
    TerrainData grassTerrainData;
    TerrainData riverTerrainData;
    TerrainData destinationTerrainData;

    Tile hillTile {TileType::HILL, 100, hillTerrainData};
    Tile grassTile {TileType::GRASS, 0, grassTerrainData};
    Tile riverTile {TileType::RIVER, 50, riverTerrainData};
    Tile destinationTile {TileType::DESTINATION, 0, destinationTerrainData};
};

int main() {
    // share the randomizer with world, and state progressing
    std::random_device rd;
    auto seed = rd();
    std::mt19937 mt(seed);

    std::cout << "SEED: " << seed << '\n';

    World world(mt);
    world.printGridLayout();

    const int N = 10; 
    int nCount = 0;
    int costAccum = 0;
    bool isWin = false;
    
    // start at the upper-left corner of the grid
    int playerPosX = 0;
    int playerPosY = 0;

    // 0 -> up
    // 1 -> right
    // 2 -> down
    // 3 -> left
    std::uniform_int_distribution<int> distDir(0, 3);

    while (nCount < N) {
        int randDir = distDir(mt);
        bool actionTaken = false;

        if (randDir == 0 && playerPosY > 0) {
            --playerPosY;
            ++nCount;
            actionTaken = true;
            std::cout << "Move up  ";
        }
        else if (randDir == 1 && playerPosX < World::GRID_WIDTH-1) {
            ++playerPosX;
            ++nCount;
            actionTaken = true;
            std::cout << "Move right";
        }
        else if (randDir == 2 && playerPosX < World::GRID_HEIGHT-1) {
            ++playerPosY;
            ++nCount;
            actionTaken = true;
            std::cout << "Move down";
        }
        else if (randDir == 3 && playerPosX > 0) {
            --playerPosX;
            ++nCount;
            actionTaken = true;
            std::cout << "Move left";
        }

        if (actionTaken) {
            const Tile& tile = world.getTile(playerPosY, playerPosX);
            costAccum += tile.getMovementCost();

            if (tile.getType() == TileType::DESTINATION) {
                isWin = true;
                std::cout << "\t- Reached DESTINATION\n";
                break;
            }
            else if (tile.getType() == TileType::HILL) {
                std::cout << "\t- currently on Hill\n";
            }
            else if (tile.getType() == TileType::GRASS) {
                std::cout << "\t- currently on Grass\n";
            }
            else if (tile.getType() == TileType::RIVER) {
                std::cout << "\t- currently on River\n";
            }
        }
    }

    if (isWin) {
        std::cout << "Win (cost: " << costAccum << ")\n";
    }
    else {
        std::cout << "Lose (cost: " << costAccum << ")\n";
    }
    return 0;
}
