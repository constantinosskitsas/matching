
#include "spectra.h"
#include "../../graph/graph.h"
#include <unordered_map>
#include "../eigenHelper.h"
#include "../GrM.h"
#include "../IO.h"
#include "../Experiments.h"
#include <numeric>
#include <mutex>

// using namespace cs;
using namespace std;
using namespace Eigen;
// using namespace Spectra;
using namespace std::chrono;
std::mutex fcsMutex;
std::mutex Mymutex;
std::mutex Mymutex1;
std::mutex mtx;

inline bool OneHopEigenPM(CSV &cvertex, unordered_map<ui, ui> &ID, ui *&SID, int *IDDLC,
                         float **&LM, map<ui, int> &EvalNeigb2, map<ui, int> &EvalNeigb, Graph *query_graph, vector<pair<VertexID, VertexID>> &q_curr, ui Omax)
{

    ID.insert({cvertex.ID, 0});
    IDDLC[0] = 1;
    VertexID vx1 = 0;
    VertexID vx2 = 0;
    ui count2 = 0;
    ui stop = EvalNeigb2.size();
    ui labela = 0;
    for (int k = 0; k < cvertex.edges.size(); k++)
    {
        if (cvertex.edges[k].first == 1000000)
            continue;
        
        SID[cvertex.edges[k].first]=1;
        auto result = ID.insert({cvertex.edges[k].second, IDDLC[0]});

        if (result.second)
        {   
            if (IDDLC[1] > 0)
            {
                labela = query_graph->getVertexLabel(cvertex.edges[k].first);
                EvalNeigb2[labela]--;
                EvalNeigb[labela]--;
                if (EvalNeigb2[labela] == 0)
                    IDDLC[1]--;
                if (EvalNeigb[labela] == 0)
                    IDDLC[2]--;
            }
            if (IDDLC[0] > Omax)
            {   
                return true;
            }
            else{
            LM[0][IDDLC[0]]=-1;
            LM[IDDLC[0]][0]=-1;
            IDDLC[0]++;
            }
        }
        q_curr.emplace_back(cvertex.edges[k]);
    }
    return true;
}
/*Second Hop Evaluation
*Omax size limit to calculate eigenvalues and Omax2 limit of 2-hop
* Extension of degree and Neighborhood safety pruning
*Add edges in triplet format.
*/

inline bool SecHopEigenLM(vector<pair<VertexID, VertexID>> &q_curr, unordered_map<ui, ui> &ID, ui *&SID,map<ui, int> &EvalNeigb2,
                               int *IDDLC, vector<vector<CSV>> &FCS, float **&LM, Graph *query_graph, ui Omax, int qID,CSV &cvertex)
{
    pair<VertexID, VertexID> temp1;
    VertexID tempxx = 0;
    vector<VertexID> temp2;
    VertexID vx1 = 0;
    VertexID vx2 = 0;
    VertexID DN = 0;
    unordered_set<ui> EdgeF;
    int kk = 0;
    ui labela = 0;
    VertexID vtemp = 0;
    vector<ui> tempx;
    ui counter=0;
    kk=0;
    ui omaxUP=500;
    while (kk < q_curr.size())
    {
        temp1 = q_curr[kk];
        //tempxx=tempx[kk];
        kk++;

        // QN = ;
        if (temp1.first == 1000000){
            
            continue;
        }
            
        // SID.insert(temp1.first);
        tempxx = findIndBS(FCS, temp1.second, temp1.first);
        
        vx1 = ID[FCS[temp1.first][tempxx].ID];
        for (int i = 0; i < FCS[temp1.first][tempxx].edges.size(); i++)
        {counter++;

             if (FCS[temp1.first][tempxx].edges[i].first == 1000000|| FCS[temp1.first][tempxx].edges[i].first == qID)
                continue;
            SID[FCS[temp1.first][tempxx].edges[i].first]=1;
            vtemp = FCS[temp1.first][tempxx].edges[i].second;
            auto [it1, success] = ID.try_emplace(vtemp, IDDLC[0]);

            if (success)
            {
                vx2 = IDDLC[0];
                IDDLC[0]++;
                if (IDDLC[1] > 0)
                {
                    labela = query_graph->getVertexLabel(FCS[temp1.first][tempxx].edges[i].first);
                    EvalNeigb2[labela]--;
                    if (EvalNeigb2[labela] == 0)
                        IDDLC[1]--;
                }
                if (IDDLC[0] > Omax )

                {  // if (counter>1000)
                    //cout<<"Exited counter "<<counter<<endl;
                    
                    return true;
                }

            }
            else
            {
                vx2 = ID[vtemp];
                
            }
            LM[vx1][vx2]=-1;
            LM[vx2][vx1]=-1;  
            if (counter>omaxUP){
                IDDLC[0]=Omax+1;
                return true;
           }
        }
    }
    q_curr.clear();
    //if (counter>1000)
    //cout<<"Non Exited counter "<<counter<<endl;
    return (true);
}

/*Extract Label Information for the query for 2hops
*Createas a map with Label ID and count that we can easily compare 
*with a candidate node. Self Included!
*/
void ExtractUI2h(vector<ui> &Deg, vector<map<ui, int>> &QueryNlabel2, Graph *query_graph, int qsiz)
{
    const VertexID *u_nbrs;
    ui u_nbrs_count;
    const VertexID *u_nbrsN;
    ui u_nbrs_countN;
    set<ui> QueryVec;
    map<ui, int> QueryVec1;
    ui labela = 0;
    for (int i = 0; i < qsiz; i++)
    {
        QueryVec.insert(i);
        u_nbrs = query_graph->getVertexNeighbors(i, u_nbrs_count);

        for (int j = 0; j < u_nbrs_count; j++)
        {   //First hop
            if (QueryVec.insert(u_nbrs[j]).second)
            {
                labela = query_graph->getVertexLabel(u_nbrs[j]);
                if (QueryVec1.count(labela) == 0)
                {
                    // Key does not exist, add it with a value of 1
                    QueryVec1[labela] = 1;
                }
                else
                {
                    // Key exists, increment its value
                    QueryVec1[labela]++;
                }
            }
                //second Hop
            u_nbrsN = query_graph->getVertexNeighbors(u_nbrs[j], u_nbrs_countN);
            for (int k = 0; k < u_nbrs_countN; k++)
            {   if(u_nbrsN[k] == i)
                    continue;
                labela = query_graph->getVertexLabel(u_nbrsN[k]);
                if (QueryVec.insert(u_nbrsN[k]).second)
                {
                    if (QueryVec1.count(labela) == 0)
                    {
                        // Key does not exist, add it with a value of 1
                        QueryVec1[labela] = 1;
                    }
                    else
                    {
                        // Key exists, increment its value
                        QueryVec1[labela]++;
                    }
                }
            }
        }
        Deg.emplace_back(QueryVec.size());
        QueryVec.clear();
        QueryNlabel2.emplace_back(QueryVec1);
        QueryVec1.clear();
    }
}

/*Neighborhood safety 1st hop Omax and Omax2 version.
*This funciton version is used as the first step of EIgenCalculation
*Stores edges of vertex in vector.
*Keeps track of unique CS and nodes.
*/

/*Checks the One hop Neigborhood Neigboorhood safety. 
*/
inline bool OneHopEigenMap(CSV &cvertex, map<ui, int> EvalNeigb, Graph *query_graph, unordered_set<ui> IDN1)
{

    ui count2 = 0;
    ui labela = 0;
    ui k;
    for (k = 0; k < cvertex.edges.size(); k++)
    {
        if (cvertex.edges[k].first == 1000000)
            continue;
        labela = query_graph->getVertexLabel(cvertex.edges[k].first);
        //check label only if it didnt pass the check yet
        if (EvalNeigb[labela] < 0)
            continue;
        //Count only unique ID
        if (IDN1.insert(cvertex.edges[k].second).second)
        {
            EvalNeigb[labela]--;
            if (EvalNeigb[labela] == 0)
            {
                count2++;
                if (count2 == EvalNeigb.size())
                    return true;
            }
        }
    }
    return false;
}

inline bool OneHopEigenMapMT(CSV &cvertex, map<ui, int> EvalNeigb, Graph *query_graph)
{
    unordered_set<ui> IDN1;
    ui count2 = 0;
    ui labela = 0;
    ui k;
    for (k = 0; k < cvertex.edges.size(); k++)
    {
        if (cvertex.edges[k].first == 1000000)
            continue;
        labela = query_graph->getVertexLabel(cvertex.edges[k].first);
        //check label only if it didnt pass the check yet
        if (EvalNeigb[labela] < 0)
            continue;
        //Count only unique ID
        if(IDN1.find(cvertex.edges[k].second)==IDN1.end())
        //if (IDN1.insert(cvertex.edges[k].second).second)
        {IDN1.insert(cvertex.edges[k].second);
            EvalNeigb[labela]--;
            if (EvalNeigb[labela] == 0)
            {
                count2++;
                if (count2 == EvalNeigb.size())
                    return true;
            }
        }
    }
    return false;
}


void allocateBufferFCS(vector<vector<CSV>> &FCS, const Graph *query_graph, ui **&candidates,
                       ui *&candidates_count)
{
    ui query_vertex_num = query_graph->getVerticesCount();
    // ui candidates_max_num = data_graph->getGraphMaxLabelFrequency();

    candidates_count = new ui[query_vertex_num];
    memset(candidates_count, 0, sizeof(ui) * query_vertex_num);

    candidates = new ui *[query_vertex_num];

    for (ui i = 0; i < query_vertex_num; ++i)
    {
        candidates[i] = new ui[FCS[i].size()];
    }
}
void allocateBufferFCS1(vector<vector<CSV>> &FCS, const Graph *query_graph, ui **&candidates,
                       ui *&candidates_count,float **&EWeight)
{
    ui query_vertex_num = query_graph->getVerticesCount();
    // ui candidates_max_num = data_graph->getGraphMaxLabelFrequency();

    candidates_count = new ui[query_vertex_num];
    memset(candidates_count, 0, sizeof(ui) * query_vertex_num);

    candidates = new ui *[query_vertex_num];
    EWeight= new float *[query_vertex_num];
    for (ui i = 0; i < query_vertex_num; ++i)
    {
        candidates[i] = new ui[FCS[i].size()];
        EWeight[i] = new float[FCS[i].size()];
    }
}
/*Extract Label Information for the query
*Createas a map with Label ID and count that we can easily compare 
*with a candidate node
*/
void ExtractNImap(vector<map<ui, int>> &QueryNlabel, Graph *query_graph, int qsiz)
{
    const VertexID *u_nbrs;

    ui u_nbrs_count;
    for (int i = 0; i < qsiz; i++)
    {
        u_nbrs = query_graph->getVertexNeighbors(i, u_nbrs_count);
        map<ui, int> QueryVec;

        for (int j = 0; j < u_nbrs_count; j++)
        {
            ui labela = query_graph->getVertexLabel(u_nbrs[j]);
            if (QueryVec.count(labela) == 0)
            {
                // Key does not exist, add it with a value of 1
                QueryVec[labela] = 1;
            }
            else
            {
                // Key exists, increment its value
                QueryVec[labela]++;
            }
        }
        QueryNlabel.emplace_back(QueryVec);
        QueryVec.clear();
    }
}

/*Initial Pruning. After the CS creation we remove nodes that have
*less edges that their CS. 
*/
bool InitPrunTCSR(vector<vector<CSV>> &FCS, int qsiz, Graph *query_graph)
{
    int jj = 0;
    ui VDP;
    VertexID i = 0;
    VertexID rev;
    bool ret = false;
    for (VertexID kk = 0; kk < qsiz; kk++)
    {
        jj = FCS[kk].size();
        VDP = query_graph->getVertexDegree(kk);
        while (jj > 0)
        {
            jj--;
            if (FCS[kk][jj].Ichange == true)
            {
                if (FCS[kk][jj].edges.size() == 0)
                {
                    ret = true;
                    FCS[kk][jj].deleted = true;
                }
                //pruning rule
                else if (FCS[kk][jj].edges.size() < VDP)
                {
                    i = 0;

                    while (i < FCS[kk][jj].edges.size())
                    {
                        if (FCS[kk][jj].edges[i].first == 1000000)
                        {
                            i++;
                            continue;
                        }

                        rev = findIndBS(FCS, FCS[kk][jj].edges[i].second, FCS[kk][jj].edges[i].first); // vertex to remove ID?
                        FCS[FCS[kk][jj].edges[i].first][rev].Ichange = true;
                        for (int dd = 0; dd < FCS[FCS[kk][jj].edges[i].first][rev].edges.size(); dd++)
                            if (FCS[FCS[kk][jj].edges[i].first][rev].edges[dd].first == kk && FCS[FCS[kk][jj].edges[i].first][rev].edges[dd].second == FCS[kk][jj].ID)
                            {
                                // FCS[FCS[kk][jj].edges[i].first][rev].edges.erase(FCS[FCS[kk][jj].edges[i].first][rev].edges.begin() + dd);
                                FCS[FCS[kk][jj].edges[i].first][rev].edges[dd].first = 1000000;
                                i++;
                                break;
                            }
                    }
                    // FCS[kk].erase(FCS[kk].begin() + jj);
                    FCS[kk][jj].deleted = true;
                    ret = true;
                }

                FCS[kk][jj].Ichange = false;
            }
        }
    }
/*
    for (auto &row : FCS)
    {
        row.erase(remove_if(row.begin(), row.end(), [&](CSV &csv)
                            {
        if (csv.deleted) {
            return true;
        }
        auto newEnd = remove_if(csv.edges.begin(), csv.edges.end(), [](const pair<VertexID, VertexID> &edge) {
            return edge.first == 100000;
        });
        csv.edges.erase(newEnd, csv.edges.end());
        return false; }),
                  row.end());
    }*/

    return ret;
}

bool InitPrunTCSRMT(vector<vector<CSV>> &FCS, int qsiz, Graph *query_graph)
{   
    auto IPMT = [](Graph *query_graph,vector<vector<CSV>> &FCS,int start,int qsiz,int *pos,bool *res){
    int jj = 0;
    ui VDP;
    VertexID i = 0;
    VertexID rev;
    
    int kk=start;
    while(kk<qsiz)
    {
        jj = FCS[kk].size();
        VDP = query_graph->getVertexDegree(kk);
        while (jj > 0)
        {
            jj--;
            if (FCS[kk][jj].Ichange == true)
            {
                if (FCS[kk][jj].edges.size() == 0)
                {
                    *res = true;
                    FCS[kk][jj].deleted = true;
                }
                //pruning rule
                else if (FCS[kk][jj].edges.size() < VDP)
                {
                    i = 0;

                    while (i < FCS[kk][jj].edges.size())
                    {
                        if (FCS[kk][jj].edges[i].first == 1000000)
                        {
                            i++;
                            continue;
                        }

                        rev = findIndBS(FCS, FCS[kk][jj].edges[i].second, FCS[kk][jj].edges[i].first); // vertex to remove ID?
                        FCS[FCS[kk][jj].edges[i].first][rev].Ichange = true;
                        for (int dd = 0; dd < FCS[FCS[kk][jj].edges[i].first][rev].edges.size(); dd++)
                            if (FCS[FCS[kk][jj].edges[i].first][rev].edges[dd].first == kk && FCS[FCS[kk][jj].edges[i].first][rev].edges[dd].second == FCS[kk][jj].ID)
                            {
                                // FCS[FCS[kk][jj].edges[i].first][rev].edges.erase(FCS[FCS[kk][jj].edges[i].first][rev].edges.begin() + dd);
                                FCS[FCS[kk][jj].edges[i].first][rev].edges[dd].first = 1000000;
                                i++;
                                break;
                            }
                    }
                    // FCS[kk].erase(FCS[kk].begin() + jj);
                    FCS[kk][jj].deleted = true;
                    *res = true;
                }

                FCS[kk][jj].Ichange = false;
            }
        }mtx.lock();
        (*pos)++;
        kk=*pos;
        mtx.unlock();
    }
              
    };
    bool ret = false;
    bool *ret1=&ret;
        int Tnum=5;
    thread th[Tnum];
    int pos=Tnum-1;
    int *pos1=&pos;
    for (int d = 0; d < Tnum; d++)
    {
        th[d] = thread(IPMT,query_graph,ref(FCS),d,qsiz,pos1,ret1);
        
    }
   
    for (int d = 0; d < Tnum; d++)
        th[d].join();

    return *ret1;
}


void EdgesCSBasicSetMT(vector<vector<CSV>> &FCS, int qsiz, int dsiz, Graph *data_graph, Graph *query_graph)
{

    auto EDMT = [](Graph *data_graph,Graph *query_graph,vector<unordered_map<ui, ui>> &s,vector<vector<CSV>> &FCS,int start,int qsiz,int *pos)
    {
        ui u_nbrs_count = 0;
        ui u_nbrs_countD = 0;
        int sizA = 0;
        int sizC;
        VertexID VID = 0;
        VertexID de = 0;
        VertexID cne = 0;
        VertexID labela = 0;
        int a=start;
        while(a<qsiz)
    { //take the neiboors of the query node FCS[a]
        const VertexID *u_nbrs = query_graph->getVertexNeighbors(a, u_nbrs_count);
        // VertexID* u_nbrs = query_graph->getVertexNeighbors(a, u_nbrs_count);
        sizA = FCS[a].size();
        //Now for every node of the neigboors of FCS[a]---
        for (VertexID c = 0; c < u_nbrs_count; c++)
        {   //we start checking query nodes with smaller id to higher
            //thus is the query neigboor has smaller ID we already 
            //added the edge to the CS.
            //if (u_nbrs[c] < a)
            //    continue;
            cne = u_nbrs[c];
            labela = query_graph->getVertexLabel(cne);
            sizC = FCS[cne].size();
            //For every node of the CS(i)-> the candidates query node we evaluate
            //candidate vertex for a query a is FCS[a][b].ID
            for (VertexID b = 0; b < sizA; b++)
            { 
                VID = FCS[a][b].ID;
                // const VertexID* u_nbrsD = data_graph->getVertexNeighbors(FCS[a][b].ID, u_nbrs_countD); //real neigboors of the candidate vertex
                 //get all the neigbors of the FCS[a][b].ID in the data graph
                const VertexID *u_nbrsD = data_graph->getNeighborsByLabel(FCS[a][b].ID, labela, u_nbrs_countD); // real neigboors of the candidate vertex
               // for every neigboor of the candidate vertex of the real graph
               //check if the node exists in the set of neigboors
                for (VertexID e = 0; e < u_nbrs_countD; e++)
                { 
                    // if(u_nbrsD[e]== FCS[cne][d].ID){
                    auto got = s[cne].find(u_nbrsD[e]);
                    if (got != s[cne].end())
                    {   //Mymutex.lock();
                        FCS[a][b].edges.emplace_back(make_pair(cne, FCS[cne][got->second].ID));
                        //FCS[cne][got->second].edges.emplace_back(make_pair(a, VID));
                        //Mymutex.unlock();
                        //FCS[cne][got->second].edges.emplace_back(make_pair(a, VID));
                    }
                }
            }
        }
        mtx.lock();
        (*pos)++;
        a=*pos;
        mtx.unlock();     

    }
    };


    //unordered_map<ui, ui> s[qsiz];
    vector<std::unordered_map<ui, ui>> s(qsiz);
    //create a set with unique edges for every CS(i)
    for (int i = 0; i < qsiz; i++)
    {
        s[i].reserve(FCS[i].size());
        for (int j = 0; j < FCS[i].size(); j++)
            s[i].insert({FCS[i][j].ID, j});
    }
    // for CS. for every node of the CS(i)

    int Tnum=5;
    thread th[Tnum];
    int pos=Tnum-1;
    int *pos1=&pos;
    for (int d = 0; d < Tnum; d++)
    {
        th[d] = thread(EDMT,data_graph,query_graph,ref(s),ref(FCS),d,qsiz,pos1);
        
    }
   
    for (int d = 0; d < Tnum; d++)
        th[d].join();

        for (int i = 0; i < qsiz; i++)
    {
        s[i].clear();
    } 
}


/*Add edges to the candidate space based on the paper rules.
*/
void EdgesCSBasicSet(vector<vector<CSV>> &FCS, int qsiz, int dsiz, Graph *data_graph, Graph *query_graph)
{
    ui u_nbrs_count = 0;
    ui u_nbrs_countD = 0;
    int sizA = 0;
    int sizC;
    VertexID VID = 0;
    VertexID de = 0;
    VertexID cne = 0;
    VertexID labela = 0;
    unordered_map<ui, ui> s[qsiz];
    //create a set with unique edges for every CS(i)
    for (int i = 0; i < qsiz; i++)
    {
        s[i].reserve(FCS[i].size());
        for (int j = 0; j < FCS[i].size(); j++)
            s[i].insert({FCS[i][j].ID, j});
    }
    // for CS. for every node of the CS(i)
    for (VertexID a = 0; a < qsiz; a++)
    { //take the neiboors of the query node FCS[a]
        const VertexID *u_nbrs = query_graph->getVertexNeighbors(a, u_nbrs_count);
        // VertexID* u_nbrs = query_graph->getVertexNeighbors(a, u_nbrs_count);
        sizA = FCS[a].size();
        //Now for every node of the neigboors of FCS[a]---
        for (VertexID c = 0; c < u_nbrs_count; c++)
        {   //we start checking query nodes with smaller id to higher
            //thus is the query neigboor has smaller ID we already 
            //added the edge to the CS.
            if (u_nbrs[c] < a)
                continue;
            cne = u_nbrs[c];
            labela = query_graph->getVertexLabel(cne);
            sizC = FCS[cne].size();
            //For every node of the CS(i)-> the candidates query node we evaluate
            //candidate vertex for a query a is FCS[a][b].ID
            for (VertexID b = 0; b < sizA; b++)
            { 
                VID = FCS[a][b].ID;
                // const VertexID* u_nbrsD = data_graph->getVertexNeighbors(FCS[a][b].ID, u_nbrs_countD); //real neigboors of the candidate vertex
                 //get all the neigbors of the FCS[a][b].ID in the data graph
                const VertexID *u_nbrsD = data_graph->getNeighborsByLabel(FCS[a][b].ID, labela, u_nbrs_countD); // real neigboors of the candidate vertex
               // for every neigboor of the candidate vertex of the real graph
               //check if the node exists in the set of neigboors
                for (VertexID e = 0; e < u_nbrs_countD; e++)
                { 
                    // if(u_nbrsD[e]== FCS[cne][d].ID){
                    auto got = s[cne].find(u_nbrsD[e]);
                    if (got != s[cne].end())
                    {
                        FCS[a][b].edges.emplace_back(make_pair(cne, FCS[cne][got->second].ID));
                        FCS[cne][got->second].edges.emplace_back(make_pair(a, VID));
                    }
                }
            }
        }
    }
    for (int i = 0; i < qsiz; i++)
    {
        s[i].clear();
    }
}
/* Add vertices that Pass NLF and Eigen rule to candidate Space.
* OpenData1 need to be removed from here and pass the eigenVD1 as parameter(index)
**310 is hardcodes max number of label ID-> can be just extracted from data graph->number of Labels.
**I preload for every label all the possible candidate nodes, so I will not have to reload them if 
**a query node has the same label.
**Important to note by construction the nodes are ordered by ID so FCS[0][0].ID<FCS[0][1].ID
**1)Pruning 1 l(q)=l(v) So labelsNum[reverseLab[label]] already has nodes that have the same label
**2)Pruning 2 d(q)<=d(v) Degree
**3)Pruning 3 LE(q)<=LE(v) top S laplacian
**4)Pruning 4 N(q)<=N(v) Neigboorhood labels 
*/ 
void Vertices(vector<vector<CSV>> &FCS, int qsiz, int dsiz, Graph *data_graph, Graph *query_graph, float **&eigenVq1, vector<map<ui, int>> &QueryNlabel,float **&eigenVD1)
{

    
    VectorXd devalues;
    VectorXd qevalues;
    bool con = true;
    ui com = data_graph->getGraphMaxLabelFrequency();
    ui copies = query_graph->getLabelsCount() + 1;
    ui labelsNum[copies];
    ui kk;
    ui i;
    ui j;
    ui reverseLab[310];
    ui u_nbrs_countD = 0;
    const ui *labelData[copies];
    LabelID label = 0;
    for (i = 0; i < 310; i++)
        reverseLab[i] = 310;
    int pos = 0;
    for (i = 0; i < qsiz; i++)
    {
        label = query_graph->getVertexLabel(i);
        ui vdata_vertex_num = 0;
        if (reverseLab[label] == 310)
        {
            reverseLab[label] = pos;
            labelData[pos] = data_graph->getVerticesByLabel(label, vdata_vertex_num);
            labelsNum[pos] = vdata_vertex_num;
            pos++;
        }
    }
    ui reserveS;
    vector<CSV> CS;
    ui k = 0;
    ui degree = 0;
    ui data_vertex_num;

    int prunES = qsiz-3;
    prunES=30;
    //for every C(q)
    ui vdata_vertex_num = 0;
    for (i = 0; i < qsiz; i++)
    {
        label = query_graph->getVertexLabel(i);
        degree = query_graph->getVertexDegree(i);
        
        data_vertex_num = 0;
        reserveS = com * degree;
        //L(q)=L(v) same label
        for (j = 0; j < labelsNum[reverseLab[label]]; ++j)
        {
            //get Vertex ID
            VertexID data_vertex = labelData[reverseLab[label]][j];
            //D(q)<=D(v)
            if (data_graph->getVertexDegree(data_vertex) >= degree)
            {
                con = true;
                
                //Eigen Value Pruning up to pruneEs value
                for (kk = 0; kk < prunES; kk++)
                {      //quick termination with qsiz
                    //if(eigenVq1[i][kk]<=-1)
                    //    break;
                    if (eigenVD1[data_vertex][kk] < eigenVq1[i][kk])
                    //Rounding errors for eigenvalue
                        if ((eigenVq1[i][kk] - eigenVD1[data_vertex][kk]) > 0.0001)
                        {
                            con = false;
                            break;
                        }
                } 
                
                if (con)
                {
                    //Neigborhood Check
                    for (auto it = QueryNlabel[i].begin(); it != QueryNlabel[i].end(); ++it)
                    {
                        data_graph->getNeighborsByLabelCount(data_vertex, it->first, u_nbrs_countD);
                        if (u_nbrs_countD < it->second)
                        {
                            con = false;
                            break;
                        }
                    }
                    //If all rules true -> add to candidate space
                    if (con)
                    {
                        //CSV cat(10, data_vertex, reserveS);
                        CSV cat(data_vertex);
                        CS.emplace_back(cat);
                    }
                }
            }
        }
        FCS.emplace_back(CS);
        CS.clear();
    }
}

void VerticesMT2(vector<vector<CSV>> &FCS, int qsiz, int dsiz, Graph *data_graph, Graph *query_graph, float **&eigenVq1 , vector<map<ui, int>> &QueryNlabel,float **&eigenVD1)
{   
    auto MTVert = [](int *pos,int i,int qsiz,ui* labelsNum,const ui** labelData,Graph *data_graph,Graph *query_graph,float **eigenVD1, float **eigenVq1,vector<map<ui, int>> &QueryNlabel,vector<vector<CSV>> &FCS,ui *reverseLab )
    {
        int prunES = 30;
        ui reserveS;
        ui u_nbrs_countD;
        vector<CSV> CS;
        ui com = data_graph->getGraphMaxLabelFrequency();
        
    while (i<qsiz)    
    
    {
        
        ui label = query_graph->getVertexLabel(i);
        ui degree = query_graph->getVertexDegree(i);
        int data_vertex_num = 0;
        reserveS = com * degree;
        bool con=false;

        for (int j = 0; j < labelsNum[reverseLab[label]]; ++j)
        {//cout<<"in1"<<endl;
            VertexID data_vertex = labelData[reverseLab[label]][j];
            //
            if (data_graph->getVertexDegree(data_vertex) >= degree)
            {//cout<<"in2"<<endl;

                con = true;
                for (int kk = 0; kk < prunES; kk++)
                {
                    if(eigenVq1[i][kk]<=-1)
                        break;
                    if (eigenVD1[data_vertex][kk] < eigenVq1[i][kk])
                        if (eigenVq1[i][kk] - eigenVD1[data_vertex][kk] > 0.001)
                        {
                            con = false;
                            break;
                        }
                } // con=true;
                if (con)
                {//cout<<"in4"<<endl;

                    for (auto it = QueryNlabel[i].begin(); it != QueryNlabel[i].end(); ++it)
                    {
                        data_graph->getNeighborsByLabelCount(data_vertex, it->first, u_nbrs_countD);
                        if (u_nbrs_countD < it->second)
                        {
                            con = false;
                            break;
                        }
                    }
                    if (con)
                    {
                        CSV cat(data_vertex);
                        CS.emplace_back(cat);
                    }
                }
            }
        }
        fcsMutex.lock();
        FCS[i].resize(CS.size());
        FCS[i]=CS;
        //FCS.insert(FCS.begin() + i, CS);
        (*pos)++;
        //cout<<i<<endl;
        i=*pos;
        fcsMutex.unlock();     
        CS.clear();
    }
    };
    VectorXd devalues;
    VectorXd qevalues;
    bool con = true;
    ui com = data_graph->getGraphMaxLabelFrequency();
    ui copies = query_graph->getLabelsCount() + 1;
   // ui labelsNum[copies];
    ui* labelsNum = new ui[copies];
    ui kk;
    ui i;
    ui j;
    ui LS = data_graph->getLabelsCount() + 3;
    //ui reverseLab[LS]; // 310 magic number
    ui* reverseLab = new ui[LS];
    ui u_nbrs_countD;
    //const ui *labelData[copies];
    const ui **labelData;
    

// Allocate memory for an array of pointers
labelData = (const ui **)malloc(copies * sizeof(const ui *));   
    //cout<<"sure"<<endl;
    LabelID label = 0;
    for (i = 0; i < LS; i++)
        reverseLab[i] = LS;
    int pos = 0;
    for (i = 0; i < qsiz; i++)
    {
        label = query_graph->getVertexLabel(i);
        ui vdata_vertex_num = 0;
        if (reverseLab[label] == LS)
        {
            reverseLab[label] = pos;
            labelData[pos] = data_graph->getVerticesByLabel(label, vdata_vertex_num);
            labelsNum[pos] = vdata_vertex_num;
            pos++;
        }
    }
    
    vector<CSV> CS;
    ui k = 0;
    ui degree = 0;
    ui data_vertex_num;

    int prunES = 5;
    prunES = qsiz - 3;
    prunES=30;
    int Tnum=5;
    thread th[Tnum];
    pos=Tnum-1;
    int *pos1=&pos;
    for (int d = 0; d < Tnum; d++)
    {
        th[d] = thread(MTVert,pos1,d,qsiz,labelsNum,labelData,data_graph,query_graph, eigenVD1,eigenVq1,ref(QueryNlabel),ref(FCS),reverseLab);
        
    }
   
    for (int d = 0; d < Tnum; d++)
        th[d].join();
}


/*Removed all nodes from the Candidate space that are set to be pruned.
**While iterating we set edges to 1000000 as a max value 
**and notes to csv deleted. Then we gather all the nodes and edges to
** remove them all together to avoid reallocations and reorderings
*/
inline void clearWrong(vector<vector<CSV>> &FCS)
{
    for (auto &row : FCS)
    {
        row.erase(remove_if(row.begin(), row.end(), [&](CSV &csv)
                            {
        if (csv.deleted) {
            return true;
        }
        auto newEnd = remove_if(csv.edges.begin(), csv.edges.end(), [](const pair<VertexID, VertexID> &edge) {
            return edge.first == 1000000;
        });
        csv.edges.erase(newEnd, csv.edges.end());
        return false; }),
                  row.end());
    }
}


/*Prune Node and edges, set node to deleted and edge to 1000000.
**Also find all neigboors of node and set the edges to the pruned node to 1000000
**Checks also if the remove edge makes the node to be pruned by the Nedge rule.
*/
inline void removeVertexAndEgjesFK(vector<vector<CSV>> &FCS, int i, int deli)
{   
    VertexID vx1;
    // CSV cvertexpair;
    
    for (int j = 0; j < FCS[i][deli].edges.size(); j++)
    {
        // BSCHange
        if (FCS[i][deli].edges[j].first == 1000000)
            continue;
        vx1 = findIndBS(FCS, FCS[i][deli].edges[j].second, FCS[i][deli].edges[j].first);
        FCS[FCS[i][deli].edges[j].first][vx1].change = true;
        FCS[FCS[i][deli].edges[j].first][vx1].IPchange = true;
        Mymutex.lock();
        FCS[FCS[i][deli].edges[j].first][vx1].Nedge[i]--;

        if (FCS[FCS[i][deli].edges[j].first][vx1].Nedge[i] == 0)
            FCS[FCS[i][deli].edges[j].first][vx1].NedgeC = true;
        Mymutex.unlock();
        for (int k = 0; k < FCS[FCS[i][deli].edges[j].first][vx1].edges.size(); k++)
        {
            if (FCS[FCS[i][deli].edges[j].first][vx1].edges[k].first == 1000000)
                continue;
            if (FCS[FCS[i][deli].edges[j].first][vx1].edges[k].first == i && FCS[FCS[i][deli].edges[j].first][vx1].edges[k].second == FCS[i][deli].ID)
            {
                FCS[FCS[i][deli].edges[j].first][vx1].edges[k].first = 1000000;
                break;
            }
        }
    }

    FCS[i][deli].edges.clear();
    FCS[i][deli].deleted = true;
    /*for (auto &row : FCS)
    {
        row.erase(remove_if(row.begin(), row.end(), [&](CSV &csv)
                            {
        if (csv.deleted) {
            return true;
        }
        auto newEnd = remove_if(csv.edges.begin(), csv.edges.end(), [](const pair<VertexID, VertexID> &edge) {
            return edge.first == 1000000;
        });
        csv.edges.erase(newEnd, csv.edges.end());
        return false; }),
                  row.end());
    }*/
}
/*Prune Node and edges, set node to deleted and edge to 1000000.
**Also find all neigboors of node and set the edges to the pruned node to 1000000
**Checks also if the remove edge makes the node to be pruned by the Nedge rule.
*/
inline void removeVertexAndEgjesFKNPMT(vector<vector<CSV>> &FCS, int i, int deli)
{
    VertexID vx1;
    ui j;
    ui k;
    // CSV cvertexpair;
    vector<ui> zz;
    vector<ui> z1;
    vector<ui> z2;
    for (j = 0; j < FCS[i][deli].edges.size(); j++)
    {
        // BSCHange
        if (FCS[i][deli].edges[j].first == 1000000)
            continue;
        vx1 = findIndBS(FCS, FCS[i][deli].edges[j].second, FCS[i][deli].edges[j].first);
        FCS[FCS[i][deli].edges[j].first][vx1].IPchange = true;
        FCS[FCS[i][deli].edges[j].first][vx1].change = true;
        zz.emplace_back(j);
        z1.emplace_back(vx1);
        
        for (k = 0; k < FCS[FCS[i][deli].edges[j].first][vx1].edges.size(); k++)
        {
            if (FCS[FCS[i][deli].edges[j].first][vx1].edges[k].first == 1000000)
                continue;
            if (FCS[FCS[i][deli].edges[j].first][vx1].edges[k].first == i && FCS[FCS[i][deli].edges[j].first][vx1].edges[k].second == FCS[i][deli].ID)
            {   z2.emplace_back(k);
                //FCS[FCS[i][deli].edges[j].first][vx1].edges[k].first = 1000000;
                break;
            }
        }
    }
    Mymutex.lock();
        
       
    for (int aa=0;aa<zz.size();aa++){
        FCS[FCS[i][deli].edges[zz[aa]].first][z1[aa]].Nedge[i]--;
        
        if (FCS[FCS[i][deli].edges[zz[aa]].first][z1[aa]].Nedge[i] == 0)
            FCS[FCS[i][deli].edges[zz[aa]].first][z1[aa]].NedgeC = true;
        FCS[FCS[i][deli].edges[zz[aa]].first][z1[aa]].edges[z2[aa]].first = 1000000;
    } Mymutex.unlock();
    zz.clear();
    z1.clear();
    z2.clear();
    FCS[i][deli].edges.clear();
    FCS[i][deli].deleted = true;

}
inline void removeVertexAndEgjesFKNP(vector<vector<CSV>> &FCS, int i, int deli)
{
    VertexID vx1;
    ui j;
    ui k;
    // CSV cvertexpair;
    
    for (j = 0; j < FCS[i][deli].edges.size(); j++)
    {
        // BSCHange
        if (FCS[i][deli].edges[j].first == 1000000)
            continue;
        vx1 = findIndBS(FCS, FCS[i][deli].edges[j].second, FCS[i][deli].edges[j].first);
        FCS[FCS[i][deli].edges[j].first][vx1].IPchange = true;
        FCS[FCS[i][deli].edges[j].first][vx1].change = true;
        Mymutex.lock();
        FCS[FCS[i][deli].edges[j].first][vx1].Nedge[i]--;
        
        if (FCS[FCS[i][deli].edges[j].first][vx1].Nedge[i] == 0)
            FCS[FCS[i][deli].edges[j].first][vx1].NedgeC = true;
        Mymutex.unlock();
        for (k = 0; k < FCS[FCS[i][deli].edges[j].first][vx1].edges.size(); k++)
        {
            if (FCS[FCS[i][deli].edges[j].first][vx1].edges[k].first == 1000000)
                continue;
            if (FCS[FCS[i][deli].edges[j].first][vx1].edges[k].first == i && FCS[FCS[i][deli].edges[j].first][vx1].edges[k].second == FCS[i][deli].ID)
            {
                FCS[FCS[i][deli].edges[j].first][vx1].edges[k].first = 1000000;
                break;
            }
        }
    }


    FCS[i][deli].edges.clear();
    FCS[i][deli].deleted = true;

}
/* 
    Loads the query graph, 
    Calculates the eigenvalues for the query
*/
int SpectralMatching(int sizd, Graph *data_graph, string input_query_graph_file, int twohop, ui **&candidates, ui *&candidates_count,float **&EWeight,float **&eigenVD1)
{
    ui** candidates1 = NULL;
    
    ui* candidates_count1 = NULL;
    
    Graph *query_graph = new Graph(true);
    query_graph->loadGraphFromFile(input_query_graph_file);
    
    int sizq = query_graph->getVerticesCount();
    ui Eprun = sizq - 3;
    Eprun=30;

    MatrixXd eigenVq1(sizq, Eprun);
    int oMax=sizq*3;
    oMax=300;
    MTcalc12(query_graph, query_graph->getGraphMaxDegree(), eigenVq1, true, Eprun,oMax);
    float **eigenQ = NULL;
    eigenQ = new float *[sizq];
    
    for (ui i = 0; i < sizq; ++i)
    {
        eigenQ[i] = new float[Eprun];
        for (ui j = 0; j < Eprun; j++)
        {
            eigenQ[i][j] = eigenVq1(i, j);
        }
    }
    
    //cout<<CSInitMT(data_graph, query_graph, eigenQ, twohop, candidates1, candidates_count1,EWeight,eigenVD1)<<endl;
    return CSInitMT(data_graph, query_graph, eigenQ, twohop, candidates, candidates_count,EWeight,eigenVD1);
}

/*Assuming FCS[i][j].ID is sorted we cand find any j given IDC=i and IDQ=FCS[i][j].ID with Binary search.
**Keep in mind sorted properties have to be kept when removing elements.
*/
inline VertexID findIndBS(vector<vector<CSV>> &FCS, VertexID IDC, VertexID IDQ)
{
    int lo = 0, hi = FCS[IDQ].size() - 1;
    int mid;
    // This below check covers all cases , so need to check
    // for mid=lo-(hi-lo)/2
    while (hi - lo > 1)
    {
        int mid = (hi + lo) / 2;
        if (FCS[IDQ][mid].ID < IDC)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }
    if (FCS[IDQ][lo].ID == IDC)
    {
        return lo;
    }
    else if (FCS[IDQ][hi].ID == IDC)
    {
        return hi;
    }
    cout << "error Prob" << endl;
    return -10000;
}

/*Degree check for every node so we avoid computations in the future.
**Assuming that to be here the degree check is valid
**For every query vertex the node has to have at least 1 edge in 
**a neigborhood candidate space. We start by adding all the count
**for every edge and every time we remove 1 edge we decrease the count.
**
**
*/
void fillEN(vector<vector<CSV>> &FCS, int qsiz, Graph *query_graph)
{
    for (int i = 0; i < qsiz; i++)
    {
        ui de = query_graph->getVertexDegree(i);

        for (int j = 0; j < FCS[i].size(); j++)
        {   
            FCS[i][j].Nedge=new int[qsiz];
            memset(FCS[i][j].Nedge, 0, sizeof(int) * qsiz);
            for (int kk = 0; kk < FCS[i][j].edges.size(); kk++)
            {
                FCS[i][j].Nedge[FCS[i][j].edges[kk].first]++;
            }
                ui sd=0;
            for(int kk=0;kk<qsiz;kk++){
                if(FCS[i][j].Nedge[kk]!=0)
                sd++;
            }
            //No needed as to be here it passed the Degree Check
            if (de > sd)
                FCS[i][j].NedgeC = true;

            //if (de < sd)
            //    cout << "Sanity Check" << endl;
        }
    }
}


void fillENMT(vector<vector<CSV>> &FCS, int qsiz, Graph *query_graph)
{
    auto ENMT = [](Graph *query_graph,vector<vector<CSV>> &FCS,int start,int qsiz,int *pos){
   int i=start;
   while(i<qsiz)
    {
        ui de = query_graph->getVertexDegree(i);

        for (int j = 0; j < FCS[i].size(); j++)
        {   
           // FCS[i][j].Nedge=new int[qsiz];
            //memset(FCS[i][j].Nedge, 0, sizeof(int) * qsiz);
            for (int kk = 0; kk < FCS[i][j].edges.size(); kk++)
            {
                FCS[i][j].Nedge[FCS[i][j].edges[kk].first]++;
            }
                ui sd=0;
            for(int kk=0;kk<qsiz;kk++){
                if(FCS[i][j].Nedge[kk]!=0)
                sd++;
            }
            //No needed as to be here it passed the Degree Check
            if (de > sd)
                FCS[i][j].NedgeC = true;

            //if (de < sd)
            //    cout << "Sanity Check" << endl;
        }
    mtx.lock();
        (*pos)++;
        i=*pos;
        mtx.unlock();
    }
    };
    for (int i=0;i<qsiz;i++){
        for (int j=0;j<FCS[i].size();j++){
            FCS[i][j].Nedge=new int[qsiz];
            memset(FCS[i][j].Nedge, 0, sizeof(int) * qsiz);
        }
    }
        int Tnum=5;
    thread th[Tnum];
    int pos=Tnum-1;
    int *pos1=&pos;
    for (int d = 0; d < Tnum; d++)
    {
        th[d] = thread(ENMT,query_graph,ref(FCS),d,qsiz,pos1);
        
    }
   
    for (int d = 0; d < Tnum; d++)
        th[d].join();
}


/* CSInit is unfortunate name, contains all the pruning operations of EigenValues
*   ** ExtractNImap and ExtractUI2h was calculated together at the begining but
   ** due to remove/addition of 1-2 hop I had it separated. 
   ** Can be added together again however there is real no runtime cost.
   **clearWrong() Removes nodes.
*
*/
int CSInit(Graph *data_graph, Graph *query_graph, float **&eigenVq1, int twohop, ui **&candidates, ui *&candidates_count,float **&EWeight,float **&eigenVD1)
{   
    int qsiz = query_graph->getVerticesCount();
    int dsiz = data_graph->getVerticesCount();
    vector<vector<CSV>> FCS; //candidate space
    FCS.reserve(qsiz);
    vector<vector<CSV>> FCS1(qsiz);
    //    vector<vector<CSV>> FCS1; //candidate space
    //FCS1.reserve(qsiz);
    vector<ui> DegreeK; //Discovered nodes for 2 hop
    vector<vector<pair<ui, int>>> QueryNlabel; 
    vector<map<ui, int>> NLabel; //Number of Labels 1hop
    vector<map<ui, int>> NLabel2; //Number of Labels 2hop 
    

    //Exctract 1hop label information for query graph
    ExtractNImap(NLabel, query_graph, qsiz);
    //Extract 2hop label information for query graph
    ExtractUI2h(DegreeK, NLabel2, query_graph, qsiz);

    //Initial Pruning add remaining Candidate Vertices to Candidate Space

     
    Vertices(FCS, qsiz, dsiz, data_graph, query_graph, eigenVq1, NLabel,eigenVD1);
    //VerticesMT2(FCS1, qsiz, dsiz, data_graph, query_graph, eigenVq1, NLabel,eigenVD1);
    for (int aa=0;aa<qsiz;aa++)
    FCS.emplace_back(FCS1[aa]);
    
    int count = 0;
    int Tcount = 0;
   
    //Add Edges between nodes in candidate space 
    EdgesCSBasicSet(FCS, qsiz, dsiz, data_graph, query_graph);
    //EdgesCSBasicSetMT(FCS, qsiz, dsiz, data_graph, query_graph);
   
  
    // Get candidate nodes neigborhood information for fast pruning¨
    //Initial Pruning on Candidate Space
    while (InitPrunTCSR(FCS, qsiz, query_graph))     
    clearWrong(FCS);
    //while (InitPrunTCSRMT(FCS, qsiz, query_graph))     
    //clearWrong(FCS);

    fillEN(FCS, qsiz, query_graph);
    //fillENMT(FCS, qsiz, query_graph);

    int GDegree = query_graph->getGraphMaxDegree();
    //Neigborhood Pruning

    int cc=0;
        while (ReverseRefinementNOTESN(NLabel,
                                  FCS, qsiz, query_graph, GDegree))
       clearWrong(FCS);
      
      
   /*   bool st=true;
 st= ReverseRefinementNOTESNMT(NLabel,
                                  FCS, qsiz, query_graph, GDegree);
                                  clearWrong(FCS);
   // int ct=0;
    while (st==true){
                                   st= ReverseRefinementNOTESN(NLabel,
                              FCS, qsiz, query_graph, GDegree);
                                    clearWrong(FCS);
                                    
                                  }
        
                        
                                        
            */                      
       
    //Not used Anymore-without the one degree rule


        ui mc=0;
      //if (twohop<=1)
         mc=3;
         //2nd hop and EigenPruning
     
       while (RefinementEigen(NLabel, NLabel2, FCS, qsiz, query_graph, eigenVq1, DegreeK, twohop)&&mc<5)
       //while (RefinementEigenMT2(NLabel, NLabel2, FCS, qsiz, query_graph, eigenVq1, DegreeK, twohop)&&mc<5)
       
        { 
            mc++;
            clearWrong(FCS);
            while (ReverseRefinementNOTESN(NLabel, FCS, qsiz, query_graph, GDegree))
                clearWrong(FCS);
        }
    //allocateBufferFCS(FCS, query_graph, candidates, candidates_count);
    //ADD the candidates to the format That In-Memory Paper has everything.
    allocateBufferFCS1(FCS, query_graph, candidates, candidates_count,EWeight);

    for (int i = 0; i < qsiz; i++)
    {
        for (int j = 0; j < FCS[i].size(); j++){
            candidates[i][j] = FCS[i][j].ID;

            //ED is for Eigen Ordering -> Not used.
           EWeight[i][j]=FCS[i][j].ED;
            //EWeight[i][j]=FCS[i][j].edges.size();
            
        }

        candidates_count[i] = FCS[i].size();
    }

    int totalCand = 0;
    for (int i = 0; i < query_graph->getVerticesCount(); i++)
    {
        // cout<<"C(i) "<<candidates_count[i]<<",";
        totalCand = candidates_count[i] + totalCand;
    }
    return totalCand;
}

int CSInitMT(Graph *data_graph, Graph *query_graph, float **&eigenVq1, int twohop, ui **&candidates, ui *&candidates_count,float **&EWeight,float **&eigenVD1)
{   
    int qsiz = query_graph->getVerticesCount();
    int dsiz = data_graph->getVerticesCount();
    vector<vector<CSV>> FCS; //candidate space
    FCS.reserve(qsiz);
    vector<vector<CSV>> FCS1(qsiz);
    //    vector<vector<CSV>> FCS1; //candidate space
    //FCS1.reserve(qsiz);
    vector<ui> DegreeK; //Discovered nodes for 2 hop
    vector<vector<pair<ui, int>>> QueryNlabel; 
    vector<map<ui, int>> NLabel; //Number of Labels 1hop
    vector<map<ui, int>> NLabel2; //Number of Labels 2hop 
    //DegreeK[i]= sum(NLabel2[i])

    //Exctract 1hop label information for query graph
    ExtractNImap(NLabel, query_graph, qsiz);
    //Extract 2hop label information for query graph
    ExtractUI2h(DegreeK, NLabel2, query_graph, qsiz);
    //auto stop4 = high_resolution_clock::now();
    //auto duration4 = duration_cast<milliseconds>(stop4 - start4);
    //cout << "Extract Query Info " << duration4.count() << endl;
    //Initial Pruning add remaining Candidate Vertices to Candidate Space
    // start4 = high_resolution_clock::now();
     
    //Vertices(FCS, qsiz, dsiz, data_graph, query_graph, eigenVq1, NLabel,eigenVD1);
    VerticesMT2(FCS1, qsiz, dsiz, data_graph, query_graph, eigenVq1, NLabel,eigenVD1);
    for (int aa=0;aa<qsiz;aa++)
    FCS.emplace_back(FCS1[aa]);
    
     //stop4 = high_resolution_clock::now();
     //duration4 = duration_cast<milliseconds>(stop4 - start4);
    //cout << "Vertices Pruning" << duration4.count() << endl;
    int count = 0;
    int Tcount = 0;
   // start4 = high_resolution_clock::now();
    //Add Edges between nodes in candidate space 
    //EdgesCSBasicSet(FCS, qsiz, dsiz, data_graph, query_graph);
    EdgesCSBasicSetMT(FCS, qsiz, dsiz, data_graph, query_graph);
     //   stop4 = high_resolution_clock::now();
     //duration4 = duration_cast<milliseconds>(stop4 - start4);
    //cout << "Edges CS " << duration4.count() << endl;
    // Get candidate nodes neigborhood information for fast pruning¨
   // start4 = high_resolution_clock::now();
    //Initial Pruning on Candidate Space
    //while (InitPrunTCSR(FCS, qsiz, query_graph))     
    //clearWrong(FCS);
    while (InitPrunTCSRMT(FCS, qsiz, query_graph))     
    clearWrong(FCS);
    // stop4 = high_resolution_clock::now();
    // duration4 = duration_cast<milliseconds>(stop4 - start4);
   // cout << "IP " << duration4.count() << endl;
  //  start4 = high_resolution_clock::now();
    //fillEN(FCS, qsiz, query_graph);
    fillENMT(FCS, qsiz, query_graph);
   // stop4 = high_resolution_clock::now();
  //   duration4 = duration_cast<milliseconds>(stop4 - start4);
  //  cout << "fill EN " << duration4.count() << endl;
    int GDegree = query_graph->getGraphMaxDegree();
    //Neigborhood Pruning
  //  start4 = high_resolution_clock::now();
    int cc=0;
    //while (ReverseRefinementNOTESN(NLabel,
     //                             FCS, qsiz, query_graph, GDegree))
    //   clearWrong(FCS);
      
      
      bool st=true;
 st= ReverseRefinementNOTESNMT(NLabel,
                                  FCS, qsiz, query_graph, GDegree);
                                  clearWrong(FCS);
   // int ct=0;
    while (st==true){
                                   st= ReverseRefinementNOTESN(NLabel,
                              FCS, qsiz, query_graph, GDegree);
                                    clearWrong(FCS);
                                    
                                  }
        
                        
                                        
                                

        ui mc=0;
         mc=3;
     
       //while (RefinementEigen(NLabel, NLabel2, FCS, qsiz, query_graph, eigenVq1, DegreeK, twohop)&&mc<5)
       while (RefinementEigenMT2(NLabel, NLabel2, FCS, qsiz, query_graph, eigenVq1, DegreeK, twohop)&&mc<5)
       
        { 
            mc++;
            clearWrong(FCS);
            while (ReverseRefinementNOTESN(NLabel, FCS, qsiz, query_graph, GDegree))
                clearWrong(FCS);
        }
  //  stop4 = high_resolution_clock::now();
  //   duration4 = duration_cast<milliseconds>(stop4 - start4);
  //  cout << "Twohop " << duration4.count() << endl;
    //allocateBufferFCS(FCS, query_graph, candidates, candidates_count);
    //ADD the candidates to the format That In-Memory Paper has everything.
    allocateBufferFCS1(FCS, query_graph, candidates, candidates_count,EWeight);

    for (int i = 0; i < qsiz; i++)
    {
        for (int j = 0; j < FCS[i].size(); j++){
            candidates[i][j] = FCS[i][j].ID;

            //ED is for Eigen Ordering -> Not used.
           EWeight[i][j]=FCS[i][j].ED;
            //EWeight[i][j]=FCS[i][j].edges.size();
            
        }

        candidates_count[i] = FCS[i].size();
    }

    int totalCand = 0;
    for (int i = 0; i < query_graph->getVerticesCount(); i++)
    {
        // cout<<"C(i) "<<candidates_count[i]<<",";
        totalCand = candidates_count[i] + totalCand;
    }
    return totalCand;
}

/*Counts the total number of candidates.
*Now that we remove nodes the function is not needed
**as a single loop can get FCS[i].size for all i.
*/
int CSSizeReal(vector<vector<CSV>> &FCS, int qsiz)
{
    int count = 0;
    int Tcount = 0;
    for (int kk = 0; kk < qsiz; kk++)
    {
        for (int zz = 0; zz < FCS[kk].size(); zz++)
            // if(FCS[kk][zz].ID!=1000000)
            if (FCS[kk][zz].deleted == false)
            {
                count++;
            } 

        Tcount = count + Tcount;
        count = 0;
    }
    return Tcount;
}

/*Neighborhood NLF in candidate space.
*/

bool ReverseRefinementNOTESN(vector<map<ui, int>> NLabel, vector<vector<CSV>> &FCS, int qsiz, Graph *query_graph, ui GDegree)
{
    unordered_set<ui> SID;
    SID.reserve(GDegree / 4);
    bool returnhere = true;
    ui i;
    ui j;
    ui IDC;
    ui pos;
    ui NI;
    //while(returnhere==true){
       returnhere=false;
    for (i = 0; i < qsiz; i++)
    { 

        for (j = 0; j < FCS[i].size(); j++)
        {
            // cvertex = FCS[i][j];
            if (FCS[i][j].IPchange == false || FCS[i][j].deleted == true)
                continue;
            //1 degree rule check
            
            if (!FCS[i][j].NedgeC)
                for (int ia=0;ia<query_graph->getVerticesCount();ia++){
                    
                    if(FCS[i][j].Nedge[ia]==1&&NLabel[i][query_graph->getVertexLabel(ia)]>1){
                        for (int aa=0;aa<FCS[i][j].edges.size();aa++){
                            if(ia==FCS[i][j].edges[aa].first){
                                IDC=FCS[i][j].edges[aa].second;
                                pos=aa;
                                break;
                            }
                        }
                        for (int aa=0;aa<FCS[i][j].edges.size();aa++){
                            if(FCS[i][j].edges[aa].second==IDC &&aa!=pos && FCS[i][j].edges[aa].first!=1000000){
                              
                                ui queryN=FCS[i][j].edges[aa].first;
                                NI = findIndBS(FCS,IDC,queryN);
                                for (int ao=0;ao< FCS[queryN][NI].edges.size();ao++){
                                    if (FCS[queryN][NI].edges[ao].first==i &&FCS[queryN][NI].edges[ao].second==FCS[i][j].ID){
                                        //remove neigboor
                                        FCS[queryN][NI].edges[ao].first=1000000;
                                        FCS[queryN][NI].Nedge[i]--;
                                        if(FCS[queryN][NI].Nedge[i]==0)
                                        FCS[queryN][NI].NedgeC=true;
                                        FCS[queryN][NI].change=true;
                                        FCS[queryN][NI].IPchange=true;
                                        
                                        //remove edge
                                        FCS[i][j].Nedge[queryN]--;
                                        if(FCS[i][j].Nedge[queryN]==0)
                                        FCS[i][j].NedgeC=true;
                                        FCS[i][j].edges[aa].first=1000000;
                                        break;
                                    }

                                }
                            }
                                

                        }
                        
                    }

                }
             
             //Degree Check
             if (FCS[i][j].NedgeC)
            {
                removeVertexAndEgjesFKNP(FCS, i, j);
                returnhere = true;
            }//Neighborhood Check
            else if (!OneHopEigenMap(FCS[i][j], NLabel[i], query_graph, SID))
            {
                removeVertexAndEgjesFKNP(FCS, i, j);
                returnhere = true;
            }
            else
            {
                FCS[i][j].IPchange = false;
            }
        }
    }
    //clearWrong(FCS);
    //}

    return returnhere;
}

bool ReverseRefinementNOTESNMT(vector<map<ui, int>> &NLabel, vector<vector<CSV>> &FCS, int qsiz, Graph *query_graph, ui GDegree)
{
    
    auto ROH = [](int *pos1,vector<vector<CSV>> &FCS,int i, int siz,vector<map<ui, int>> &NLabel, 
         Graph *query_graph,ui GDegree,int d,bool *returnhere){
        unordered_set<ui> SID;
    //SID.reserve(GDegree / 4);
    ui IDC;
    ui pos;
    ui NI;
    
    ui num=10;
    int svertex;
        int evertex;
        int opd=d;
        while ((opd*num)<siz){
            //cout<<opd<<endl;
            svertex=opd*num;
            evertex=svertex+num;
            if(evertex>siz)
                evertex=siz;

        for (int j = svertex; j < evertex; j++)
        {
            // cvertex = FCS[i][j];
            if (FCS[i][j].IPchange == false || FCS[i][j].deleted == true)
                continue;
            //1 degree rule check
            
            if (!FCS[i][j].NedgeC)
                for (int ia=0;ia<query_graph->getVerticesCount();ia++){
                    
                    if(FCS[i][j].Nedge[ia]==1&&NLabel[i][query_graph->getVertexLabel(ia)]>1){
                        for (int aa=0;aa<FCS[i][j].edges.size();aa++){
                            if(ia==FCS[i][j].edges[aa].first){
                                IDC=FCS[i][j].edges[aa].second;
                                pos=aa;
                                break;
                            }
                        }
                        for (int aa=0;aa<FCS[i][j].edges.size();aa++){
                            if(FCS[i][j].edges[aa].second==IDC &&aa!=pos && FCS[i][j].edges[aa].first!=1000000){
                              
                                ui queryN=FCS[i][j].edges[aa].first;
                                NI = findIndBS(FCS,IDC,queryN);
                                for (int ao=0;ao< FCS[queryN][NI].edges.size();ao++){
                                    if (FCS[queryN][NI].edges[ao].first==i &&FCS[queryN][NI].edges[ao].second==FCS[i][j].ID){
                                        //remove neigboor
                                        //remove edge
                                        //combined for better mutex
                                       
                                        Mymutex.lock();
                                        FCS[queryN][NI].Nedge[i]--;
                                        FCS[i][j].Nedge[queryN]--;
                                
                                        if(FCS[queryN][NI].Nedge[i]==0)
                                        FCS[queryN][NI].NedgeC=true;
                                        
                                        if(FCS[i][j].Nedge[queryN]==0)
                                        FCS[i][j].NedgeC=true;
                                         Mymutex.unlock();

                                        FCS[queryN][NI].edges[ao].first=1000000;
                                        FCS[queryN][NI].change=true;
                                        FCS[queryN][NI].IPchange=true;
                                        FCS[i][j].edges[aa].first=1000000;
                                       
                                        break;
                                    }

                                }
                            }
                                

                        }
                        
                    }

                }
             
             //Degree Check
             if (FCS[i][j].NedgeC)
            {
                removeVertexAndEgjesFKNPMT(FCS, i, j);
                *returnhere = true;
            }//Neighborhood Check
            else if (!OneHopEigenMapMT(FCS[i][j], NLabel[i], query_graph))
            {
                removeVertexAndEgjesFKNPMT(FCS, i, j);
                *returnhere = true;
            }
            else
            {
                FCS[i][j].IPchange = false;
            }
        }
    mtx.lock();
        (*pos1)++;
        opd=*pos1;
        
        mtx.unlock();
    }
         };


    bool returnhere = true;
    
    
    bool *ret1=&returnhere;
        int Tnum=5;

    
    ui num=10;
    //while(returnhere==true){
        returnhere = false;
    
    for (int i=0;i<qsiz;i++){
        int siz =  FCS[i].size();
        int Tnum=5;
        if(Tnum*num>siz){
        
        Tnum=int(siz/num);
        if ((Tnum*num)!=siz)
            Tnum=Tnum+1;
    
    }//cout<<"i ,j"<<i<<" , "<<siz<<endl;
    thread th[Tnum];
    int pos=Tnum-1;
    
    int *pos1=&pos;
    
        for (int d = 0; d < Tnum; d++)
    {
        th[d] = thread(ROH,pos1, ref(FCS),i,siz,ref(NLabel),query_graph,GDegree,d,ret1);
    }
    
    for (int d = 0; d < Tnum; d++)
        th[d].join();
    }
    //clearWrong(FCS);
   // }
    //return false;
    return returnhere;
}

/*Main function for Eigenvalue Pruning.
**Small Changes Added OMax and Omax2 First is limit for eigenvalues and second limit to twohop.
** We store the edges as triplets nd create a sparse Eigen MatrixXD
** We calculate eigenValues only if it passed the second pruning rule and the size of Matrix is less than oMax
*/
bool RefinementEigen(vector<map<ui, int>> NLabel, vector<map<ui, int>> NLabel2, vector<vector<CSV>> &FCS,
 int qsiz, Graph *query_graph, float **&eigenVq1, vector<ui> DM, int twohop)
{
    vector<T> tripletList;
    std::map<int, int> count_uniques;
    std::set<std::pair<int, int>> seen;
    std::vector<Triplet<double>> unique_triplets;
    unordered_map<ui, ui> SID; //number of unique nodes visited
    unordered_set<ui> SIDD;   //number of unique query nodes (q) visited 
    //SIDD.reserve(qsiz);

    int IDDLC[3] = {0, 0, 0};
    bool returnhere = false;
    VertexID vertex = 0;
    vector<VertexID> temp2;
    vector<pair<VertexID, VertexID>> q_curr;

    int Eprun = qsiz - 3;
    Eprun=30;
    VertexID vertexDegree = 0;
    VertexID vertexlabel = 0;
    ui SIDDSize = 0;
    bool continueE = false;
    bool con = true;
    ui oMax;
    ui oMax2;
    oMax = 150;
    //oMax2=qsiz * 5;
    if (twohop==0){
    oMax = 25;   
    }
    else if (twohop==1){
    oMax = 50;   
    }else if (twohop==2){
    oMax = 75;   
    }else if (twohop==3){
    oMax = 100;   
    }else if (twohop==4){
    oMax = 125;   
    }
    //if(qsiz*3<150)
    //oMax=qsiz*3;
    //cout<<"oMax"<<oMax<<endl;
            float** LM = new float*[oMax+1];
    ui* SIDN = new ui[qsiz];
    for (int i = 0; i <= oMax; i++) {
        LM[i] = new float[oMax+1];
        memset(LM[i], 0, oMax+1 * oMax+1 * sizeof(float));
    }
    //oMax=00;
    //oMax2=600;
    //SID.reserve(oMax + 10);
    //tripletList.reserve(oMax * 2);
    VectorXd evalues(Eprun);
    ui i;
    ui j;
    float sumD = 0;
    map<ui, int> NLabelT;
    map<ui, int> NLabelT1;
    /*
    *Ordered filter
    int sorted_indices[qsiz];
        if(twohop==10){
    int FO[qsiz];

    for (int i=0;i<qsiz;i++){
        FO[i]=FCS[i].size();
    }
    
    iota(sorted_indices, sorted_indices + qsiz, 0);
    sort(sorted_indices, sorted_indices + qsiz,
              [&FO](int i, int j){return FO[i] > FO[j];});
    }*/
    for (int dd = 0; dd < qsiz; dd++)
    {   i=dd;
        ui NDL=query_graph->getVertexDegree(i);
        /**
         * *ORdered filter
        ** if(twohop==10){
        **    i=sorted_indices[dd];
        **    NDL+1;
        }*/
            

            
        //if(twohop==1 &&NDL==1){
        //cout<<"test";
        //        continue;}
            
        for (j = 0; j < FCS[i].size(); j++)
        {
            if (FCS[i][j].deleted == true || FCS[i][j].change == false)
                continue;
            if (!FCS[i][j].NedgeC)
            {
                tripletList.clear();
                q_curr.clear();
                SID.clear();
                IDDLC[0] = 0;
                //SIDD.clear();
                //SIDD.insert(i);
                NLabelT = NLabel2[i];
                NLabelT1 = NLabel[i];
                IDDLC[1] = NLabel2[i].size();
                IDDLC[2] = NLabel[i].size();

                for (int aa=0;aa<oMax;aa++){
                    for (int bb=0;bb<oMax;bb++){
                        LM[aa][bb]=0;
                    }
                }
                for (int aa=0;aa<qsiz;aa++)
                SIDN[aa]=0;
                SIDN[i]=1;
                SIDDSize=0;

                OneHopEigenPM(FCS[i][j], SID, SIDN, IDDLC, LM, NLabelT,NLabelT1,
                                 query_graph, q_curr, oMax); 

                if (IDDLC[0] <= oMax||(twohop==100 &&IDDLC[0] <= oMax2))
                //if (IDDLC[0] <= oMax)
                {

                    //if (IDDLC[2] <= 0)

                    //{   
                        SecHopEigenLM(q_curr, SID, SIDN, NLabelT, IDDLC, FCS, LM, query_graph, oMax, i,FCS[i][j]);
                      for (int aa=0;aa<qsiz;aa++)
                        if(SIDN[aa]==1)
                            SIDDSize ++; }
                        //SIDDSize = SIDD.size();
                    //}
                //}
                //if (IDDLC[0] <= oMax ||(twohop==100 &&IDDLC[0] <= oMax2))
                if (IDDLC[0] <= oMax )
                
                    if (IDDLC[0] < DM[i] || IDDLC[1] > 0 || SIDDSize < DM[i] || (IDDLC[2] > 0))
                    {
                        removeVertexAndEgjesFK(FCS, i, j);
                        returnhere = true;
                        //continue;
                        IDDLC[0] = oMax + 1;
                    }

                //if (IDDLC[0] <= oMax && (NDL>1 || twohop!=1))
                //if (IDDLC[0] <= oMax && (NDL>1 )&&twohop!=3)
                //if (IDDLC[0] <= oMax && (NDL>1 ))
                if (IDDLC[0] <= oMax)
                {   //if(IDDLC[0]!=SID.size())
                        //cout<<IDDLC[0]<<" ID "<<SID.size()<<" SID"<<endl;
                    if(false){
                                            count_uniques.clear();
                    seen.clear();
                    unique_triplets.clear();
                    for (auto t : tripletList)
                    {
                        if (seen.count({t.row(), t.col()}) == 0)
                        {
                            unique_triplets.push_back(Triplet<double>(t.row(), t.col(), -1));
                            seen.insert({t.row(), t.col()});
                            count_uniques[t.row()]++;
                        }
                    }
                    for (auto it : count_uniques)
                    {
                        unique_triplets.push_back(Triplet<double>(it.first, it.first, it.second));
                    }
                    tripletList = unique_triplets;
                    }else{
                        ui cunt=0;
                        for (int wish=0;wish<IDDLC[0];wish++){
                             cunt=0;
                            for (int best=0;best<IDDLC[0];best++){
                                if (LM[wish][best]==-1){
                                    cunt++;
                                    tripletList.emplace_back(T(wish, best, -1));
                                }
                            }
                            tripletList.emplace_back(T(wish, wish, cunt));
                        }
                    }

                    if (tripletList.size() == IDDLC[0] * IDDLC[0])
                    {//cout<<"hi"<<endl;
                        evalues.resize(Eprun);

                        for (ui ss = 0; ss < Eprun; ss++)
                        {
                            if (ss < IDDLC[0]){
                                evalues(ss) = IDDLC[0] ;
                            }
                                
                            else if (ss == IDDLC[0]-1)
                                evalues(ss) = 0;
                            else
                                //evalues(ss) = -1;
                                evalues(ss) = 0;
                        }
                    }
                    else
                    {
                        SparseMatrix<double> M(IDDLC[0], IDDLC[0]);
                        M.setFromTriplets(tripletList.begin(), tripletList.end(), [](double a, double b)
                                          { return b; });
                                           M.makeCompressed();
                                            //if(!isLaplacianMatrix(M))
                                            //cout<<"help";
                        calcEigens1(M, Eprun, evalues, IDDLC[0]);
                        //calcEigensEigenLib(M,Eprun,evalues,IDDLC[0]);
                    }
                    con = true;
                    sumD = 0;
                    for (int dd = 0; dd < Eprun; dd++)
                    {
                        //if (eigenVq1[i][dd] <= -1) //last change
                        //    break;
                        if (evalues[dd] < eigenVq1[i][dd])
                        {
                            if ((eigenVq1[i][dd] - evalues[dd]) > 0.0001)
                            {
                                con = false;
                                break;
                            }
                        }
                        //Eigen Ordering If we want to eigenvalues uncomment.
                        //Add the values up 
                        else
                            sumD += (evalues[dd] - eigenVq1[i][dd]);
                    }
                    if (!con)
                    {
                        removeVertexAndEgjesFK(FCS, i, j);
                        returnhere = true;
                    }

                    else
                    {
                        FCS[i][j].change = false;
                        //Eigen Ordering
                        FCS[i][j].ED = sumD;
                    }
                }
                else
                {//cout<<"i "<<i<<"j "<<j;
                    FCS[i][j].change = false;
                }
            }
            else
            {
                removeVertexAndEgjesFK(FCS, i, j);
                returnhere = true;
            }
        }
    }
    return returnhere;
}

bool RefinementEigenMT2(vector<map<ui, int>> &NLabel, vector<map<ui, int>> &NLabel2, vector<vector<CSV>> &FCS,
 int qsiz, Graph *query_graph, float **&eigenVq1, vector<ui> &DM, int twohop)
{

        auto AdJAdl1 = [](int *pos,vector<vector<CSV>> &FCS,int i, VertexID d, VertexID siz,vector<map<ui, int>> &NLabel, vector<map<ui, int>> &NLabel2,
         Graph *query_graph, float **eigenVq1, vector<ui> &DM,int twohop,int oMax,int qsiz,int oMax2, bool *returnhere)
    {   //mutex mutex;
       
        int sumD;
        ui Eprun=30;
        vector<T> tripletList;
        bool continueE = false;
        bool con = true;
        int SIDDSize = 0;
        unordered_map<ui, ui> SID; //number of unique nodes visited
        unordered_set<ui> SIDD;   //number of unique query nodes (q) visited 
        int IDDLC[3] = {0, 0, 0};
        map<ui, int> NLabelT;
        map<ui, int> NLabelT1;
        VectorXd evalues(Eprun);
        float** LM = new float*[oMax+1];
        ui* SIDN = new ui[qsiz];
        std::map<int, int> count_uniques;
        std::set<std::pair<int, int>> seen;
        std::vector<Triplet<double>> unique_triplets;
        VertexID vertex = 0;
        vector<VertexID> temp2;
        vector<pair<VertexID, VertexID>> q_curr;
    
        VertexID vertexDegree = 0;
        VertexID vertexlabel = 0;
        
        int svertex;
        int evertex;
    for (int dd = 0; dd <= oMax; dd++) {
        LM[dd] = new float[oMax+1];
        memset(LM[dd], 0, oMax+1 * oMax+1 * sizeof(float));
    }
        ui num=10;
        while ((d*num)<siz){
            svertex=d*num;
            evertex=svertex+num;
            if(evertex>siz)
                evertex=siz;
        for (int j = svertex; j < evertex; j++)
        {
            if (FCS[i][j].deleted == true || FCS[i][j].change == false)
                continue;
            if (!FCS[i][j].NedgeC)
            {
                tripletList.clear();
                q_curr.clear();
                SID.clear();
                IDDLC[0] = 0;
                NLabelT = NLabel2[i];
                NLabelT1 = NLabel[i];
                IDDLC[1] = NLabel2[i].size();
                IDDLC[2] = NLabel[i].size();
                for (int aa=0;aa<oMax;aa++){
                    for (int bb=0;bb<oMax;bb++){
                        LM[aa][bb]=0;
                    }
                }
                for (int aa=0;aa<qsiz;aa++)
                SIDN[aa]=0;
                SIDN[i]=1;
                SIDDSize=0;
                OneHopEigenPM(FCS[i][j], SID, SIDN, IDDLC, LM, NLabelT,NLabelT1,
                                 query_graph, q_curr, oMax); 

                if (IDDLC[0] <= oMax||(twohop==100 &&IDDLC[0] <= oMax2))
                
                {
                SecHopEigenLM(q_curr, SID, SIDN, NLabelT, IDDLC, FCS, LM, query_graph, oMax, i,FCS[i][j]);
                      for (int aa=0;aa<qsiz;aa++)
                        if(SIDN[aa]==1)
                            SIDDSize ++; }
                if (IDDLC[0] <= oMax )
                
                    if (IDDLC[0] < DM[i] || IDDLC[1] > 0 || SIDDSize < DM[i] || (IDDLC[2] > 0))
                    {   // Mymutex.lock();
                       // removeVertexAndEgjesFK(FCS, i, j);
                        removeVertexAndEgjesFKNPMT(FCS, i, j);
                       // Mymutex.unlock();
                        *returnhere = true;
                        IDDLC[0] = oMax + 1;
                    }
                if (IDDLC[0] <= oMax)
                //if (false)
                {  
                        ui cunt=0;
                        for (int wish=0;wish<IDDLC[0];wish++){
                             cunt=0;
                            for (int best=0;best<IDDLC[0];best++){
                                if (LM[wish][best]==-1){
                                    cunt++;
                                    tripletList.emplace_back(T(wish, best, -1));
                                }
                            }
                            tripletList.emplace_back(T(wish, wish, cunt));
                        }
                    
                    //VectorXd evalues(Eprun);
                    if (tripletList.size() == IDDLC[0] * IDDLC[0])
                    {   
                        evalues.resize(Eprun);

                        for (ui ss = 0; ss < Eprun; ss++)
                        {
                            if (ss < IDDLC[0]){
                                evalues(ss) = IDDLC[0] ;
                            }
                                
                            else if (ss == IDDLC[0]-1)
                                evalues(ss) = 0;
                            else
                                evalues(ss) = -1;
                        }
                    }
                    else
                    {
                        SparseMatrix<double> M(IDDLC[0], IDDLC[0]);
                        M.setFromTriplets(tripletList.begin(), tripletList.end(), [](double a, double b)
                                          { return b; });
                                           M.makeCompressed();
                        //calcEigensEigenLib(M,Eprun,evalues,IDDLC[0]);
                        calcEigens1(M, Eprun, evalues, IDDLC[0]);
                    }
                    con = true;
                    sumD = 0;
                
                    
                    for (int dd = 0; dd < Eprun; dd++)
                    {
                        //if (eigenVq1[i][dd] <= -1) 
                       //     break;
                        if (evalues[dd] < eigenVq1[i][dd])
                        {
                            if ((eigenVq1[i][dd] - evalues[dd]) > 0.0001)
                            {  
                                con = false;
                                break;
                            }
                        }
                        else
                            sumD += (evalues[dd] - eigenVq1[i][dd]);
                    }
                    if (!con)
                    {
                        // Mymutex.lock();
                        //removeVertexAndEgjesFK(FCS, i, j);
                        removeVertexAndEgjesFKNPMT(FCS, i, j);
                       // Mymutex.unlock();
                        *returnhere = true;
                    }

                    else
                    {
                        FCS[i][j].change = false;
                        FCS[i][j].ED = sumD;
                    }
                }
                else
                {
                    FCS[i][j].change = false;
                }
            }
            else
            {
                 //Mymutex.lock();
                       //removeVertexAndEgjesFK(FCS, i, j);
                        removeVertexAndEgjesFKNPMT(FCS, i, j);
                     //   Mymutex.unlock();
                *returnhere = true;
            }
        }
        mtx.lock();
        (*pos)++;
        d=*pos;
        //cout<<d<<","<<d*num<<endl;
        mtx.unlock();
    }
    

    };

    bool returnhere = true;
    bool *ret=&returnhere;
    ui oMax;
    ui oMax2;
    oMax = 150;

    if (twohop==0){
    oMax = 25;   
    }
    else if (twohop==1){
    oMax = 50;   
    }else if (twohop==2){
    oMax = 75;   
    }else if (twohop==3){
    oMax = 100;   
    }else if (twohop==4){
    oMax = 125;   
    }
    ui i;
    ui j;
    float sumD = 0;
    for (int dd = 0; dd < qsiz; dd++)
    {   i=dd;
    
        
    ui NDL=query_graph->getVertexDegree(i);
    int Tnum=5;
    ui num=10;
    int siz =  FCS[i].size();
    if(Tnum*num>siz){
        
        Tnum=int(siz/num);
        if ((Tnum*num)!=siz)
            Tnum=Tnum+1;
    //cout<<siz<<"Tnum"<<Tnum <<endl;
    }
    int div = (int)(siz / Tnum);
    thread th[Tnum];
    int pos=Tnum-1;
    int *pos1=&pos;
    //if(siz<11)
    //cout<<"ID i= "<<i<<"siz"<<siz<<endl;
    for (int d = 0; d < Tnum; d++)
    {//cout<< div * d<<" to - "<<div * (d + 1)<<endl;
        th[d] = thread(AdJAdl1,pos1, ref(FCS), i,d, siz, ref(NLabel), ref(NLabel2), query_graph,eigenVq1,ref(DM),twohop,oMax,qsiz,oMax2,ret);
    }
    //cout<< div * (Tnum - 1)<<" to - "<<siz<<endl;
    for (int d = 0; d < Tnum; d++)
        th[d].join();
    
}

    return returnhere;
}