/**
 * Actor can be either Player or Enemey.
 * There is fixed number of step to execute and progress the game. Whenever the game progresses
 * and reaches such target number of steps, then the game stop and evaluate whether player wins or
 * loses.
 *
 * Each step, either Player or Enemy can make a move (which can only be up/right/down/left) or
 * decide to attack opponent. Each attack will reduce opponent's HP by 1. Whoever has HP down to 0
 * first loses.
 *
 * Changes
 *  - the program accepts "-nowait" as parameter to the execution command to not wait in each loop
 *  - undo mechanism is added and checked for correctness at the end
 */
#include <iostream>
#include <cmath>
#include <random>
#include <deque>
#include <chrono>
#include <thread>
#include <memory>
#include <cassert>

enum class ActorId {
    PLAYER = 0,
    ENEMY
};

class Actor {
public:
    Actor(ActorId id_, int hp_)
        : id(static_cast<int>(id_))
        , hp(hp_) {
    }

    void moveBy(int x, int y) {
        posX += x;
        posY += y;
    }

    bool attack(Actor& target, int atkPoint) const {
        // only able to attack if in range of no more than 1 slot
        if (std::abs(target.posX - posX) > 1 ||
            std::abs(target.posY - posY) > 1)
            return false;
        
        target.hp -= atkPoint;
        return true;
    }

public:
    int posX, posY;
    int hp;
    int id;
};

class Command {
public:
    Command()
        : source(nullptr)
        , target(nullptr) {
    }

    virtual ~Command() { }
    virtual void execute() = 0;
    virtual void undo() = 0;
    
    Actor* source;
    Actor* target;
    int id;
};

enum CommandId {
    MOVE = 0,
    ATTACK
};

class MoveUnitCommand: public Command {
public:
    MoveUnitCommand(Actor& actor, int dx_, int dy_)
        : dx(dx_),
          dy(dy_)
    {
        source = &actor;
        id = CommandId::MOVE;
    }

    void execute() override {
        source->moveBy(dx, dy);
    }

    void undo() override {
        source->moveBy(-dx, -dy);
    }

    int getDx() const { return dx; }
    int getDy() const { return dy; } 

private:
    int dx, dy;
};

class AttackCommand: public Command {
public:
    AttackCommand(Actor& source_, Actor& target_, int atkPoint_) {
        source = &source_;
        target = &target_;
        atkPoint = atkPoint_;
        id = CommandId::ATTACK;
    }

    void execute() override {
        isAttackDone = source->attack(*target, atkPoint);
    }

    void undo() override {
        if (isAttackDone)
           source->attack(*target, -atkPoint);     // undo the attack 
    }

    int getAtkPoint() const { return atkPoint; }

private:
    int atkPoint;
    bool isAttackDone;
};

template <typename T>
void printCommandResultStatus(const T* cmd, bool reverse) {
    // not implemented, see the specialized template functions below
}

template <>
void printCommandResultStatus<MoveUnitCommand>(const MoveUnitCommand* cmd, bool reverse) {
    ActorId aid = static_cast<ActorId>(cmd->source->id);
    if (aid == ActorId::PLAYER) {
        if (reverse)
            std::printf("  Player moves by\t(dx=%d, dy=%d)\tto\t(x=%d, y=%d)\n", -cmd->getDx(), -cmd->getDy(), cmd->source->posX, cmd->source->posY);
        else
            std::printf("  Player moves by\t(dx=%d, dy=%d)\tto\t(x=%d, y=%d)\n", cmd->getDx(), cmd->getDy(), cmd->source->posX, cmd->source->posY);
    }
    else if (aid == ActorId::ENEMY) {
        if (reverse)
            std::printf("  Enemy moves by \t(dx=%d, dy=%d)\tto\t(x=%d, y=%d)\n", -cmd->getDx(), -cmd->getDy(), cmd->source->posX, cmd->source->posY);
        else
            std::printf("  Enemy moves by \t(dx=%d, dy=%d)\tto\t(x=%d, y=%d)\n", cmd->getDx(), cmd->getDy(), cmd->source->posX, cmd->source->posY);
    }
}

template <>
void printCommandResultStatus<AttackCommand>(const AttackCommand* cmd, bool reverse) {
    ActorId aid = static_cast<ActorId>(cmd->source->id);
    // TODO: Assume target actor only from source actor, cover all cases...
    if (aid == ActorId::PLAYER) {
        if (reverse)
            std::printf("  Player undo attacks Enemy for\t%d hit point\n", std::abs(cmd->getAtkPoint()));
        else
            std::printf("  Player attacks Enemy for\t%d hit point\n", std::abs(cmd->getAtkPoint()));
    }
    else if (aid == ActorId::ENEMY) {
        if (reverse)
            std::printf("  Enemy undo attacks Player for\t%d hit point\n", std::abs(cmd->getAtkPoint()));
        else
            std::printf("  Enemy attacks Player for\t%d hit point\n", std::abs(cmd->getAtkPoint()));
    }
}

void processAllCommands(std::deque<std::shared_ptr<Command>>& cmdList, int& startIdx) {
    int i = startIdx;
    for (; i<cmdList.size(); ++i) {
        std::shared_ptr<Command> cmdPtr = cmdList[i];

        cmdPtr->execute();

        if (cmdPtr->id == CommandId::MOVE) {
            printCommandResultStatus<MoveUnitCommand>(static_cast<const MoveUnitCommand*>(cmdPtr.get()), false);              
        }
        else if (cmdPtr->id == CommandId::ATTACK) {
            printCommandResultStatus<AttackCommand>(static_cast<const AttackCommand*>(cmdPtr.get()), false);
        }
    }
    startIdx = i;
}

void undoCommands(std::deque<std::shared_ptr<Command>>& cmdList) {
    for (int i=cmdList.size()-1; i>=0; --i) {
        std::shared_ptr<Command> cmdPtr = cmdList[i];
        cmdPtr->undo();

        if (cmdPtr->id == CommandId::MOVE) {
            printCommandResultStatus<MoveUnitCommand>(static_cast<const MoveUnitCommand*>(cmdPtr.get()), true);              
        }
        else if (cmdPtr->id == CommandId::ATTACK) {
            printCommandResultStatus<AttackCommand>(static_cast<const AttackCommand*>(cmdPtr.get()), true);
        }
    }
}

int main(int argc, char* argv[]) {
    bool hasWait = true;
    if (argc >= 2) {
        for (int i=1; i<argc; ++i) {
            std::string str = argv[i];
            if (str.compare("-nowait") == 0) {
                hasWait = false;
                break;
            }
        }
    }

    Actor player(ActorId::PLAYER, 3);
    Actor enemy(ActorId::ENEMY, 1);

    std::deque<std::shared_ptr<Command>> cmdList;

    int theoricalNumSteps = 10;

    std::random_device rd;
    const auto seed = rd();
    std::mt19937 mt(seed);

    int cmdStartIdx = 0;

    std::cout << "Seed: " << seed << '\n';

    std::uniform_int_distribution<int> distBoolean(0,1);
    std::uniform_int_distribution<int> distMovement(0,3);   // 0 -> up, 1 -> right, 2 -> down, 3 -> left

    for (int step=0; step<theoricalNumSteps; ++step) {
        std::cout << "GAME STEP: " << (step+1) << '\n';

        bool isEitherSideAttacked = false;

        // -- Enemy --
        // check for attack
        if (distBoolean(mt) && player.hp > 0) {
            cmdList.push_back(std::move(std::make_shared<AttackCommand>(enemy, player, 1)));
            isEitherSideAttacked = true;
        }
        else {
            int m = distMovement(mt);
            if (m == 0) {
                cmdList.push_back(std::move(std::make_shared<MoveUnitCommand>(enemy, 0, 1)));
            }
            else if (m == 1) {
                cmdList.push_back(std::move(std::make_shared<MoveUnitCommand>(enemy, 1, 0)));
            }
            else if (m == 2) {
                cmdList.push_back(std::move(std::make_shared<MoveUnitCommand>(enemy, 0, -1)));
            }
            else {
                cmdList.push_back(std::move(std::make_shared<MoveUnitCommand>(enemy, -1, 0)));
            }
        }


        // -- Player --
        // check for attack
        if (!isEitherSideAttacked && distBoolean(mt) && enemy.hp > 0) {
            cmdList.push_back(std::move(std::make_shared<AttackCommand>(player, enemy, 1)));
        }
        // check for movement of player
        else {
            int m = distMovement(mt);
            if (m == 0) {
                cmdList.push_back(std::move(std::make_shared<MoveUnitCommand>(player, 0, 1)));
            }
            else if (m == 1) {
                cmdList.push_back(std::move(std::make_shared<MoveUnitCommand>(player, 1, 0)));
            }
            else if (m == 2) {
                cmdList.push_back(std::move(std::make_shared<MoveUnitCommand>(player, 0, -1)));
            }
            else {
                cmdList.push_back(std::move(std::make_shared<MoveUnitCommand>(player, -1, 0)));
            }
        }

        processAllCommands(cmdList, cmdStartIdx);       

        // check status
        if (player.hp <= 0 || enemy.hp <= 0)
            break;

        if (hasWait)
            if (step < theoricalNumSteps-1)
                std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (player.hp > 0 && enemy.hp <= 0) {
        std::cout << "WIN with 2x BONUS (Bonus from killing Enemy)\n";
    }
    else if (player.hp > 0) {
        std::cout << "WIN (Player didn't die)\n";
    }
    else {
        std::cout << "Lose (Player died)\n";
    }
    std::printf("Player: HP=%d, posX=%d, posY=%d\n", player.hp, player.posX, player.posY);
    std::printf("Enemey: HP=%d, posX=%d, posY=%d\n", enemy.hp, enemy.posX, enemy.posY);

    // test undo mechanism
    std::cout << "\nUndo testing\n";
    undoCommands(cmdList);

    assert(player.hp == 3 && "Player should have initially set hp of 3");
    assert(enemy.hp == 1 && "Enemy should have initially set hp of 1");
    assert(player.posX == 0 && player.posY == 0 && "Player should have position set to origin");
    assert(enemy.posX == 0 && enemy.posY == 0 && "Enemy should have position set to origin");

    std::cout << "Undo all checks passed\n";

    return 0;
}
