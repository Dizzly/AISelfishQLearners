#ifndef QLEARNER_H_INCLUDED
#define QLEARNER_H_INCLUDED


#undef output //bullet overrides the word output
#include "Fann\src\include\fann.h"
#include "SmartPointer.h"
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string>
#include<fstream>



enum HERO_INDEX{
    NONE = -1,
    DEEPS,
    HOTS,
    TANK,
    TOO_HIGH,
};

struct Hero
{
    Hero() :current_health(0), max_health(0), damage(0){}
    Hero(int max, int dmg, int heal) :max_health(max), current_health(max), damage(dmg), healing(heal){}
    bool operator ==(const Hero& h)
    {
        if (current_health != h.current_health)
        {
            int i = 0;
        }
        return(current_health == h.current_health&&
            max_health == h.max_health&&
            damage == h.damage);
    }
    int current_health;
    int max_health;

    int healing;
    int damage;
};

struct Enemy :RefCounted
{
    Enemy() :Enemy(9,3, rand() % 3,true)
    {}
    Enemy(int max, int dmg, int target = NONE, bool isSick = true) :max_health(max), current_health(max), damage(dmg), hero_target_index(target), sick(isSick){}
    bool operator!=(const Enemy& en)
    {
        return !(current_health == en.current_health&&
            max_health == en.max_health&&
            damage == en.damage&&
            sick == en.sick&&
            hero_target_index == en.hero_target_index);
    }
    Enemy(const Enemy& en)
    {
        current_health = en.current_health;
        max_health = en.max_health;
        damage = en.damage;
        sick = en.sick;
        if (en.hero_target_index == NONE)
        {
            hero_target_index = rand() % 3;
        }
        else
        {
            hero_target_index = en.hero_target_index;
        }
    
    }
    int current_health;
    int max_health;

    int damage;
    bool sick;

    int hero_target_index;
};
typedef SmartPointer<Enemy> EnemySP;

struct QState
{
public:
    float agent_health_percent;

    int enemies_targeting_agent;
    int enemies_total;

    int enemies_lowest_health;
    float enemies_average_health;

    int number_of_allies;
    int proximity_to_exit;

    int allies_lowest_health;
    float allies_average_health;
};

struct State
{
public:
    Hero heros[3];
    std::vector<EnemySP> enemies;
    State()
    {}
    State(const State& state)
    {
        number_of_allies = state.number_of_allies;
        exit_distance = state.exit_distance;
        proximity_to_exit = state.proximity_to_exit;
        memcpy(&heros[0], &state.heros[0], sizeof(Hero) * 3);
        enemies.clear();
        for (int i = 0; i < state.enemies.size(); ++i)
        {
            enemies.push_back(new Enemy(*state.enemies[i]));
        }
    }

    //data variables containing data representing the scene
    int number_of_allies;

    int exit_distance;
    int proximity_to_exit;


    //logic variables, only set at initialisation time, should not use as part of rewards
    // or comparison with other states
    int num_enemies_spawn_per_room;
    bool enemies_spawn_sick;

    Enemy prefab;


    bool operator==(const State& st)
    {

        bool same = true;

        same == number_of_allies == st.number_of_allies&&
            exit_distance == st.exit_distance&&
            proximity_to_exit == st.proximity_to_exit;

        if (!same)
        {
            return false;
        }


        same = heros[0] == st.heros[0] &&
            heros[1] == st.heros[1] &&
            heros[2] == st.heros[2];

        if (!same)
        {
            return false;
        }
        
        same = enemies.size() == st.enemies.size();

        if (!same)
        {
            return false;
        }

        for (int i = 0; i < enemies.size(); ++i)
        {
            if (*enemies[i] != *st.enemies[i])
            {
                return false;
            }
        }

        return true;
    }

    void Init(int enemyDamage, int enemyHealth, int enemiesPerRoom,bool spawnSick,
        int exitDist)
    {
        number_of_allies = 3;
        exit_distance = exitDist;
        proximity_to_exit = exit_distance;
        heros[DEEPS] = Hero(12, 7,3);
        heros[HOTS] = Hero(10, 2,7);
        heros[TANK] = Hero(20, 4,3);
        num_enemies_spawn_per_room = enemiesPerRoom;
        enemies_spawn_sick = spawnSick;
        prefab.current_health = prefab.max_health = enemyHealth;
        prefab.damage = enemyDamage;
        prefab.sick = spawnSick;
        prefab.hero_target_index = NONE;
        enemies.clear();
        for (int i = 0; i < num_enemies_spawn_per_room; ++i)
        {
            enemies.push_back(new Enemy(prefab));
        }
    }

    void DumpToFile(std::fstream& file)
    {
        char buff[200];
        sprintf(buff,"Deeps :HP(%i) | Hots :HP(%i) |  Tank :HP(%i)\n", 
            heros[DEEPS].current_health, heros[HOTS].current_health, heros[TANK].current_health);

        file.write(buff, std::strlen(buff));

        for (int i = 0; i < enemies.size(); ++i)
        {
            sprintf(buff,"Enemy%i :HP(%i) Target(%i)\n", i, enemies[i]->current_health, enemies[i]->hero_target_index);
            file.write(buff, std::strlen(buff));
        }
    
        file.write("End of scene\n",13);
    }


    void Simulate()
    {
        if (proximity_to_exit)
        {
            int numDead = 0;
            for (int i = 0; i < 3; ++i)
            {
                if (heros[i].current_health <= 0)
                {
                    numDead++;
                }
            }
            number_of_allies = 3 - numDead;
            if (number_of_allies == 0)
            {
                return;
            }

            for (int i = 0; i < enemies.size(); ++i)
            {
                if (enemies[i]->sick == false)
                {
                    if (heros[enemies[i]->hero_target_index].current_health <= 0)
                    {
                        int target = GetAllyWithLowestHealth();
                        if (target != NONE)
                        {
                            enemies[i]->hero_target_index = target;
                        }
                    }

                    heros[enemies[i]->hero_target_index].current_health -= enemies[i]->damage;
                  
                }
                else
                {
                    enemies[i]->sick = false;
                }
            }
        }
    }

    float GetRewardDiff(const State& nextState, int heroIndex) const
    {
        const Hero* startHero = &heros[heroIndex];
        const Hero* nextHero = &nextState.heros[heroIndex];
        float totalReward = 0;


        
        //Character health reward/punishment
        float startFract = (float)startHero->current_health / startHero->max_health;
        float endFract = (float)nextHero->current_health / nextHero->max_health;

        //reward/punish 0.1*inverseHealth for every 10% of health lost
        totalReward += (endFract - startFract) * (1 - startFract);
        //with an extra -1 if the action kills the character
        if (nextHero->current_health <= 0)
        {
            totalReward -= 1;
        }
        //END

        //Enemy numbers reward/punishment
         
        int startNumEnemies = enemies.size();
        int endNumEnemies = nextState.enemies.size();
        //punishment and reward per enemy added to scenario
        if (endNumEnemies > startNumEnemies)
        {
            //punishment
            totalReward += (startNumEnemies - endNumEnemies)*0.2;
        }
        else if(endNumEnemies < startNumEnemies)
        {
            //reward
            totalReward += (startNumEnemies - endNumEnemies)*0.7;
        }
        //END

        //Agro (Number of enemies targeting character) reward/punishment
        int startEnemeyAgro = GetNumEnemiesTargetingHero(heroIndex);
        int endEnemyAgro = nextState.GetNumEnemiesTargetingHero(heroIndex);

        //if less then 50% health, punish more sevealy
        if (endFract <= 0.5f)
        {
            totalReward += (startEnemeyAgro - endEnemyAgro)*0.6;
        }
        else
        {
            totalReward += (startEnemeyAgro - endEnemyAgro)*0.15;
        }

        if (proximity_to_exit == 0)
        {
            //big payout at the end
            totalReward += 2;
        }

        if (totalReward != totalReward)
        {
            assert(0); //checking for NAN reward.
        }
        if (totalReward < 0)
        {
            totalReward = 0;
        }
        return totalReward;
    }

    int GetNumEnemiesTargetingHero(int heroIndex)const
    {
        if (enemies.size() == 0)
        {
            return 0;
        }
        int numOfEnemies = 0;
        for (int i = 0; i < enemies.size(); ++i)
        {
            if (enemies[i]->hero_target_index == heroIndex)
            {
                numOfEnemies++;
            }

        }
        return numOfEnemies;
    }

    int GetEnemyLowestHealth()const
    {
        if (enemies.size() == 0)
        {
            return 0;
        }
        int lowestHealth = ~0;
        for (int i = 0; i < enemies.size(); ++i)
        {
            if (enemies[i]->current_health < lowestHealth&&enemies[i]->current_health>0)
            {
                lowestHealth = enemies[i]->current_health;
            }

        }
        return lowestHealth;
    }

    Enemy* GetEnemyWithLowestHealth()
    {
        if (enemies.size() == 0)
        {
            return NULL;
        }
        int lowestHealthIndex = -1;
        int lowestHealth = INT_MAX;
        for (int i = 0; i < enemies.size(); ++i)
        {
            if (enemies[i]->current_health < lowestHealth&&enemies[i]->current_health>0)
            {
                lowestHealth = enemies[i]->current_health;
                lowestHealthIndex = i;
            }

        }
        return enemies[lowestHealthIndex];
    }

    Enemy* GetEnemyWithHighestHealth()
    {
        if (enemies.size() == 0)
        {
            return NULL;
        }
        int highestHealthIndex = -1;
        int highestHealth = -INT_MAX;
        for (int i = 0; i < enemies.size(); ++i)
        {
            if (enemies[i]->current_health > highestHealth)
            {
                highestHealth = enemies[i]->current_health;
                highestHealthIndex = i;
            }

        }
        return enemies[highestHealthIndex];
    }

    float GetEnemyAverageHealth()const
    { 
        return GetEnemyTotalHealth() / enemies.size();
    }

    float GetEnemyTotalHealth()const
    {
        float total = 0;
        for (int i = 0; i < enemies.size(); ++i)
        {
            total += enemies[i]->current_health;
        }
        return total;
    }

    float GetEnemyAverageMaxHealth()const
    {
        float total = 0;
        for (int i = 0; i < enemies.size(); ++i)
        {
            total += enemies[i]->max_health;

        }
        return total / enemies.size();
    }

    int GetAllyWithMostThreat(int heroIndex)const
    {
        int mostThreat = -1000;
        int hIndex = -1;

        for (int i = 0; i < 3; ++i)
        {
            if (i == heroIndex)
            {
                continue;
            }
            int threat = GetNumEnemiesTargetingHero(i);
            if (threat > mostThreat)
            {
                hIndex = i;
                mostThreat = threat;
            }
        }
        return hIndex;
    }

    int GetAllyLowestHealth(int heroIndex)const
    {
        int lowestHealth = 1000;
        for (int i = 0; i < 3; ++i)
        {
            if (i == heroIndex)
            {
                continue;
            }
            if (heros[i].max_health-heros[i].current_health < lowestHealth&&heros[i].current_health>=0)
            {
                lowestHealth = heros[i].current_health;
            }

        }
        return lowestHealth;
    }

    int GetAllyWithLowestHealth()const
    {
        int lowestHealth = 1000;
        int lowestIndex = NONE;
        for (int i = 0; i < 3; ++i)
        {

            if (heros[i].current_health < lowestHealth&&heros[i].max_health!=heros[i].current_health&&heros[i].current_health>0)
            {
                lowestHealth = heros[i].max_health - heros[i].current_health;
                lowestIndex = i;
            }
        }
        return lowestIndex;
    }

    float GetAllyAverageHealth()const
    {
        float avHealth = 0;
            for (int i = 0; i < 3; ++i)
            {
                avHealth += heros[i].current_health;
            }
        
        return avHealth / 3;
    }

    int GetNumAliveAllies()const
    {
        int numAllies = 3;
        for (int i = 0; i < 3; ++i)
        {
            if (heros[i].current_health <= 0)
            {
                numAllies--;
            }

        }
        return numAllies;
    }

    void ToQState(QState* q, int heroIndex)const
    {
        q->agent_health_percent = (heros[heroIndex].current_health / heros[heroIndex].max_health);
        q->enemies_targeting_agent = GetNumEnemiesTargetingHero(heroIndex);
        q->enemies_total = enemies.size();
        q->enemies_lowest_health = GetEnemyLowestHealth();
        q->enemies_average_health = GetEnemyAverageHealth();
        q->number_of_allies = GetNumAliveAllies();
        q->proximity_to_exit = proximity_to_exit;
        q->allies_lowest_health = GetAllyLowestHealth(heroIndex);
        q->allies_average_health = GetAllyAverageHealth();
    }
};



class QAction
{
public:
    
    virtual ~QAction() = 0{}
    virtual void PerformOnState(State* state, int heroIndex) = 0{ state->Simulate(); }
    virtual float GetSimulatedReward(const State* state, int heroIndex){
        State nextState = *state;
        PerformOnState(&nextState, heroIndex);
        return state->GetRewardDiff(nextState, heroIndex);

    }
    virtual void PrintAction() = 0{}
    virtual void DumpActionToFile(std::fstream& file) = 0{}
    void SetIndex(int i){
        index = i;
    }
    int GetIndex(){ return index; }
    
   
private:
    int index;
};
//actions
//attack
class AttackLowestEnemy :public QAction
{
public:
    ~AttackLowestEnemy(){}

    void PerformOnState(State* state, int heroIndex)override{
        Enemy* e = state->GetEnemyWithLowestHealth();
        if (!e)
        {
            
            return;
        }
        e->current_health -= state->heros[heroIndex].damage;
        if (e->current_health <=0)
        {
            //delete e;
            state->enemies.erase(std::find(state->enemies.begin(), state->enemies.end(), e));
        }

        
    }

    void DumpActionToFile(std::fstream& file) override
    {
        file.write("ALE", 3);


    }
    void PrintAction()override{
        printf("ALE");
    }
};
class AttackHighestEnemy :public QAction
{
public:
    ~AttackHighestEnemy(){}

    void PerformOnState(State* state, int heroIndex)override{
        Enemy* e = state->GetEnemyWithHighestHealth();
        if (!e)
        {
            
            return;
        }
        e->current_health -= state->heros[heroIndex].damage;
        if (e->current_health <= 0)
        {
            //delete e;
            state->enemies.erase(std::find(state->enemies.begin(), state->enemies.end(), e));
        }
        
        
    }
    void DumpActionToFile(std::fstream& file) override
    {
        file.write("AHE", 3);


    }
    void PrintAction()override{
        printf("AHE");
    }
};
class AttackRandomEnemy :public QAction
{
public:
    ~AttackRandomEnemy(){}

    void PerformOnState(State* state, int heroIndex)override{
        if (state->enemies.size() == 0)
        {
            
            return;
        }
        int choice = rand() % state->enemies.size();
        Enemy* e = state->enemies[choice];
        e->current_health -= state->heros[heroIndex].damage;
        if (e->current_health <= 0)
        {
            //delete e;
            state->enemies.erase(std::find(state->enemies.begin(), state->enemies.end(), e));
        }

        
    }
    void DumpActionToFile(std::fstream& file) override
    {
        file.write("ARE", 3);


    }
    void PrintAction()override{
        printf("ARE");
    }

};
//taunts
class TauntLowestEnemy :public QAction
{
public:
    ~TauntLowestEnemy(){}

    void PerformOnState(State* state, int heroIndex)override{
        Enemy* e = state->GetEnemyWithLowestHealth();
        if (!e)
        {
            
            return;
        }
        e->hero_target_index = heroIndex;
        
    }
    void DumpActionToFile(std::fstream& file) override
    {
        file.write("TLE", 3);


    }
    void PrintAction()override{
        printf("TLE");
    }

    
};
class TauntHighestEnemy :public QAction
{
public:
    ~TauntHighestEnemy(){}

    void PerformOnState(State* state, int heroIndex)override{
        Enemy* e = state->GetEnemyWithHighestHealth();
        if (!e)
        {
            
            return;
        }
        e->hero_target_index = heroIndex;
        
    }
    void DumpActionToFile(std::fstream& file) override
    {
        file.write("THE", 3);


    }
    void PrintAction()override{
        printf("THE");
    }

    
};
class TauntUnTauntedEnemy :public QAction
{
public:
    ~TauntUnTauntedEnemy(){}

    void PerformOnState(State* state, int heroIndex)override{

        for (int i = 0; i < state->enemies.size(); ++i)
        {
            if (state->enemies[i]->hero_target_index != heroIndex)
            {
                state->enemies[i]->hero_target_index = heroIndex;
                
                return;
            }
        }
        
    }
    void DumpActionToFile(std::fstream& file) override
    {
        file.write("TUT", 3);


    }
    void PrintAction()override{
        printf("TUT");
    }

    
};
//heals
class HealSelf :public QAction
{
public:
    ~HealSelf(){}

    void PerformOnState(State* state, int heroIndex)override{
        Hero* h = &state->heros[heroIndex];
        if (h->current_health > 0)
        {
            h->current_health = min(h->max_health,
                h->current_health + h->healing);
        }
        
        
    }

    void DumpActionToFile(std::fstream& file) override
    {
        file.write("HSF", 3);


    }
    void PrintAction()override{
        printf("HSF");
    }

    

};
class HealLowestAlly :public QAction
{
public:
    ~HealLowestAlly(){}

    void PerformOnState(State* state, int heroIndex)override{
        int lowestHealthIndex = state->GetAllyWithLowestHealth();
        if (lowestHealthIndex == -1)
        {
            return;
        }
        Hero* h = &state->heros[lowestHealthIndex];
        Hero* healer = &state->heros[heroIndex];
        if (h->current_health > 0)
        {
            h->current_health = min(h->max_health,
                h->current_health + healer->healing);
        }

        
    }
    void DumpActionToFile(std::fstream& file) override
    {
        file.write("HLA", 3);


    }
    void PrintAction()override{
        printf("HLA");
    }
};
class HealThreatenedAlly :public QAction
{
public:
    ~HealThreatenedAlly(){}

    void PerformOnState(State* state, int heroIndex)override{
        int highestThreatIndex = state->GetAllyWithMostThreat(heroIndex);
        Hero* h = &state->heros[highestThreatIndex];
        Hero* healer = &state->heros[heroIndex];
        if (h->current_health > 0)
        {
            h->current_health = min(h->max_health,
                h->current_health + healer->healing);
        }

        
    }
    void DumpActionToFile(std::fstream& file) override
    {
        file.write("HTA", 3);


    }
    void PrintAction()override{
        printf("HTA");
    }
};
//utility
class MoveRoom :public QAction
{
public:
    ~MoveRoom(){}
    void PerformOnState(State* state, int heroIndex)override{
        state->proximity_to_exit -= 1;
        for (int i = 0; i < state->num_enemies_spawn_per_room; ++i)
        {

            state->enemies.push_back(new Enemy(state->prefab));
        }
        
        
    }
    void DumpActionToFile(std::fstream& file) override
    {
        file.write("MRM", 3);


    }
    void PrintAction()override{
        printf("MRM");

    }
};

class QLearner
{
public:
    QAction* GetChosenAction(const State* state){
        //loop through each possible action, find the action with the highest suitability for the current situatio

        //run to calculate the neural net
        QState q;
        fann_type* out;
        float outArr[10];
        state->ToQState(&q, heroIndex);
        out = fann_run(ann_, (fann_type*)&q);

        //copy q values into array
        memcpy(outArr, out, sizeof(float) * 10);

        //find the action with the highest QValue (exploitation)
        float best = -100000;
        int bestIndex = -1;
        for (int i = 0; i < 10; ++i)
        {
            if (*out > best)
            {
                best = *out;
                bestIndex = i;
            }
            ++out;
        }
        assert(bestIndex != -1);
        //calculate Q Learning value using predicted Q's
        State nextState = *state;
        actions_[bestIndex]->PerformOnState(&nextState,heroIndex);
        nextState.Simulate();
        float nextBestQ = GetMaxQForState(&nextState);
        float reward = state->GetRewardDiff(nextState,heroIndex);

        //add the best Q value of the next state
        reward += learningDiscount*nextBestQ;
        
        //iteratively change the neural network
        outArr[bestIndex] = reward;
        fann_train(ann_, (fann_type*)&q, outArr);

        return actions_[bestIndex];
    }

    float GetMaxQForState(const State* state)
    {
        float maxQ = -1000;
        QState q;
        fann_type* out;

        state->ToQState(&q, heroIndex);
        out = fann_run(ann_, (fann_type*)&q);

        for (int j = 0; j < 10; ++j)
        {
            if (*out > maxQ)
            {
                maxQ = *out;
            }
            ++out;
        }

        return maxQ;
    }

    void SaveNeuralNet(std::string filename)
    {
        fann_save(ann_, filename.c_str());
    
    }

    void Init(int hI)
    {
        //number of state variables (only 4 byte) +1 variable for chosen action index
        const unsigned int num_input = (sizeof(QState) / 4) + 1;
        // Q value of Q(s,a)
        const unsigned int num_outputs = 10;
        //a choice of one hidden layer, chosen as it is the most used number of layers
        const unsigned int num_layers = 4;
        //chosen as it is reccomended to have a numbet between the input and output nueron number
        const unsigned int num_neurons_hidden = 10;

        ann_ = fann_create_standard(num_layers, num_input,num_neurons_hidden, num_neurons_hidden,num_outputs);
        

        fann_set_bit_fail_limit(ann_, 0.01f);
        learningDiscount = 0.8;

        int index = 0;

        actions_.push_back(new AttackLowestEnemy());
        actions_.back()->SetIndex(index++);
        actions_.push_back(new AttackHighestEnemy());
        actions_.back()->SetIndex(index++);
        actions_.push_back(new AttackRandomEnemy());
        actions_.back()->SetIndex(index++);

        actions_.push_back(new TauntLowestEnemy());
        actions_.back()->SetIndex(index++);
        actions_.push_back(new TauntHighestEnemy());
        actions_.back()->SetIndex(index++);
        actions_.push_back(new TauntUnTauntedEnemy());
        actions_.back()->SetIndex(index++);

        actions_.push_back(new HealSelf());
        actions_.back()->SetIndex(index++);
        actions_.push_back(new HealLowestAlly());
        actions_.back()->SetIndex(index++);
        actions_.push_back(new HealThreatenedAlly());
        actions_.back()->SetIndex(index++);

        actions_.push_back(new MoveRoom());
        actions_.back()->SetIndex(index++);

        printf("Network created");
        heroIndex = hI;
    }


private:
    std::vector<QAction*> actions_;
    //index describing which hero we are to reccive data
    // try to abstract our games implementation from this class!
    int heroIndex;
    //nural network thing
    struct fann *ann_;

    float learningDiscount;
};

#endif