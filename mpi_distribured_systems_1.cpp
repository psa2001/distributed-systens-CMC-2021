#include "../ompi/include/mpi.h"
//#include "mpi.h"
#include <iostream>
#include <cstdio>
#include <random>
#include <map>
#include <vector>
#include <set>
#include <utility>
#include <cmath>
#include <string>


using namespace std;
//launch mpic++ mpi_distribured_systems_1.cpp -o mpi_distribured_systems_1 && mpirun -np 16 mpi_distribured_systems_1

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    string symbols = "aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ";
    int very_long_message_len = 12345; //very long message length
    int k = 1234; //the number of parts into which the message is divided  
    char very_long_message[very_long_message_len + 1]; //for \0 for last element
    very_long_message[very_long_message_len] = '\0'; //line end
    int penultimate_sending_size = 0, last_sending_size = 0;
    int send = ceil(k / 2.);
    int one_piece_len = very_long_message_len / k; //length of one piece sending
    if(k%2 == 0){
        penultimate_sending_size = one_piece_len;
        if(very_long_message_len % k == 0)
            last_sending_size = one_piece_len;
        else
            last_sending_size = very_long_message_len - one_piece_len * (k - 1);
    } else {
        last_sending_size = 0;
        if(very_long_message_len % k == 0)
            penultimate_sending_size = one_piece_len;
        else
            penultimate_sending_size = very_long_message_len - one_piece_len * (k - 1);
    }


    int number_of_processes_in_each_dimension = 4;
    int ndims = 2;
    const int dims[2] = {number_of_processes_in_each_dimension, number_of_processes_in_each_dimension};
    const int periods[2] = {false, false};
    int coords[2];
    int reorder = false;


    vector<pair<int, int>> path_of_sending_mes = {{0, 1}, {1, 2}, {2, 3}, {3, 7}, {7, 11}, {11, 15}, {0, 4}, {4, 8}, {8, 12}, {12, 13}, {13, 14}, {14, 15}}; //chained rules of processes sendings 
    set<int> used_prs = {1,2,3,7,11,4,8,12,13,14}; //not first and not last processes, which are used


    int rank;
    MPI_Comm tr_mat;
    MPI_Cart_create(MPI_COMM_WORLD, ndims, dims, periods, reorder, &tr_mat);
    MPI_Comm_rank(tr_mat, &rank);
    MPI_Cart_coords(tr_mat, rank, 2, coords);
    MPI_Request req1, req2;


    // process (0;0)
    if (coords[0] == 0 && coords[1] == 0) {
        srand (time(NULL));
        for (int i = 0; i < very_long_message_len; ++i) {
            very_long_message[i] = symbols[rand() % 52];
        }
        cout << "Input message: " << very_long_message << endl;
        for(int i = 0; i < send; ++i){
            if(i != send - 1){
                MPI_Irsend(&very_long_message[2 * i * one_piece_len],  one_piece_len, MPI_CHAR, 1, 0, tr_mat, &req1);
                MPI_Irsend(&very_long_message[(2 * i + 1) * one_piece_len], one_piece_len, MPI_CHAR, 4, 0, tr_mat, &req2);
                MPI_Wait(&req1,  MPI_STATUS_IGNORE);
                MPI_Wait(&req2,  MPI_STATUS_IGNORE);
            } else {
                MPI_Irsend(&very_long_message[2 * i * one_piece_len],  penultimate_sending_size, MPI_CHAR, 1, 0, tr_mat, &req1);
                if(last_sending_size)
                    MPI_Irsend(&very_long_message[(2 * i + 1) * one_piece_len], last_sending_size, MPI_CHAR, 4, 0, tr_mat, &req2);
                MPI_Wait(&req1,  MPI_STATUS_IGNORE);
                if(last_sending_size)
                    MPI_Wait(&req2,  MPI_STATUS_IGNORE);
            }
        }
    }
    //process (3;3)
    if (coords[0] == 3 && coords[1] == 3) {
        for(int i = 0; i < send; ++i){
            if(i != send - 1){
                MPI_Irecv(&very_long_message[2 * i * one_piece_len], one_piece_len, MPI_CHAR, 11, 0, tr_mat, &req1);
                MPI_Irecv(&very_long_message[(2 * i + 1) * one_piece_len], one_piece_len, MPI_CHAR, 14, 0, tr_mat, &req2);
                MPI_Wait(&req1,  MPI_STATUS_IGNORE);
                MPI_Wait(&req2,  MPI_STATUS_IGNORE);
            } else {
                MPI_Irecv(&very_long_message[2 * i * one_piece_len], penultimate_sending_size, MPI_CHAR, 11, 0, tr_mat, &req1);
                if(last_sending_size)
                    MPI_Irecv(&very_long_message[(2 * i + 1) * one_piece_len], last_sending_size, MPI_CHAR, 14, 0, tr_mat, &req2);
                MPI_Wait(&req1,  MPI_STATUS_IGNORE);
                if(last_sending_size)
                    MPI_Wait(&req2,  MPI_STATUS_IGNORE);
            }
        } 
        cout << "Output message: " << very_long_message << endl;
    }
    //for processes 1; 2; 3; 7; 11 // 4; 8; 12; 13; 14
    if(!((coords[0] == 0 && coords[1] == 0) || (coords[0] == 3 && coords[1] == 3)) and used_prs.count(rank)){
        int prev_rank;
        int next_rank;
        for (const auto& i : path_of_sending_mes) {
            if(rank == i.second){
                prev_rank = i.first;
            }
            if (rank == i.first) {
                next_rank = i.second;
                break;
            }
        }
        for(int i = 0; i < send; ++i){
            if(i != send - 1){
                MPI_Irecv(very_long_message, one_piece_len, MPI_CHAR, prev_rank, 0, tr_mat, &req1);
                MPI_Wait(&req1,  MPI_STATUS_IGNORE);
                MPI_Rsend(very_long_message, one_piece_len, MPI_CHAR, next_rank, 0, tr_mat);
            } else {
                if(rank == 1 || rank == 2 || rank == 3 || rank == 7 || rank == 11){
                    MPI_Irecv(very_long_message, penultimate_sending_size, MPI_CHAR, prev_rank, 0, tr_mat, &req1);
                    MPI_Wait(&req1,  MPI_STATUS_IGNORE);
                    MPI_Rsend(very_long_message, penultimate_sending_size, MPI_CHAR, next_rank, 0, tr_mat);
                } else {
                    if(last_sending_size){
                        MPI_Irecv(very_long_message, last_sending_size, MPI_CHAR, prev_rank, 0, tr_mat, &req1);
                        MPI_Wait(&req1,  MPI_STATUS_IGNORE);
                        MPI_Rsend(very_long_message, last_sending_size, MPI_CHAR, next_rank, 0, tr_mat);
                    }
                }
            }
        }
    }
    MPI_Barrier(tr_mat);
    MPI_Finalize();
    return 0;
}