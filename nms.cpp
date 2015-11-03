/*
 * assume image names are grouped together
 *
 *
 *
*/

#include <iostream>
#include <cassert>
#include <string>
#include <map>
#include <fstream>
#include <utility>
#include <algorithm>
#include <numeric>
using namespace std;

const unsigned int maxnum = 100000000;
const int max_per_class = 40;
const int max_per_image = 100;
map<string, pair<int,int>> locs;
float scores[maxnum];
vector<float> buf;
float thresh = -10000000;

int X1[maxnum], Y1[maxnum], x2[maxnum], y2[maxnum];
bool del[maxnum] = {false};

double IoU(const vector<int> &boX1, const vector<int> &box2){
     int X1 = std::max(boX1[0], box2[0]);
     int Y1 = std::max(boX1[1], box2[1]);
     int x2 = std::min(boX1[2], box2[2]);
     int y2 = std::min(boX1[3], box2[3]);
     double inter = std::max(0,(x2 - X1 + 1)) * std::max(0,(y2 - Y1 + 1));
     double area1 = (boX1[2] - boX1[0] + 1) * (boX1[3] - boX1[1] + 1);
     double area2 = (box2[2] - box2[0] + 1) * (box2[3] - box2[1] + 1);
     double o = inter / (area1 + area2 - inter);
     return (o >= 0) ? o : 0;
}

int main(int argc, char *argv[]){
	
    if(argc != 4){
        cerr << "Number of arguments doesn't match!";
        return 1;
    }
    ifstream fin(argv[1]);
    assert(fin);
    ofstream fout(argv[2]);
    double nms_thresh = stod(argv[3]);
	int image_num = 0;
    string imname;
    while(fin >> imname >> scores[image_num] >> X1[image_num] >> Y1[image_num]
          >> x2[image_num] >> y2[image_num]){
        auto it = locs.find(imname);
        if(it == locs.end())
            locs.emplace(make_pair(imname,make_pair(image_num,image_num)));
        else
            it->second.second = max(it->second.second, image_num);
        image_num++;
    }

    int bar = 0;
    for(const auto &e : locs){
        vector<int> index(e.second.second - e.second.first + 1, 0);
        bar++;
        cout<<"Preprocessing " << bar <<"th image, name: "<<e.first<<endl;
        iota(index.begin(), index.end(), e.second.first);
        if(index.size() <= max_per_image){
            for(auto i : index) buf.push_back(scores[i]);
            continue;
        }

        sort(index.begin(), index.end(), [&](int a,int b){
            return scores[a] > scores[b];
        });

        for(int i = 0; i < max_per_image; i++) buf.push_back(scores[index[i]]);
        for(int i = max_per_image; i < index.size(); i++)
            del[index[i]] = true;
    }

    cout<<"------Pruning out images------"<<endl;
    sort(buf.begin(), buf.end(), [](float a, float b){return a>b;});
    thresh = buf[std::min(max_per_class * locs.size() - 1, buf.size()-1)];

    cout<<"------Beginning NMS-------"<<endl;
    for(const auto &e: locs){
        cout<<"Doing NMS for image " << e.first << endl;

        vector<int> index(e.second.second - e.second.first + 1,0);
        iota(index.begin(), index.end(), e.second.first);
        sort(index.begin(), index.end(), [&](int a,int b){
            return scores[a] > scores[b];
        });

        for(int i = 0; i < index.size(); i++)
            if(scores[index[i]] >= thresh && !del[index[i]])
                for(int j = i + 1; j < index.size(); j++)
                    if(scores[index[j]] >= thresh && !del[index[j]]){
                        int X1i = X1[index[i]], Y1i = Y1[index[i]], x2i = x2[index[i]], y2i = y2[index[i]];
                        int X1j = X1[index[j]], Y1j = Y1[index[j]], x2j = x2[index[j]], y2j = y2[index[j]];
                        if(IoU(vector<int>{X1i,Y1i,x2i,y2i}, vector<int>{X1j,Y1j,x2j,y2j}) > nms_thresh)
                            del[index[j]] = true;
                    }
        for(int i = 0; i < index.size(); i++)
            if(scores[index[i]] >= thresh && !del[index[i]])
                fout<<e.first<<" "<<scores[index[i]]<<" "<<X1[index[i]]<<" "<<Y1[index[i]]<<" "<<x2[index[i]]<<" "<<y2[index[i]]<<endl;
    }
}
