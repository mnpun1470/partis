#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <iomanip>
#include <time.h>
#include <fstream>
#include "StochHMMlib.h"
#include "job_holder.h"
#include "germlines.h"

#include "StochHMM_usage.h"
using namespace StochHMM;
using namespace std;
// NOTE use this to compile: export CPLUS_INCLUDE_PATH=/home/dralph/Dropbox/include/eigen-eigen-6b38706d90a9; make
#define STATE_MAX 1024  // TODO reduce the state max to something reasonable?
void StreamOutput(ofstream &ofs, options &opt, vector<RecoEvent> &events, sequences &seqs, double total_score);

// ----------------------------------------------------------------------------------------
// init command-line options
opt_parameters commandline[] = {
  {"--hmmtype"      ,OPT_STRING     ,false  ,"single",  {}},
  {"--hmmdir"       ,OPT_STRING     ,false  ,"./bcell", {}},
  {"--infile"       ,OPT_STRING     ,true   ,"",        {}},
  {"--outfile"      ,OPT_STRING     ,true   ,"",        {}},
  {"--algorithm"    ,OPT_STRING     ,true   ,"viterbi", {}},
  {"--debug"        ,OPT_INT        ,false  ,"0",       {}},
  {"--n_best_events",OPT_INT        ,false  ,"5",       {}},
};

int opt_size=sizeof(commandline)/sizeof(commandline[0]);  //Stores the number of options in opt
options opt;  //Global options for parsed command-line options

// ----------------------------------------------------------------------------------------
// class for reading csv input info
class Args {
public:
  Args(string fname);
  map<string, vector<string> > strings_;
  map<string, vector<int> > integers_;
  set<string> str_headers_, int_headers_;
};
// ----------------------------------------------------------------------------------------
Args::Args(string fname):
  str_headers_{"only_genes", "name", "seq", "second_name", "second_seq"},
  int_headers_{"k_v_guess", "k_d_guess", "v_fuzz", "d_fuzz"}
{
  for (auto &head: str_headers_)
    strings_[head] = vector<string>();
  for (auto &head: int_headers_)
    integers_[head] = vector<int>();
  
  ifstream ifs(fname);
  assert(ifs.is_open());
  string line;
  // get header line
  getline(ifs,line);
  stringstream ss(line);
  vector<string> headers;  // keep track of the file's column order
  while (!ss.eof()) {
    string head;
    ss >> head;
    headers.push_back(head);
  }
  while (getline(ifs,line)) {
    stringstream ss(line);
    string tmpstr;
    int tmpint;
    for (auto &head: headers) {
      if (str_headers_.find(head) != str_headers_.end()) {
	ss >> tmpstr;
	strings_[head].push_back(tmpstr);
      } else if (int_headers_.find(head) != int_headers_.end()) {
	ss >> tmpint;
	integers_[head].push_back(tmpint);
      } else {
	assert(0);
      }
    }
  }
}
// ----------------------------------------------------------------------------------------
vector<sequences*> GetSeqs(Args &args, track *trk) {
  vector<sequences*> all_seqs;
  for (size_t iseq=0; iseq<args.strings_["seq"].size(); ++iseq) {
    sequences *seqs = new sequences;
    sequence *sq = new(nothrow) sequence(args.strings_["seq"][iseq], trk, args.strings_["name"][iseq]);
    assert(sq);
    seqs->addSeq(sq);
    if (trk->n_seqs == 2) {
      assert(args.strings_["second_seq"][iseq].size() > 0);
      assert(args.strings_["second_seq"][iseq] != "x");
      sequence *second_sq = new(nothrow) sequence(args.strings_["second_seq"][iseq], trk, args.strings_["second_name"][iseq]);
      assert(second_sq);
      seqs->addSeq(second_sq);
    } else {
      assert(args.strings_["second_seq"][iseq] == "x");  // er, not really necessary, I suppose...
    }

    all_seqs.push_back(seqs);
  }
  return all_seqs;
}
// ----------------------------------------------------------------------------------------
int main(int argc, const char * argv[]) {
  srand(time(NULL));
  opt.set_parameters(commandline, opt_size, "");
  opt.parse_commandline(argc,argv);
  assert(opt.sopt("--hmmtype") == "single" || opt.sopt("--hmmtype") == "pair");
  assert(opt.sopt("--algorithm") == "viterbi" || opt.sopt("--algorithm") == "forward");
  assert(opt.iopt("--debug") >=0 && opt.iopt("--debug") < 3);
  Args args(opt.sopt("--infile"));

  // write csv output header
  ofstream ofs(opt.sopt("--outfile"));
  assert(ofs.is_open());
  if (opt.sopt("--algorithm") == "viterbi")
    ofs << "unique_id,second_unique_id,v_gene,d_gene,j_gene,vd_insertion,dj_insertion,v_3p_del,d_5p_del,d_3p_del,j_5p_del,score,seq,second_seq" << endl;
  else
    ofs << "unique_id,second_unique_id,score" << endl;

  // init some stochhmm infrastructure
  size_t n_seqs_per_track(opt.sopt("--hmmtype")=="pair" ? 2 : 1);
  vector<string> characters{"A","C","G","T"};
  track trk("NUKES", n_seqs_per_track, characters);
  vector<sequences*> seqs(GetSeqs(args, &trk));
  GermLines gl;
  HMMHolder hmms(opt.sopt("--hmmdir"), n_seqs_per_track, &gl);

  assert(seqs.size() == args.strings_["name"].size());
  for (size_t is=0; is<seqs.size(); is++) {
    int k_v_guess = args.integers_["k_v_guess"][is];
    int k_d_guess = args.integers_["k_d_guess"][is];
    int v_fuzz = args.integers_["v_fuzz"][is];
    int d_fuzz = args.integers_["d_fuzz"][is];
    // TODO oh wait shouldn't k_d be allowed to be zero?
    // NOTE this is the *maximum* fuzz allowed -- job_holder::Run is allowed to skip ksets that don't make sense

    JobHolder jh(&gl, opt.sopt("--algorithm"), seqs[is], &hmms, opt.iopt("--n_best_events"), args.strings_["only_genes"][is]);
    jh.SetDebug(opt.iopt("--debug"));
    jh.Run(max(k_v_guess - v_fuzz, 1), 2*v_fuzz, max(k_d_guess - d_fuzz, 1), 2*d_fuzz);  // note that these events will die when the JobHolder dies

    sequences seqs_a;
    // sequence *sq = new(nothrow) sequence(args.strings_["seq"][iseq], trk, args.strings_["name"][iseq]);
    seqs_a.addSeq(&(*seqs[is])[0]);
    JobHolder jh_a(&gl, opt.sopt("--algorithm"), &seqs_a, &hmms, opt.iopt("--n_best_events"), args.strings_["only_genes"][is]);
    jh_a.SetDebug(opt.iopt("--debug"));
    jh_a.Run(max(k_v_guess - v_fuzz, 1), 2*v_fuzz, max(k_d_guess - d_fuzz, 1), 2*d_fuzz);  // note that these events will die when the JobHolder dies

    sequences seqs_b;
    // sequence *sq = new(nothrow) sequence(args.strings_["seq"][iseq], trk, args.strings_["name"][iseq]);
    seqs_b.addSeq(&(*seqs[is])[1]);
    JobHolder jh_b(&gl, opt.sopt("--algorithm"), &seqs_b, &hmms, opt.iopt("--n_best_events"), args.strings_["only_genes"][is]);
    jh_b.SetDebug(opt.iopt("--debug"));
    jh_b.Run(max(k_v_guess - v_fuzz, 1), 2*v_fuzz, max(k_d_guess - d_fuzz, 1), 2*d_fuzz);  // note that these events will die when the JobHolder dies

    // cout
    //   << "FOOP"
    //   << setw(12) << jh.total_score()
    //   << setw(12) << jh_a.total_score()
    //   << setw(12) << jh_b.total_score()
    //   << " --> "
    //   << setw(12) << jh.total_score() - jh_a.total_score() - jh_b.total_score()
    //   << endl;

    StreamOutput(ofs, opt, jh.events(), *seqs[is], jh.total_score() - jh_a.total_score() - jh_b.total_score());
  }

  ofs.close();
  return 0;
}

// ----------------------------------------------------------------------------------------
void StreamOutput(ofstream &ofs, options &opt, vector<RecoEvent> &events, sequences &seqs, double total_score) {
  if (opt.sopt("--algorithm") == "viterbi") {
    for (size_t ievt=0; ievt<opt.iopt("--n_best_events"); ++ievt) {
      RecoEvent *event = &events[ievt];
      string second_seq_name,second_seq;
      if (opt.sopt("--hmmtype") == "pair") {
	second_seq_name = event->second_seq_name_;
	second_seq = event->second_seq_;
      }
      ofs
	<< event->seq_name_
	<< "," << second_seq_name
	<< "," << event->genes_["v"]
	<< "," << event->genes_["d"]
	<< "," << event->genes_["j"]
	<< "," << event->insertions_["vd"]
	<< "," << event->insertions_["dj"]
	<< "," << event->deletions_["v_3p"]
	<< "," << event->deletions_["d_5p"]
	<< "," << event->deletions_["d_3p"]
	<< "," << event->deletions_["j_5p"]
	<< "," << event->score_
	<< "," << event->seq_
	<< "," << second_seq
	<< endl;
    }
  } else {
    assert(seqs.size() == 2);  // er, at least for the moment
    ofs
      << seqs[0].name_
      << "," << seqs[1].name_
      << "," << total_score << endl;
  }
}
