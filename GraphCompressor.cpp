#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <utility>

using std::string;
using std::map;
using std::set;
using std::pair;
using std::vector;
using std::make_pair;

static void addEdge(uint32_t srcNum, uint32_t dstNum, map<uint32_t, set<uint32_t>>& edgesMap)
{
    const auto& srcIter = edgesMap.find(srcNum);
    if (srcIter == edgesMap.end()) {
        auto newList = new set<uint32_t>{ dstNum };
        edgesMap.insert(make_pair(srcNum, std::move(*newList)));
    } else {
        srcIter->second.insert(dstNum);
    }
}

static void deleteEdge(uint32_t srcNum, uint32_t dstNum, map<uint32_t, set<uint32_t>>& edgesMap)
{
    const auto& srcIter = edgesMap.find(srcNum);
    if (srcIter != edgesMap.end() && srcIter->second.find(dstNum) != srcIter->second.end()) {
        srcIter->second.erase(dstNum);
    }
}

static void readSortedGraph(std::ifstream& input, vector<pair<uint32_t, vector<uint32_t>>>& graphMap,
    map<pair<uint32_t, uint32_t>, uint8_t>& edgesWeights)
{
    map<uint32_t, set<uint32_t>> edgesMap;

    for (string edge; std::getline(input, edge); ) {
        string src, dst, weight;
        size_t pos = edge.find("\t");
        src = edge.substr(0, pos);
        size_t newPos = edge.find("\t", pos+1);
        dst = edge.substr(pos+1, newPos - pos - 1);
        weight = edge.substr(newPos+1, edge.size() - newPos);

        uint32_t srcNum = static_cast<uint32_t>(std::atol(src.c_str()));
        uint32_t dstNum = static_cast<uint32_t>(std::atol(dst.c_str()));
        uint8_t weightVal = static_cast<uint8_t>(std::atoi(weight.c_str()));

        addEdge(srcNum, dstNum, edgesMap);
        if (srcNum != dstNum)
            addEdge(dstNum, srcNum, edgesMap);

        if (srcNum > dstNum)
            std::swap(srcNum, dstNum);
        edgesWeights.insert(make_pair(make_pair(srcNum, dstNum), weightVal));
    }

    vector<pair<uint32_t, size_t>> vertexVector;
    vertexVector.reserve(edgesMap.size());
    for (const auto& edge : edgesMap)
        vertexVector.push_back(make_pair(edge.first, edge.second.size()));

    std::sort(vertexVector.begin(), vertexVector.end(), 
        [](const pair<uint32_t, size_t>& a, const pair<uint32_t, size_t>& b) {
            return a.second > b.second;
        }
    );

    for (const auto& vertexAndDegree : vertexVector) {
        const auto& vertexIter = edgesMap.find(vertexAndDegree.first);
        
        vector<uint32_t> dstVec;
        dstVec.reserve(vertexAndDegree.second);

        set<uint32_t> dstToDelete = vertexIter->second;
        for (const auto& dst : dstToDelete) {
            dstVec.push_back(dst);
            deleteEdge(dst, vertexAndDegree.first, edgesMap);
        }

        if (!dstVec.empty()) {
            graphMap.emplace_back(make_pair(vertexAndDegree.first, std::move(dstVec)));
        }
    }
}

static void serializeGraph(std::ifstream& input, std::ofstream& output)
{
    vector<pair<uint32_t, vector<uint32_t>>> graphMap;
    map<pair<uint32_t, uint32_t>, uint8_t> edgesWeights;
    readSortedGraph(input, graphMap, edgesWeights);

    size_t graphSize = graphMap.size();
    output.write(reinterpret_cast<const char*>(&graphSize), sizeof(graphSize));
    for (const auto& edge : graphMap) {
        uint32_t src = edge.first;
        int dstNum = edge.second.size();
        if (dstNum == 0) {
            continue;
        }

        output.write(reinterpret_cast<const char*>(&src), sizeof(src));
        output.write(reinterpret_cast<const char*>(&dstNum), sizeof(dstNum));

        for (uint32_t dst : edge.second) {
            uint32_t begin = src;
            uint32_t end = dst;
            if (begin > end) {
                std::swap(begin, end);
            }
            const auto& weightIter = edgesWeights.find(make_pair(begin, end));
            output.write(reinterpret_cast<const char*>(&dst), sizeof(dst));

            uint8_t weight = weightIter->second;
            output.write(reinterpret_cast<const char*>(&weight), sizeof(weight));
        }
    }
}

static void deserializeGraph(std::ifstream& input, std::ofstream& output)
{
    size_t graphSize = 0;
    input.read(reinterpret_cast<char*>(&graphSize), sizeof(graphSize));

    for ( ; graphSize > 0; --graphSize) {
        uint32_t src = 0;
        input.read(reinterpret_cast<char*>(&src), sizeof(src));

        int dstNum = 0;
        input.read(reinterpret_cast<char*>(&dstNum), sizeof(dstNum));
        for ( ; dstNum > 0; --dstNum) {
            uint32_t dst = 0;
            input.read(reinterpret_cast<char*>(&dst), sizeof(dst));

            uint8_t weight = 0;
            input.read(reinterpret_cast<char*>(&weight), sizeof(weight));

            string edge = std::to_string(src);
            edge += " ";
            edge += std::to_string(dst);
            edge += " ";
            edge += std::to_string(weight);
            if (graphSize != 1) {
                edge += "\n";
            }
            output.write(edge.c_str(), edge.size());
        }
    }
}

int main(int argc, char** argv)
{
    if (argc != 6) {
        std::cout << "Invalid number of parameters" << std::endl;
        return 0;
    }

    std::ifstream input;
    std::ofstream output;
    if (string(argv[1]) == "-s") {
        input.open(argv[3]);
        output.open(argv[5], std::ios::binary);
        serializeGraph(input, output);
    }
    else if (string(argv[1]) == "-d") {
        input.open(argv[3], std::ios::binary);
        output.open(argv[5]);
        deserializeGraph(input, output);
    }

    input.close();
    output.close();

    return 0;
}
