//
//  seqTracks.h
//Copyright (c) 2007-2012 Paul C Lott 
//University of California, Davis
//Genome and Biomedical Sciences Facility
//UC Davis Genome Center
//Ian Korf Lab
//Website: www.korflab.ucdavis.edu
//Email: lottpaul@gmail.com
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

#ifndef SEQTRACK_H
#define SEQTRACK_H

#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <queue>
#include "text.h"
#include "traceback_path.h"
#include "hmm.h"
#include "sequences.h"
#include <stdlib.h>

namespace StochHMM{
    
        
#ifdef THREADS
  extern pthread_cond_t exit_thread_flag_cv;
  extern pthread_mutex_t exit_thread_flag_mutex;
#endif 
    
  //!\file seqTracks.h
  //! Contains functions to import FASTA/FASTQ sequences from files and select the applicable model to deal with that sequence.
  //! It was set up to generate a seqJob for each sequence, then select a model, and then allow the programmer to thread the decoding algorithm.
    
  //!\enum SeqFileFormat
  //!File format of the sequences
  //!Currently only FASTA and FASTQ formats are supported
  enum SeqFileFormat {FASTA, FASTQ};
    
  //!\enum SeqFilesType 
  //!Sequence files have single track or multiple track sequences per file
  //!SINGLE means that only a single sequence is required (using single track)
  //!MULTI means that multiple sequences are required from multiple tracks
  enum SeqFilesType {SINGLE_TRACK, MULTI_TRACK};
    
  class seqJob;

  //!\struct ppTrack
  //!Stores what track is determined by a Track Function
  //!trackNumber is the index reference the derived track
  //!trackToUse is the track to use to generate the derived track
  //!Example: Function would be SIDD, the track we would use to get the sidd track
  //!would be a DNA sequence track.
  struct ppTrack{
    size_t trackNumber;
    size_t trackToUse;
    StochHMM::pt2TrackFunc* func;
  };

    
  /*! \class SeqTracks
    \brief SeqTracks are used to integrate the information provided by the model with the sequences that are being imported
  */
  class seqTracks{
  public:
        
    seqTracks();
    seqTracks(model&, std::string&, SeqFileFormat, TrackFuncs* trFuncs=0);  //Single Model, Single Seq File
    ~seqTracks();
        
    // mutators
    void loadSeqs(model&, std::string&, SeqFileFormat, TrackFuncs* trFuncs=0); // <-Main Function

    //!Sets the function to evaluate the which model to use with a particular sequence
    inline void setAttribFunc(pt2Attrib* func){attribModelFunc=func;}
		
    //!Sets a trackfunction that will be evaluated to generate a necessary track for the model
    inline void setTrackFunc(TrackFuncs* func){trackFunctions=func;}
    inline void setNumImportJobs(size_t value){numImportJobs=value;}
        
    void setTrackFilename(std::string&,std::string&);

    //ACCESSORS
    bool getNext();
    size_t getNJobs() { return n_jobs; };
    seqJob* getJob();

    sequence* getFasta(int);
    sequence* getFastq(int);
    sequence* getReal(int);
        
    size_t size(void){return jobQueue.size();}
    inline size_t getTrackCount(){return trackCount;}
        
    void print();
    void getInformation();
    float remainingSeqs(){
      float val = (float) seqFilenames.size() / (float) importTracks.size();
      //std::cout << seqFilenames.size() <<"\t" <<  importTracks.size() <<"\t" <<  val << std::endl;
      return val;}
        
  private:
        
    std::vector<std::ifstream*> filehandles; //input file stream
    std::vector<std::string> seqFilenames; //input filenames
    size_t numImportJobs;  // not sure what this is for -- it is set to 1 in the constructor and then not modified
    bool good;  // set to false if one of our ifstreams has an error bit set
        
    TrackFuncs* trackFunctions;
    pt2Attrib* attribModelFunc;
        
        
    int TrackToUseForAttrib;
        
    stateInfo* info;
        
    std::vector<std::pair<int,trackType> > importTracks;  // list of the tracks we're importing from the model
    std::vector<ppTrack> postprocessTracks;
        
        
    models* hmms; //Models
    model* hmm; //Models
    tracks* modelTracks; //Tracks defined by models (NOTE may include tracks which this seqTracks doesn't use)
        
    SeqFileFormat seqFormat;  //File format (Fasta or FastQ);
    SeqFilesType fileType;    //File Type (Single File or Multiple Files);
        
        
    //bool fastq;  
    //bool multiFile;
        
    size_t trackCount;
    stringList input;
        
    //Threading Variables
    std::queue<seqJob*> jobQueue; //used to be trcks
    size_t n_jobs;  //Counts of # of jobs waiting. Decremented in getJob(), incremented in getNext(), but otherwise not modified.
    int exit_thread;  //set to 0 if file stream is EOF
        
    //External Definition import function for Sequence
    ExDefSequence* getExDef(int,int);

    bool _loadFASTA(std::string&, SeqFilesType);
    bool _loadFASTA(std::vector<std::string>&, SeqFilesType);
        
    bool _loadFASTQ(std::string&, SeqFilesType);
    bool _loadFASTQ(std::vector<std::string>&, SeqFilesType);
        
        
    bool _initImportTrackInfo(void);
    void _reset();
    void _init();
    bool _open();
    bool _close();
  };
    
    
    

    
        
  //!\class seqJob
  //!Stores the model and sequence information for each job
  class seqJob{   //Could make a derivative of sequences
  public:
    //Constructor
    seqJob(size_t);  // NOTE this initializes the sequence vector to size sz, so *don't* use this if you're later going to push back the sequences
        
    //Destructor
    ~seqJob();
        
    friend class seqTracks;
        
        
    //MUTATORS
    void evaluateFunctions();
        
        
    //ACCESSORS
    inline size_t size(){return set->getLength();};
        
    inline model* getModel(){return hmm;};
    inline sequences* getSeqs(){return set;};
    inline sequences getSubSeqs(size_t pos, size_t len) { return set->getSubSequences(pos,len); }  // return a *new* set of (sub)sequences from <pos> of size <len>
        
    inline std::string getHeader(){return set->getHeader();};
        
    inline bool evaluated(){return funcEvaluated;};
        
        
    inline void printModel(){if(hmm) hmm->print();};
        
    inline void printSeq(){set->print();};
        
    inline traceback_path* getPath(){if (decodingPerformed) return path;else return NULL;};
        
    double getSeqAttrib(){return attrib;};
        
    inline std::string getSeqFilename(size_t iter){return seqFilename[iter];};
    inline void setSeqFilename(std::string& filename){seqFilename.push_back(filename); return;};
    inline void printFilenames(){for(size_t i=0;i<seqFilename.size();i++){ std::cout << seqFilename[i] << std::endl;}};
        
  private:
    model* hmm;
    sequences* set;
        
    std::vector< std::string>  seqFilename;
        
    double attrib;
    bool funcEvaluated;
        
    TrackFuncs* functions;
        
    bool decodingPerformed;
    traceback_path* path;
        
  };


}
#endif /*SEQTRACK_H*/
