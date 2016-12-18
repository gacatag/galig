#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <chrono>
#include <ctime>

#include <sdsl/bit_vectors.hpp>
#include "Snap.h"

using namespace std;
using namespace sdsl;

TStr toTStr(string s) {
  return TStr(s.c_str());
}

struct MEM {
  unsigned int t;
  unsigned int p;
  unsigned int l;

  MEM() {
    t = -1;
    p = -1;
    l = -1;
  }

  MEM(TSIn& SIn) { }

  string toStr() {
    return "(" + to_string(t) + "," + to_string(p) + "," + to_string(l) + ")";
  }

  void Save(TSOut& SOut) const { }

};

struct MEMs_Graph {
  //Attributes
  TPt<TNodeEDatNet<MEM, TInt> > Graph;
  TIntStrH labels;
  vector<vector<vector<int> > > subpaths;

  //Constructor
  MEMs_Graph(const vector<vector<MEM > > MEMs, const vector<vector<int> > edges, const unsigned int plen, const int k, const rrr_vector<>::rank_1_type rank_BV, const rrr_vector<>::select_1_type select_BV) {
    Graph = TNodeEDatNet<MEM, TInt>::New();
    build(MEMs, edges, plen, k, rank_BV, select_BV);
    subpaths = vector<vector<vector<int> > >(Graph->GetNodes(), { vector<vector<int> > { vector<int> { } } });
  }

  //Methods
  int getId(MEM m) {
    int index = -1;
    for (TNodeEDatNet<MEM, TInt>::TNodeI NI = Graph->BegNI(); NI < Graph->EndNI(); NI++) {
      if(labels.GetDat(labels.GetKey(labels.GetKeyId(NI.GetId()))).EqI(toTStr(m.toStr()))) {
        index = NI.GetId();
        break;
      }
    }
    return index;
  }

  MEM getNodeAttr(const int i) {
    return Graph->GetNI(i).GetDat();
  }

  int getEdgeAttr(const int i, const int j) {
    return Graph->GetEI(i,j).GetDat();
  }

  /*********************************************************************
   * CONSTRUCTION 1
   ********************************************************************/

  TPt<TNodeEDatNet<MEM, TInt> > build(const vector<vector<MEM > > MEMs, const vector<vector<int> > edges, const unsigned int plen, const int k, const rrr_vector<>::rank_1_type rank_BV, const rrr_vector<>::select_1_type select_BV) {
    int nodes_index = 1;

    Graph->AddNode(0, MEM());
    labels.AddDat(0, "Start");
    unsigned int curr_p = 1;
    while(curr_p < MEMs.size()) {
      for(MEM m1 : MEMs[curr_p]) {
	int m1_index = getId(m1);
	if(m1_index == -1) {
	  m1_index = nodes_index;
	  nodes_index++;
	  Graph->AddNode(m1_index, m1);
	  Graph->AddEdge(0, m1_index, 0);
	  labels.AddDat(m1_index, toTStr(m1.toStr()));
	}

	unsigned int i = m1.p + 1;
	while(i < plen and i < m1.p + m1.l + k) {
	  for(MEM m2 : MEMs[i]) { //Per tutti i m2 "consecutivi" a m1
	    if(m1.p + m1.l != m2.p + m2.l) { //Se m1 e m2 non finiscono nello stesso punto sul pattern
	      if(m1.t != m2.t && m1.t + m1.l != m2.t + m2.l) { //Se m1 e m2 non iniziano e finiscono negli stessi punti sul testo
		if(rank_BV(m1.t - 1) == rank_BV(m2.t - 1)) { //Se m1 e m2 sono nello stesso nodo
		  //cout << "(1) Checking " << m1.toStr() << " -> " << m2.toStr() << endl;
		  if(m2.t > m1.t && m2.t < m1.t + m1.l + k && m1.t + m1.l != m2.t + m2.l) {
		    //cout << "\tLinking " << m1.toStr() << " to " << m2.toStr() << endl;
		    int m2_index = getId(m2);

		    if(m2_index == -1) {
		      m2_index = nodes_index;
		      nodes_index++;
		      Graph->AddNode(m2_index, m2);
		      labels.AddDat(m2_index, toTStr(m2.toStr()));
		    }

		    //Weight
		    int wt = m2.t - m1.t - m1.l;
		    int wp = m2.p - m1.p - m1.l;
		    int w;
		    if(wt<0 || wp<0) {
		      //cout << "1" << endl;
		      w = abs(wt - wp);
		    }
		    else {
		      //cout << "2" << endl;
		      w = max(wt, wp);
		    }

		    Graph->AddEdge(m1_index, m2_index, w);
		  }
		}
		else { //Se m1 e m2 sono in due nodi differenti
		  //cout << "(2) Checking " << m1.toStr() << " -> " << m2.toStr() << endl;
		  vector<int> curr_edge { rank_BV(m1.t), rank_BV(m2.t) };
		  if(find(edges.begin(), edges.end(), curr_edge) != edges.end()) {
		    if(m1.t + m1.l >= select_BV(rank_BV(m1.t-1) + 1) - k && m2.t <= select_BV(rank_BV(m2.t-1)) + k) {
		      //cout << "\tLinking " << m1.toStr() << " to " << m2.toStr() << endl;
		      int m2_index = getId(m2);

		      if(m2_index == -1) {
			m2_index = nodes_index;
			nodes_index++;
			Graph->AddNode(m2_index, m2);
			labels.AddDat(m2_index, toTStr(m2.toStr()));
		      }
		      //Weight
		      int wt = (select_BV(rank_BV(m1.t-1) + 1) - m1.t - m1.l) + (m2.t - select_BV(rank_BV(m2.t-1)) - 1);
		      int wp = abs(m2.p - m1.p - m1.l);
		      int w = max(wt, wp);
		      Graph->AddEdge(m1_index, m2_index, w);
		    }
		  }
		}
	      }
	    }
	  }
	  i++;
	}
      }
      curr_p++;
    }

    Graph->AddNode(nodes_index, MEM());
    labels.AddDat(nodes_index, "End");
    for (TNodeEDatNet<MEM, TInt>::TNodeI NI = Graph->BegNI(); NI < Graph->EndNI(); NI++) {
      if(NI.GetOutDeg() == 0 && NI.GetId() != nodes_index) {
	Graph->AddEdge(NI.GetId(), nodes_index, 0);
      }
    }

    return Graph;
  }

  /*********************************************************************
   * VISIT
   ********************************************************************/
  vector<vector<int> > visit() {
    return rec_visit(Graph->BegNI());
  }

  vector<vector<int> > rec_visit(const TNodeEDatNet<MEM, TInt>::TNodeI node) {
    int node_id = node.GetId();
    //cout << node_id;

    if(subpaths[node_id][0].size() != 0) {
      //cout << " break" << endl;
      return subpaths[node_id];
    }

    int out = node.GetOutDeg();
    if(out == 0) {
      //cout << " end" << endl;
      vector<vector<int> > starting_paths { vector<int> { node_id } };
      subpaths.at(node_id) = starting_paths;
      return starting_paths;
    }

    //cout << " rec" << endl;
    int i = 0;
    vector<vector<int> > paths;
    while(i < out) {
      int child_id = node.GetOutNId(i);
      i++;

      TNodeEDatNet<MEM, TInt>::TNodeI child = Graph->GetNI(child_id);
      vector<vector<int> > starting_paths = rec_visit(child);

      for(vector<int> sp : starting_paths) {
	vector<int> p { node_id };
	p.insert(p.end(),sp.begin(),sp.end());
	paths.push_back(p);
      }
    }
    subpaths.at(node_id) = paths;
    return paths;
  }

  /*********************************************************************
   * SAVE
   ********************************************************************/
  void save() {
    ofstream myfile;
    myfile.open("./out/graph.dot");

    string dot = "digraph G {\n graph [splines=true overlap=false]\n node  [shape=ellipse, width=0.3, height=0.3]\n";

    for (TNodeEDatNet<MEM, TInt>::TNodeI NI = Graph->BegNI(); NI < Graph->EndNI(); NI++) { 
      dot += " " + to_string(NI.GetId()) + " [label=\"" + labels.GetDat(labels.GetKey(labels.GetKeyId(NI.GetId()))).GetCStr() + "\"];\n";
    }

    for (TNodeEDatNet<MEM, TInt>::TEdgeI EI = Graph->BegEI(); EI < Graph->EndEI(); EI++) { 
      dot += " " + to_string(EI.GetSrcNId()) + " -> " + to_string(EI.GetDstNId()) + " [label=\" " + to_string(EI.GetDat()) + "\"];\n";
    }
    dot += "}";

    myfile << dot;
    myfile.close();
    if(system("dot -Tpng ./out/graph.dot -o ./out/graph.png") != 0) {
	cerr << "System call error" << endl;
    }
  }
};

MEM extractMEM(string line) {
  MEM m;
  string delimiter = ",";
  size_t pos = 0;
  string token;
  bool flag = true;
  while ((pos = line.find(delimiter)) != string::npos) {
    token = line.substr(0, pos);
    if(flag) {
      m.t = stoi(token);
    }
    else {
      m.p = stoi(token);
    }
    line.erase(0, pos + delimiter.length());
    flag = not(flag);
  }
  m.l = stoi(line);

  return m;
}

vector<vector<MEM > > extractMEMs(const string fpath, const unsigned int plen) {
  vector<vector<MEM > > MEMs (plen + 1, vector<MEM >());

  string line;
  ifstream memsFile(fpath);
  if (memsFile.is_open()) {
    while(getline(memsFile,line)) {
      MEM m = extractMEM(line);
      MEMs[m.p].push_back(m);
    }
    memsFile.close();
  }
  else {
    cout << "Unable to open MEMs file" << endl;
  }

  return MEMs;
}

vector<int> getExonsLengths(const string fpath) {
  vector<int> e_lens;

  string line;
  ifstream memsFile(fpath);
  if (memsFile.is_open()) {
    while(getline(memsFile,line)) {
      e_lens.push_back(stoi(line));
    }
    memsFile.close();
  }
  else {
    cout << "Unable to open exons file" << endl;
  }

  return e_lens;
}

vector<int> extractEdge(string line) {
  vector<int> edge;
  string delimiter = ",";
  size_t pos = 0;
  string token;
  while ((pos = line.find(delimiter)) != string::npos) {
    token = line.substr(0, pos);
    edge.push_back(stoi(token));
    line.erase(0, pos + delimiter.length());
  }
  edge.push_back(stoi(line));

  return edge;
}

vector<vector<int > > extractEdges(const string fpath) {
  vector<vector<int > > edges;

  string line;
  ifstream edgesFile(fpath);
  if (edgesFile.is_open()) {
    while(getline(edgesFile,line)) {
      vector<int> edge = extractEdge(line);
      edges.push_back(edge);
    }
    edgesFile.close();
  }
  else {
    cout << "Unable to open edges file" << endl;
  }

  return edges;
}

int main(int argc, char* argv[]) {
  chrono::time_point<chrono::system_clock> start, end;
  chrono::duration<double> elapsed_seconds;

  ofstream myfile;
  myfile.open("time_log");

  start = chrono::system_clock::now();
  // Input
  string mems_file = argv[1];
  string e_lens_file = argv[2];
  string edges_file = argv[3];
  //int L = stoi(argv[4]);
  int k = stoi(argv[5]);
  unsigned int plen = stoi(argv[6]);

  //Extracting exons lenghts from file
  vector<int> e_lens = getExonsLengths(e_lens_file);

  //Extracting edges from file
  vector<vector<int> > edges = extractEdges(edges_file);

  //Bit Vector Setup
  int tot_L = 1;
  for(int l:e_lens) {
    tot_L += l+1;
  }

  bit_vector BV(tot_L, 0);

  int i = 0;
  BV[i] = 1;
  for(int l:e_lens) {
    i += l+1;
    BV[i] = 1;
  }

  rrr_vector<> rrrb(BV);
  rrr_vector<>::rank_1_type rank_BV(&rrrb);
  rrr_vector<>::select_1_type select_BV(&rrrb);

  //Extracting MEMs from file
  vector<vector<MEM > > MEMs = extractMEMs(mems_file, plen);
  end = chrono::system_clock::now();
  
  elapsed_seconds = end-start;
  myfile << "Parsing input: " << elapsed_seconds.count() << "\n";
  //Build MEMs Graph
  start = chrono::system_clock::now();
  MEMs_Graph mg (MEMs, edges, plen, k, rank_BV, select_BV);
  end = chrono::system_clock::now();
  elapsed_seconds = end-start;
  myfile << "Building graph: " << elapsed_seconds.count() << "\n";
  // mg.save();
  start = chrono::system_clock::now();
  vector<vector<int> > paths = mg.visit();
  end = chrono::system_clock::now();
  elapsed_seconds = end-start;
  myfile << "Visiting  graph: " << elapsed_seconds.count() << "\n";

  start = chrono::system_clock::now();
  //Format output
  for(vector<int> path : paths) {
    unsigned int i = 0;
    while(i<path.size()-1) {
      mg.getNodeAttr(path[i]).toStr();
      mg.getEdgeAttr(path[i], path[i+1]);

      i++;
    }
  }
  end = chrono::system_clock::now();
  elapsed_seconds = end-start;
  myfile << "Formatting output: " << elapsed_seconds.count() << "\n\n";
  myfile.close();
}