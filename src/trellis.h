//
//  trellis.h
//  StochHMM
//
//Permission is hereby granted, free of charge, to any person obtaining a copy of
//this software and associated documentation files (the "Software"), to deal in
//the Software without restriction, including without limitation the rights to
//use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
//the Software, and to permit persons to whom the Software is furnished to do so,
//subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
//FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
//COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
//IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef __StochHMM__new_trellis__
#define __StochHMM__new_trellis__

#include <iostream>
#include <vector>
#include <stdint.h>
#include <vector>
#include <string>
#include <fstream>
#include <bitset>
#include <map>
#include "stochTypes.h"
#include "sequences.h"
#include "hmm.h"
#include "traceback_path.h"
#include "stochMath.h"
#include <stdint.h>
#include <iomanip>
#include "stochTable.h"
#include "sparseArray.h"

namespace StochHMM{
        
        
        
        
        
  typedef std::vector<std::vector<int16_t> > int_2D;
  typedef std::vector<std::vector<float> > float_2D;
  typedef std::vector<std::vector<double> > double_2D;
  typedef std::vector<std::vector<long double> > long_double_2D;

  typedef std::vector<std::vector<std::vector<uint16_t> > > int_3D;
  typedef std::vector<std::vector<std::vector<float> > > float_3D;
  typedef std::vector<std::vector<std::vector<double> > > double_3D;
  typedef std::vector<std::vector<std::vector<long double> > > long_double_3D;
  //      typedef std::vector<std::vector<std::vector<std::pair<int16_t,int16_t> >
        
  class nthScore{
  public:
    int16_t st_tb;
    int16_t score_tb;
    double score;
    nthScore():st_tb(0),score_tb(0),score(-INFINITY){};
    nthScore(int16_t st, int16_t tb, double sc):st_tb(st),score_tb(tb),score(sc){};
                
  };
        
  //      struct nthTrace{
  //              int16_t st_tb;
  //              int16_t score_tb;
  //              nthTrace():st_tb(0),score_tb(0){};
  //              nthTrace(int16_t st, int16_t tb):st_tb(st),score_tb(tb){};
  //      };
        
  class nthTrace{
  public:
    std::map<int32_t,int32_t> tb;
                
    inline void assign(int16_t st, int16_t n_score, int16_t tb_ptr, int16_t tb_sc){
      tb[(((int32_t) st) << 16 | n_score)] = (((int32_t)tb_ptr) << 16) | tb_sc;
    }
                
    inline void get(int16_t& st, int16_t& n_score){
      int32_t key_val = (((int32_t) st) << 16 | n_score);
      if (tb.count(key_val)){
	st = -1;
      }
      int32_t val = tb[key_val];
      st      = (val >> 16) & 0xFFFF;
      n_score = val & 0xFFFF;
    }
  };

        
  /*! \class Trellis
   *      \brief Implements the HMM scoring trellis and algorithms
   *
   *
   *
   */
  class trellis{
  public:
    double  ending_viterbi_score;
    trellis();
    trellis(model* h , sequences* sqs);
    ~trellis();
    void reset();
    inline model* getModel(){return hmm;}
    inline sequences* getSeq(){return seqs;}
                
    // ----------------------------------------------------------------------------------------
    // decoding
    void viterbi();
    void viterbi(model* h, sequences* sqs);
    void forward(int iseq=-1);
    void forward(model* h, sequences* sqs);

    // ----------------------------------------------------------------------------------------
    // traceback
    void traceback(traceback_path& path);
    void traceback(traceback_path& path ,size_t position, size_t state);
                
    inline bool store(){return store_values;}
    inline void store(bool val){store_values=val; return;}

    void print();
    std::string stringify();
    void export_trellis(std::ifstream&);
    void export_trellis(std::string& file);
    inline model* get_model(){return hmm;}
                
    //Model Baum-Welch Updating
    void update_transitions();
    void update_emissions();
                
                
    /*----------- Accessors ---------------*/
    inline double_2D* get_naive_forward_scores(){return dbl_forward_score;}
    inline double_2D* get_naive_backward_scores(){return dbl_backward_score;}
    inline double_2D* get_naive_viterbi_scores(){return dbl_viterbi_score;}
    inline double_2D* get_dbl_posterior(){return dbl_posterior_score;}
                
                
    inline float_2D* getForwardTable(){return forward_score;}
    inline float_2D* getBackwardTable(){return backward_score;}
    inline double_2D* getPosteriorTable(){return posterior_score;}
                
    inline double getForwardProbability(){return ending_forward_prob;}
    inline double getBackwardProbability(){return ending_backward_prob;}

                
  private:
    double getEndingTransition(size_t);
    double getTransition(state* st, size_t trans_to_state);
    size_t get_explicit_duration_length(transition* trans, size_t sequencePosition,size_t state_iter, size_t to_state);
    double transitionFuncTraceback(state* st, size_t position, transitionFuncParam* func);
                
                
    model* hmm;             //HMM model
    sequences* seqs; //Digitized Sequences
    size_t nth_size; //Size of N to calculate;
                
                
    size_t state_size;      //Number of States
    size_t seq_size;        //Length of Sequence
                
    trellisType type;
                
    bool store_values;
    bool exDef_defined;
                
    //Traceback Tables
    int_2D*         traceback_table;        //Simple traceback table
    //              int_3D*         nth_traceback_table;//Nth-Viterbi traceback table
    stochTable* stochastic_table;
    //              alt_stochTable* alt_stochastic_table;
    alt_simple_stochTable* alt_simple_stochastic_table;
                
    //Score Tables
    float_2D*       viterbi_score;      //Storing Viterbi scores
    float_2D*       forward_score;      //Storing Forward scores
    float_2D*       backward_score;     //Storing Backward scores
    double_2D*      posterior_score;        //Storing Posterior scores
                
    double_2D*  dbl_forward_score;
    double_2D*      dbl_viterbi_score;
    double_2D*  dbl_backward_score;
    double_2D*      dbl_posterior_score;
    double_3D*  dbl_baum_welch_score;
    std::vector<std::vector<std::vector<nthScore >* > >* naive_nth_scores;
                
                
    std::vector<nthTrace>*  nth_traceback_table;
                
    //Ending Cells
    int16_t ending_viterbi_tb;
    double  ending_forward_prob;
    double  ending_backward_prob;
    std::vector<nthScore>* ending_nth_viterbi;

    //              std::vector<tb_score>*    ending_stoch_tb;
    //              std::vector<tb_score>*    ending_nth_viterbi;
                
    //Cells used for scoring each cell
    std::vector<double>* scoring_current;
    std::vector<double>* scoring_previous;
    std::vector<double>* alt_scoring_current;
    std::vector<double>* alt_scoring_previous;
                
    std::vector<size_t>* explicit_duration_current;
    std::vector<size_t>* explicit_duration_previous;
    std::vector<size_t>* swap_ptr_duration;
                
    std::vector<double>* swap_ptr;
                
    std::vector<std::vector<double>* > complex_emissions;
    std::vector<std::vector<std::map<uint16_t,double>* >* >* complex_transitions;
                
    std::vector<std::vector<nthScore> >* nth_scoring_current;
    std::vector<std::vector<nthScore> >* nth_scoring_previous;
    std::vector<std::vector<nthScore> >* nth_swap_ptr;
  };
        
  void sort_scores(std::vector<nthScore>& nth_scores);
  bool _vec_sort(const nthScore& i, const nthScore& j);
        
}

#endif /* defined(__StochHMM__new_trellis__) */
