#include <mpi.h>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <cmath>

using namespace std;

//run mpi while checking errors, take an error message
void check_error(int status, const string message="MPI error") {
  if ( status != 0 ) {    
    cerr << "Error: " << message << endl;
    exit(1);
  }
}

void acquire_partition_rows(vector<int> &partition_rows,
                                   const int points, const int processes, 
                                   const int rank) {
  int rows_per_process = points/processes;
  for (int i = 0; i < rows_per_process; i++) {
    if (i % 2 == 0) {
      partition_rows.push_back(rank + i);
    } else {
      partition_rows.push_back(points - rank - i);
    }
  }
}

// inline void acquire_partition_size(int &partition_size, const int points, 
//                                    const int processes, const int rank) {
//     if (processes - 1 != rank) {
//       partition_size = points / processes;
//     } else {
//       partition_size = (points / processes) + (points % processes);
//     }
// }

//read string from file into a vector -> translate chars to ints
void read_csv_coords(vector<float> &x, vector<float> &y, const string &path){
    ifstream input_stream (path);

    if (!input_stream.is_open()) {
      cerr << "coudn't find/open file..." << endl;
      exit(1);
    }

    bool alternate = 0;
    for(string line; getline(input_stream, line);) {
      stringstream ss(line);

      string float_string;
      while(getline(ss, float_string, ',')) {
        if (alternate == 0) {
          x.push_back( (float)atof(float_string.c_str()) );
        } else {
          y.push_back( (float)atof(float_string.c_str()) );
        }
        alternate = !alternate; 
      }
    }
}

template <class T>
inline void print_vector(const vector<T> &vec, const int signal) {
  for(auto i : vec) {
    cout << i << " ";
  }
  if (signal == 1) cout << endl;
}

//print a character vector
template <class T>
inline void print_vector(const vector<T> &vec) {
  for(auto i : vec) {
    cout << i << " ";
  }
}

template <class T>
inline void process_has(int process, T data, string info) {
  cout << "process " << process << " has: " << data;
  cout << " " << info << endl;
}

inline void get_2d_coords(const int index, const int rows, 
                   int &x_coord, int &y_coord) {
  x_coord = (index % rows);
  y_coord = (index / rows); 
}

inline int get_1d_coord(const int &x_coord, const int &y_coord, const int columns) {
  return (x_coord*columns)+y_coord;
}

inline void average_points(vector<float> &x, vector<float> &y, const int a, const int b) {
  x[a] = (x[a] + x[b]) / 2.0;
  y[a] = (y[a] + y[b]) / 2.0;
}

template <class T>
vector<T> flatten_2d(const vector<vector<T>> &vec_2d) {
  int rows = vec_2d[0].size();
  int columns = vec_2d.size();

  vector<T> flattened(rows*columns);

  int k = 0;
  for(int i = 0; i < rows; i++) {
    for(int j = 0; j < rows; j++) {
      flattened[k] = vec_2d[i][j];
      k++;
    }
  }

  return flattened;
}

void visualize_distance_matrix (const vector<float> &distance_matrix, 
                                const int points) {
  for(int i = 0; i < points; i ++) {
    for(int j = 0; j < i; j ++) {
      cout << distance_matrix[(i*points)+j] << " ";
    }
    cout << endl;
  }
}

void visualize_distance_matrix (const vector<float> &distance_matrix, 
                                const int points, const int signal) {
  for(int i = 0; i < points; i ++) {
    for(int j = 0; j < points; j ++) {
      cout << distance_matrix[(i*points)+j] << " ";
    }
    cout << endl;
  }
}

void visualize_clusters (const vector<vector<int>> &clusters) {
  for(auto i : clusters) {
    cout << "< ";
    for(auto j : i) {
      cout << j + 1 << " ";
    }
    cout << "> ";
  }
  cout << endl;
}

void compute_distance_matrix (const int points, 
                              const vector<int> &partition_rows, 
                              const vector<float> &x, const vector<float> &y, 
                              vector<float> &distance_matrix) {
  //iterate through the top row of matrix
  for (int i : partition_rows) {
    for (int q = 0; q < i; q++) {
        float x_diff = x[q] - x[i];
        float y_diff = y[q] - y[i];
        distance_matrix[(i*(points))+q] = sqrt((x_diff * x_diff) + 
                                              (y_diff * y_diff));
    }
  }

  // if(partition_row_1 == partition_row_2) return;

  //iterate through the bottom row of matrix
}

void compute_min_distance_between_clusters(vector<float> &min_cluster,
                                           const vector<int> &partition_rows,
                                           const vector<float> &distance_matrix, 
                                           const int points) {
  float min_distance = -1; 
  int cluster1, cluster2;

  for (int i : partition_rows) {
    for (int j = 0; j < i; j ++) {
      float curr_distance = distance_matrix[(i*points) + j];
      if (curr_distance != 0) {
        if (min_distance == -1 || 
          (min_distance != -1 && curr_distance < min_distance)) {
          cluster1 = i; 
          cluster2 = j;
          min_distance = curr_distance;
        }
      }
    }
  }

  min_cluster[0] = cluster1;
  min_cluster[1] = cluster2;
  min_cluster[2] = min_distance;

}

inline void update_clusters(const int &cluster1, const int &cluster2, 
                            vector<vector<int>> &clusters) {
  //what does update clusters do? add cluster 2 to cluster 1, get rid of cluster 2
  for(int i : clusters[cluster2]) {
    cout << i << endl;
    clusters[cluster1].push_back(i);
  }
  clusters.erase(clusters.begin() + cluster2);
}


// void update_dist_matrix () {}

int main (int argc, char *argv[]) {
  //initialize ranks and amount of processes 
  int rank;
  int p;

  // Initialized MPI
  check_error(MPI_Init(&argc, &argv), "unable to initialize MPI");
  check_error(MPI_Comm_size(MPI_COMM_WORLD, &p), "unable to obtain p");
  check_error(MPI_Comm_rank(MPI_COMM_WORLD, &rank), "unable to obtain rank");
  cout << "Starting process " << rank << "/" << p << "\n";

  //info necessary to perform task on separate processes
  int starting_points, points;
  vector<int>partition_rows;
  vector<float> x;
  vector<float> y;
  vector<float> distance_matrix;
  // vector<float> partition;
  vector<vector<int>> clusters;
  vector<float> cluster_candidates;
  vector<float> min_cluster(3);
  int expected_cluster_count = 1;

  //have main keep track of clusters?

  if(rank == 0) {
    //read csv
    read_csv_coords(x, y, "input.csv");
    points = x.size();
    cluster_candidates.resize(3*p);
  }

  sleep(1);

  check_error(MPI_Bcast(&points, 1, MPI_INT, 0, MPI_COMM_WORLD));  

  if(rank != 0) {
    x.resize(points);
    y.resize(points);
  }

  check_error(MPI_Bcast(&x[0], points, MPI_INT, 0, MPI_COMM_WORLD));  
  check_error(MPI_Bcast(&y[0], points, MPI_INT, 0, MPI_COMM_WORLD));  

  if(rank == 1) {
    cout << "non zero x vector: " << endl;
    print_vector(x, 1); 
    cout << "non zero y vector: " << endl;
    print_vector(y, 1); 
  }

  if(rank == 0) {
    starting_points = points;
    clusters.resize(points);

    for(int i = 0; i < points; i ++) {
      clusters[i].push_back(i);
    }
  }

  //each process startings in the distance matrix at r*partition size
  //they all should check until their global starting position is
  //equal to the global end

  distance_matrix.resize(pow(points, 2));

  int distance_matrix_points = (pow(points, 2) -  points)/2;
  acquire_partition_rows(partition_rows, points, p, rank);
  // process_has(rank, partition_size);

  // sleep(rank);
  // cout << rank;
  // print_vector(partition_rows); cout << endl;

  // x
  // x x
  // x x x

  compute_distance_matrix(points, partition_rows, x, y, distance_matrix);

  sleep(rank);
  cout << "--------------" << rank << endl;
  visualize_distance_matrix(distance_matrix, points);
  cout << "--------------" << endl;

  compute_min_distance_between_clusters(min_cluster, partition_rows, 
                                        distance_matrix, points);

  check_error(MPI_Barrier(MPI_COMM_WORLD));

  sleep(rank);
  cout << "rank" << rank << " clusters" << endl;
  cout << min_cluster[0] << ", " << min_cluster[1];
  cout << "--------------" << endl;

  check_error(MPI_Gather(&min_cluster[0], 3, MPI_FLOAT, &cluster_candidates[0],
              3, MPI_FLOAT, 0, MPI_COMM_WORLD));

  check_error(MPI_Barrier(MPI_COMM_WORLD));

  if (rank == 0) {
    print_vector(cluster_candidates); cout << endl;
  }

  // find_champion_min(cluster_candidates); 

  //compute mins in individual processes, gather into array of p size

  // -----------------------------------------------------------
    //i compute the distance matrix...
  // if(rank == 0) {
  //   for (int i = 0; i < starting_points - expected_cluster_count; i ++) {
  //     //distance matrix size would change...
  //     // vector<float> distance_matrix(pow(points, 2));
  //     compute_distance_matrix(points, x, y, distance_matrix, 1);

  //     cout << " ----distance---- " << endl;
  //     visualize_distance_matrix(distance_matrix, points);
  //     cout << " ----points 2---- " << endl;

  //     int cluster1, cluster2;
  //     //i find the minimum distance... 
  //     compute_min_distance_between_clusters(cluster1, cluster2, distance_matrix, points);
  //     //i add whatever is in the index of the second cluster to whatever is in the first cluster
  //     //i remove the 4 coordinates from the x,y vector and add back the average of the points

  //     print_vector(x); cout << endl;
  //     print_vector(y); cout << endl;
  //     cout << " ----points 2---- " << endl;

  //     average_points(x, y, cluster1, cluster2);
  //     x.erase(x.begin() + cluster2);
  //     y.erase(y.begin() + cluster2);

  //     print_vector(x); cout << endl;
  //     print_vector(y); cout << endl;

  //     cout << " ----clusters---- " << endl;

  //     visualize_clusters(clusters);
  //     update_clusters(cluster1, cluster2, clusters);
  //     cout << "adding from " << cluster2 << " to " << cluster1 << endl;
  //     visualize_clusters(clusters);

  //     cout << " ----updates---- " << endl;
      
  //     points = x.size();
  //     cout << "new point count: " << points << endl;
  //     // distance_matrix.clear();
  //   }
      
    // repeat...

    // after computing the min distance cluster, i...
    // add cluster 2 to wherever cluster 1 is...
    // i remove cluster 2, i average points as i go on...
    
    // print_vector(distance_matrix); cout << endl;
  // }

  //not completely parallel
  //read points -- rank 0
  //calculate distance matrix -- split over processes, gather
  //get minimum distance cluster k -- split over grid, gather, calculate min on that
    //add this cluster to rank 0
  //update points -- split over processes, gather
  //

  //compute distance matrix

  //broadcast cut_size so that processes can resize to hold enough data
  //check_error(MPI_Bcast(&cut_size, 1, MPI_INT, 0, MPI_COMM_WORLD));  
  //cut.resize(cut_size);

  //scatter input string
  //check_error(MPI_Scatter(&sequence[0], cut_size, MPI_CHAR, &cut[0], cut_size, 
                           //MPI_CHAR, 0, MPI_COMM_WORLD));  

  //count cut sequences and add to each result array
  //count_sequence(results, cut);

  //sum results using mpi reduce to an array final results
  //check_error(MPI_Reduce(&results[0], &final_results[0], 4, MPI_INT, MPI_SUM, 
              //0, MPI_COMM_WORLD));

  //print results from rank 0
  if (rank == 0) {
    // print_vector(x); cout << endl;
    // print_vector(y); cout << endl;
  }

  //finalize and quit mpi, ending processes
  check_error(MPI_Finalize());
  cout << "Ending process " << rank << "/" << "p\n";

  return 0;
}  
