#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <functional>

using namespace std;

const int DEF_MIN_GEN_PERIOD = 5 ;
const int DEF_MAX_GEN_PERIOD = 10 ;
const int DEF_SELLER_N = 3 ;
const int DEF_CLIENT_COUNT = 5 ;
const int DEF_MIN_PROCESSING_SECONDS = 3 ;
const int DEF_MAX_PROCESSING_SECONDS = 6 ;

struct SellerInfo {
    int min_processing_seconds ;
    int max_processing_seconds ;
    mutex mu ;
} ;

struct ClientGenerator {
    int min_gen_period ;
    int max_gen_period ;
} ;

struct ClientInfo {
    SellerInfo * sellers ;
    int seller_n ;
    int client_id ;
} ;

static atomic<int> clients_in_store ;
static atomic<int> sellers_in_work ;
static int tek_oper_n ;

static mutex mu_out ;

void locked_out(const function<void()> & outfunc) {
   mu_out.lock() ;
   cout<<tek_oper_n<<" | C="<<clients_in_store<<" | S="<<sellers_in_work<<" | " ;
   tek_oper_n++ ;
   outfunc() ;
   mu_out.unlock() ;
}

void thread_client(void * arg) {
    ClientInfo ci = *(ClientInfo*)arg ;
    clients_in_store++ ;
    locked_out([ci]() {
      cout << "Client "<<ci.client_id<<" enter to store"<<endl ;
    }) ;
    int visit_count = 1+rand() % (2*ci.seller_n-1) ;
    for (int i=0; i<visit_count; i++) {
        int current_seller = rand() % ci.seller_n ;
        if (ci.sellers[current_seller].mu.try_lock()) {
            sellers_in_work++ ;
            locked_out([ci,current_seller]() {
               cout << "Client "<<ci.client_id<<" begin work with seller "<<(current_seller+1)<<endl ;
            }) ;
        }
        else {			
            locked_out([ci,current_seller]() {
              cout << "Client "<<ci.client_id<<" waiting for seller "<<(current_seller+1)<<endl ;
            }) ;
            ci.sellers[current_seller].mu.lock() ;
            sellers_in_work++ ;
            locked_out([ci,current_seller]() {
              cout << "Client "<<ci.client_id<<" begin work with seller "<<(current_seller+1)<<endl ;
            }) ;
        }

        this_thread::sleep_for(chrono::seconds(ci.sellers[current_seller].min_processing_seconds+
          rand()%(ci.sellers[current_seller].max_processing_seconds-ci.sellers[current_seller].min_processing_seconds))) ;

        sellers_in_work-- ;
        locked_out([ci,current_seller]() {
          cout << "Client "<<ci.client_id<<" end work with seller "<<(current_seller+1)<<endl ;
        }) ;
        ci.sellers[current_seller].mu.unlock() ;
    }

    clients_in_store-- ;
    locked_out([ci]() {
       cout << "Client "<<ci.client_id<<" exit from store"<<endl ;
    }) ;
}

int main(int argc, char ** argv)
{
    cout << "Emulation started" << endl;

    ClientGenerator cg = {DEF_MIN_GEN_PERIOD,DEF_MAX_GEN_PERIOD} ;
    if (argc>1) cg.min_gen_period = atoi(argv[1]) ;
    if (argc>2) cg.max_gen_period = atoi(argv[2]) ;

    int seller_n = DEF_SELLER_N ;
    int client_count = DEF_CLIENT_COUNT ;
    if (argc>3) seller_n=atoi(argv[3]) ;
    if (argc>4) client_count = atoi(argv[4]) ;

    SellerInfo * si = new SellerInfo[seller_n] ;

    for (int i=0; i<seller_n; i++) {
        si[i].min_processing_seconds = DEF_MIN_PROCESSING_SECONDS ;
        si[i].max_processing_seconds = DEF_MAX_PROCESSING_SECONDS ;
        if (argc>5+2*i) si[i].min_processing_seconds = atoi(argv[5+2*i]) ;
        if (argc>5+2*i+1) si[i].max_processing_seconds = atoi(argv[5+2*i+1]) ;
    }

    clients_in_store = 0 ;
    sellers_in_work = 0 ;
    tek_oper_n = 0 ;
    vector<thread*> threads ;
    vector<ClientInfo*> infos ;
    for (int i=0; i<client_count; i++) {
       ClientInfo * ci = new ClientInfo() ;
       ci->client_id = i+1 ;
       ci->seller_n = seller_n ;
       ci->sellers = si ;
       thread * t = new thread(thread_client,ci) ;
       threads.push_back(t) ;
       infos.push_back(ci) ;
       this_thread::sleep_for(chrono::seconds(cg.min_gen_period+rand()%(cg.max_gen_period-cg.min_gen_period))) ;
    }

    for (int i=0; i<threads.size(); i++)
      threads[i]->join() ;

    for (int i=0; i<threads.size(); i++)
      delete threads[i] ;
    for (int i=0; i<infos.size(); i++)
      delete infos[i] ;

    delete[] si ;

    return 0;
}
