//g++ -O3 -o bootstrapCI.o bootstrapCI.cc -fopenmp
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <random>
#include <iterator>
#include <cmath>
#include "cxxopts.hpp"

using namespace std;

vector<double> readFile(string filePath){
    vector<double> scores;
    string line;
    vector <string> v;
    ifstream input (filePath);
    if(input.is_open()){
            //leemos el fichero
            while(getline(input,line)){
                scores.push_back(stod(line));
            }
            input.close();
            }else{
                cout << "Fallo al abrir"<<"\n";
            }
    return scores;
}

double average(const vector<double> &scores){
    return (accumulate(scores.begin(), scores.end(), 0.0)/scores.size());
}

double sumAllElements(const vector<double> &scores){
    return (accumulate(scores.begin(), scores.end(), 0.0));
}


vector<double> bootstrapVector(const vector<double> &scores){
    int sampleSize = scores.size();
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, sampleSize-1);

    vector<double> bootstrapedScores;
    bootstrapedScores.reserve(sampleSize);

    for(int i = 0; i < sampleSize; i++){
        bootstrapedScores.push_back(scores[distr(gen)]);
    }
    return bootstrapedScores;
}

void bootstrapCI(vector<double> &scores, int B, int CI){ 
    vector<double> res;
    #pragma omp parallel
    {
        vector<double> resPrivate;
        #pragma omp for
        for(int i = 0; i < B; i++){
            vector<double> bootstrapedScores = bootstrapVector(scores);
            resPrivate.push_back(average(bootstrapedScores));
        }
        #pragma omp critical
        {
            res.insert(res.end(), resPrivate.begin(), resPrivate.end());
        }
    }
    sort(res.begin(),res.end());
    double alpha = (100 - CI) / 2.0;
    int lowerIndex = (int) ((alpha / 100) * res.size());
    int upperIndex = (int) (((100 - alpha) / 100) * res.size());
    cout << "CI: [" <<  res[lowerIndex] << " , " << res[upperIndex] << "]\n";
    cout << "Original mean: " << average(scores) << "\n";
    cout << "Bootstrapped mean: " << average(res) << "\n";
}

cxxopts::ParseResult options(int argc, char* argv[]) {
    try{
        cxxopts::Options options("bootstrapCI", "Tool to calculate Confidence Intervals employing bootstrapping");
        options.add_options()("h,help", "Print this help")
        ("f,file", "Path to the scores file",cxxopts::value<string>())
        ("b", "Number of trials to calculate de CI",cxxopts::value<int>()->default_value("10000"))
        ("c", "Desired confidence interval", cxxopts::value<int>()->default_value("95"));

        cxxopts::ParseResult result = options.parse(argc, argv);

        if(result.count("help") or result.arguments().size() == 0) {
            cout << options.help({"", "Group"}) << "\n";
            exit(0);
        }

        if(result["b"].as<int>() < 1) {
            cout << "Number of repetitions must be larger than 0.\n";
            exit(0);
        }

        if(result["c"].as<int>() < 1 || result["c"].as<int>() > 99) {
            cout << "The Confidence Interval must be between 1 and 99.\n";
            exit(0);
        }

        return result;

    }catch (const cxxopts::OptionException& e) {
        std::cout << "Error parsing options " << e.what() << "\n";
        exit(1);
    }
}

int main(int argc, char *argv[]){
    cxxopts::ParseResult result = options(argc, argv);
    string scoresFile = result["f"].as<std::string>();
    int B = result["b"].as<int>();
    int CI = result["c"].as<int>();
    vector<double> scores = readFile(scoresFile);
    cout << "Starting test" << endl;
    bootstrapCI(scores, B, CI);
    return 0;
}
