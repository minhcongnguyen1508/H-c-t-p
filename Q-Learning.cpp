//#include "Functions.h"
#include <functional>
#include <fstream>
#include <iostream>
#include <string>
#include <Vector>
// Values size
int num_actions = 4;
// Optimistic initialization
bool opt_init = true;
// Frequency with which we should explore
float epsilon = 0.2f; 
/* Saves the eligibility traces, same size as the 
    feature vector. */
// Lưu dấu vết
float ***eligibility;
/* Vector of integers of length N that tells 
    us how many times we’ve played each action*/
int *counts;
/* Discounted rewards we expect to receive if we start 
    at state s, take action a*/
// Lưu trữ rewards- phần thưởng, state - trạng thái môi trường và hành động

float ***Q;
/* Parameter function vector*/
float **parameter_vector;
/* Discount factor for future rewards [0.0, 1.0]*/
float gama = 0.9f;  
float lambda = 0.1f;
/* Learning rate */
float alpha = 0.1f;

int largest_range; 

float episode_score = 0.0f;
std::vector<float> *epss = new std::vector<float> ();

int s0[2];
int a0;
float r0;
int s1[2];
int a1; // not used for Q learning
int starting = 1;

//Helper functions
int next_action();
int start_episode();
int step();
int end_episode();
int get_max_value_index();
int e_greedy();
void save_log();
bool can_move2(int a);
double distance_Calculate(int x1, int y1, int x2, int y2);
void check_Largest_Range();

int main()
{
    bool gameover = false;
    clock_t t1,t2;
    float diff,seconds; 
    int action;
    int count = 1;
    
    if( connect_RL() ) 
    {
        Q = (float***) std::malloc(sizeof(float**) * num_actions);
        eligibility =  (float***) std::malloc(sizeof(int**) * num_actions);
        for(int i = 0; i < num_actions; i++)
        {
            Q[i] = (float**) std::malloc(sizeof(float*) * NUM_ROWS);
            eligibility[i] =  (float**) std::malloc(sizeof(int*) * NUM_ROWS);
            for(int ir = 0; ir < NUM_ROWS; ir++)
            {
                Q [i][ir] =  (float*) std::malloc(sizeof(float) * NUM_COLS);
                eligibility[i][ir] =  (float*) std::malloc(sizeof(int) * NUM_ROWS);
            }
        }
        //Allocate space to world map
        wordl_map = (char**) std::malloc(sizeof(char*) * NUM_ROWS);
      	//  eligibility =  (float**) std::malloc(sizeof(float*) * NUM_ROWS);
        parameter_vector =  (float**) std::malloc(sizeof(float*) * NUM_ROWS);
        for(int i = 0; i < NUM_ROWS; i++)
        {
            wordl_map[i] = (char*) std::malloc(sizeof(char) * NUM_COLS);
       	//     eligibility[i] =  (float*) std::malloc(sizeof(float) * NUM_COLS);
            parameter_vector[i] =  (float*) std::malloc(sizeof(float) * NUM_COLS);
        }
        //Allocate space for players positions
        x = (int*) std::malloc(sizeof(int) * 1);
        y = (int*) std::malloc(sizeof(int) * 1);
        r = (int*) std::malloc(sizeof(int) * 1);
        speed = (int*) std::malloc(sizeof(int) * 1);
        alive = (int*) std::malloc(sizeof(int) * 1);
        teams =  (int*) std::malloc(sizeof(int) * 1);
        counts =  (int*) std::malloc(sizeof(int) * num_actions);

        //Game loop
        while( !gameover )
        {
            gameover = update_Map();
            if ( !gameover )
            {  //Updates players' information
               gameover = update_Players();
            }
            // AI agents get next action 
            action = next_action();
            count++;
            /* Print action to send to server */
            std::cout << action;
            
            if ( count % 100 )
            {
                count = 1;  
                save_log();
            }
        }
    }
    return 0;
}

/* RL agents next action */
int  next_action()
{
    int action = -1;
    if ( starting )
        action = start_episode();
    else 
    {
        r0 = -0.2f;
        for (int i = 1; i < 2 ; i++)
        {
        	// wordl_map == map;
            if ( wordl_map[x[PLAYER_ID] + i][y[PLAYER_ID]] == BOMB || wordl_map[x[PLAYER_ID] - i][y[PLAYER_ID]] == BOMB 
            ||   wordl_map[x[PLAYER_ID] + i][y[PLAYER_ID] +i] == BOMB || wordl_map[x[PLAYER_ID]][y[PLAYER_ID] - i] == BOMB  )
            {
                r0 = (float) -1.5f;
                break;
            }
        } 
        if (r0 == -0.2f)
        {
            if ( x[PLAYER_ID] == OBJECTIVE_X && y[PLAYER_ID] == OBJECTIVE_Y )
            {
                r0 = 20.0f;
                episode_score += r0;
                if ( episode_score <= 17.4f){
                    //std::cout << "Player " << PLAYER_ID << " scored:" << episode_score << std::endl;
                    epss->push_back(episode_score);
                }
                episode_score = 0;
                
            }
            else
            {
                episode_score += r0;
            }
        }
        action = step();
    }
    return action;
}

int start_episode()
{
    starting = 0;
    for (int i = 0; i < NUM_ROWS; ++i)	
    {
        for (int j = 0; j < NUM_COLS; ++j)	
        {
            if(!opt_init)
                parameter_vector[i][j] = 0.0f;
            else{
                parameter_vector[i][j] = 1.0f / (float)NUM_ROWS*NUM_COLS;
            }
        }
    }
    for (int a = 0; a < num_actions; a++)	
    {
        float Qoa = 0.0;
        for (int i = 0; i < NUM_ROWS; ++i)	
        {
            for (int j = 0; j < NUM_COLS; ++j)	
            {
                Q[a][i][j] = 0.0f; 
                eligibility[a][i][j] = 0;
            }
        }
    }
    //Update s0 & a0
    a0 = get_max_value_index();
    s1[0] = x[PLAYER_ID];
    s1[1] = y[PLAYER_ID];
    return a0;
}
/* calculate the target for Q(s,a)
Q learning target is Q(s0,a0) = r0 + gamma * max_a Q[s1,a] */
int step()
{
    // get next action a1
    int action = e_greedy();
    // update s0,s1,a0,a1
    s0[0] = s1[0];
    s0[1] = s1[1];
    s1[0] = x[PLAYER_ID];
    s1[1] = y[PLAYER_ID];
    //Q learning
    int qmax_i = get_max_value_index();
    float qmax = Q[qmax_i][x[PLAYER_ID]][y[PLAYER_ID]];
    float target = r0 + gama * qmax;
    
    eligibility[a0][s0[0]][s0[1]]++;
    // simpler and faster update without eligibility trace
    // update Q[sa] towards it with some step size
    float update = alpha * (target - Q[a0][s0[0]][s0[1]]);
    
    for (int a = 0; a < num_actions; a++)	
    {
        for (int i = 0; i < NUM_ROWS; ++i)	
        {
            for (int j = 0; j < NUM_COLS; ++j)	
            {
                Q[a][i][j] += update * eligibility[a][i][j];
                if ( action == qmax_i )
                    eligibility[a][i][j] = lambda * gama * eligibility[a][i][j];
                else    
                    eligibility[a][i][j] = 0;
            }
        }
    }
    a0 = action;
    return action;
}

int end_episode()
{
    
}

/* choose the action that maximizes the current value function with 
 * probability (1 - e) and a random action with the probability e */
int e_greedy( )
{
	float rnumb = (float) (rand()%100 + 1)/100;
	if( rnumb < epsilon)
    {
		return rand() % num_actions;
	}
	else
    {
		return get_max_value_index();
	}

}

int get_max_value_index()
{
    float qmax = -99999.0f;
    int qmax_index = 0;
    for (int a = 0; a < num_actions; a++)	
    {
        if ( can_move2(a) &&  Q[a][x[PLAYER_ID]][y[PLAYER_ID]] >= qmax )
        {
            qmax = Q[a][x[PLAYER_ID]][y[PLAYER_ID]];
            qmax_index = a;
        }
    }
	return qmax_index;
}
/*
void save_log()
{
    std::fstream log_data;
    char seedstring[4];
    std::sprintf(seedstring,"%d",PLAYER_ID); 
    std::string path = "Q_values";
    path += "_";
    path += seedstring;
    log_data.open(path,std::fstream::out );   
    std::string current_log = ""; 
    for ( int a = 0; a < num_actions; a++ )
    {
        for(int i = 0; i < NUM_ROWS; i++)
        {
            for(int j = 0; j < NUM_COLS; j++)
            {
                current_log += std::to_string(Q[a][i][j]); 
                current_log += " ";	
            }
            current_log += "\n";
        }
        current_log += "-------------------------\n";
    }
    log_data << current_log;
}
*/
void save_log()
{
    std::fstream log_data;
    char seedstring[4];
    std::sprintf(seedstring,"%d",PLAYER_ID); 
    std::string path = "Q_values";
    path += "_";
    path += seedstring;
    log_data.open(path,std::fstream::out );   
    std::string current_log = ""; 
    for ( int i = 0; i < epss->size(); i++ ) 
    {
        
        current_log += std::to_string(epss->at(i) ); 
        current_log += " ";	
    }
    current_log += "\n-------------------------\n";
    log_data << current_log;
}

void read_log()
{

}

bool can_move2(int a)
{
    int pos_x = x[PLAYER_ID];
    int pos_y = y[PLAYER_ID];
    std::vector<Actions> possible_moves;
    
    if( a == DOWN && wordl_map[pos_x + 1][pos_y] != STONE &&  wordl_map[pos_x + 1][pos_y] != WALL &&  wordl_map[pos_x + 1][pos_y] != BOMB &&  wordl_map[pos_x + 1][pos_y] != EXPLOSION)
        return true; //DOWN
        
    if(  a == UP && wordl_map[pos_x - 1][pos_y] != STONE &&  wordl_map[pos_x - 1][pos_y] != WALL &&  wordl_map[pos_x - 1][pos_y] != BOMB &&  wordl_map[pos_x - 1][pos_y] != EXPLOSION)
        return true; //UP

    if(  a == RIGHT && wordl_map[pos_x][pos_y + 1] != STONE &&  wordl_map[pos_x][pos_y + 1] != WALL &&  wordl_map[pos_x][pos_y + 1] != BOMB &&  wordl_map[pos_x][pos_y + 1] != EXPLOSION)
        return true; //RIGHT

    if(  a == LEFT && wordl_map[pos_x][pos_y -1] != STONE &&  wordl_map[pos_x][pos_y - 1] != WALL &&  wordl_map[pos_x][pos_y - 1] != BOMB &&  wordl_map[pos_x][pos_y - 1] != EXPLOSION)
        return true; //LEFT

    return false;
}

/* Calculates euclidean distance */
double distance_Calculate(int x1, int y1, int x2, int y2)
{
    int x = x1 - x2;
    int y = y1 - y2;
    double dist;
    dist = pow(x,2)+pow(y,2); 
    dist = sqrt(dist);
    return dist;
}
/* Update largest range */
void check_Largest_Range()
{
    double lr = -1;
    int lr_id = -1;
    for ( int i = 0; i < NUM_PLAYERS; i++)
    {
        if( alive[i])
        {
            if ( r[i] > lr )
            {
                lr = r[i];
                lr_id = i;
            }
        }
    }
    largest_range = r[lr_id];
}
