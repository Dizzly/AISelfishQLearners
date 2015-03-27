////////////////////////////////////////////////////////////////////////////////
//
// (C) Andy Thomason 2012-2014
//
// Modular Framework for OpenGLES2 rendering on multiple platforms.
//
// Text overlay
//



#include "QLearner.h"
#include <assert.h>
#include <string>
#include <fstream>
#include "File.h"

QLearner* heros[3];
/// Create a box with octet
int main(int argc, char **argv) {

    heros[DEEPS] = new QLearner();
    heros[TANK] = new QLearner();
    heros[HOTS] = new QLearner();

    heros[DEEPS]->Init(DEEPS);
    heros[TANK]->Init(TANK);
    heros[HOTS]->Init(HOTS);

    printf("\n");
    int runs = 0;
    int resets = 0;
    std::fstream f;
    File config;
    bool configFileOk = true;
    if (!config.OpenRead("config.txt"))
    {
        configFileOk = false;
        printf("No config file detected, using default values");
    }


    //options
    //print every round to file, regardless of inactivty
    bool file_verbose = false;
    //print actions and state to console
    bool console_print = false;
    bool console_verbose = false;
    //when the party succeeds it will save the neural network config of each to file
    bool save_neural_net_on_success = false;


    //enemy stat control
    int enemyDamage = 3;
    int enemyHealth = 9;
    int enemiesPerRoom = 2;
    bool enemiesSpawnSick = true;

    //number of rooms ai must complete to succeed
    int targetNumRoomsCompleted = 60;

    if (configFileOk)
    {
        config.GetBool(&file_verbose);
        config.GetBool(&console_print);
        config.GetBool(&console_verbose);
        config.GetBool(&save_neural_net_on_success);

        config.GetInteger(&enemyDamage);
        config.GetInteger(&enemyHealth);
        config.GetInteger(&enemiesPerRoom);
        config.GetBool(&enemiesSpawnSick);

        config.GetInteger(&targetNumRoomsCompleted);
    }
    f.open("CombatLog.txt", std::fstream::out);

    assert(f.is_open());


    State firstState;
    while (true)
    {
        firstState.Init(enemyDamage, enemyHealth, enemiesPerRoom, enemiesSpawnSick,
            targetNumRoomsCompleted);

        while (firstState.proximity_to_exit != 0)
        {

            QAction* d = 0, *t = 0, *h = 0;
            State prev = firstState;
            //simulate all hero actions
            if (firstState.heros[DEEPS].current_health > 0)
            {
                d = heros[DEEPS]->GetChosenAction(&firstState);
                d->PerformOnState(&firstState, DEEPS);
            }
            if (firstState.heros[HOTS].current_health > 0)
            {
                h = heros[HOTS]->GetChosenAction(&firstState);
                h->PerformOnState(&firstState, HOTS);
            }
            if (firstState.heros[TANK].current_health > 0)
            {
                t = heros[TANK]->GetChosenAction(&firstState);
                t->PerformOnState(&firstState, TANK);
            }
            //simulate enemies
            firstState.Simulate();
            //if they are all dead, reset the simulation
            if (firstState.number_of_allies == 0)
            {
                resets += 1;
                if (console_print)
                {
                    printf("RESET:%i\n", resets);
                }
                runs = 0;
                firstState.Init(enemyDamage, enemyHealth, enemiesPerRoom, enemiesSpawnSick,
                    targetNumRoomsCompleted);

                continue;
            }
            //if not increase the number of runs
            runs++;

            //check for activity, if there are any meaningful changes to the state
            bool noActivity = prev == firstState;
            //if we are verbose and save it all to file, or there was a change
            if (file_verbose || !noActivity)
            {
                char buff[100];
                sprintf(buff, "Runs:%i ", runs);
                int strl = strlen(buff);
                f.write(buff, strl);
                if (d)
                {
                    f.write("Deeps:", 6);
                    d->DumpActionToFile(f);
                }
                if (h)
                {
                    f.write(" Hots:", 5);
                    h->DumpActionToFile(f);
                }
                if (t)
                {
                    f.write(" Tank:", 5);
                    t->DumpActionToFile(f);
                }
                f.write("\n", 1);
                firstState.DumpToFile(f);
                f.write("\n", 1);
            }
            if (console_print && (console_verbose || !noActivity))
            {
                if (d)
                {
                    printf("D(%i):", firstState.heros[DEEPS].current_health);
                    d->PrintAction();
                }
                if (h)
                {
                    printf("|");
                    printf("H(%i):", firstState.heros[HOTS].current_health);
                    h->PrintAction();
                }
                if (t)
                {
                    printf("|");
                    printf("T(%i):", firstState.heros[TANK].current_health);
                    t->PrintAction();
                }
                printf("\n");
                for (int i = 0; i < firstState.enemies.size(); ++i)
                {
                    printf("E(%i)T:%i|", firstState.enemies[i]->current_health,
                        firstState.enemies[i]->hero_target_index);
                }
                printf("\n\n");
            }
        }//END OF WHILE LOOP
        //after this while then they have made it out of the dungeon
        printf("SUCCSESS after :%i wipes", resets);
        if (save_neural_net_on_success)
        {
            printf("/n Saving Neural Net\n");
            heros[0]->SaveNeuralNet("DeepsNN.net");
            heros[1]->SaveNeuralNet("HotsNN.net");
            heros[2]->SaveNeuralNet("TankNN.net");
        }

        f.close();


        system("PAUSE");
    }



    return 0;
}


