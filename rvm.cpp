#include "rvm.h"
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <dirent.h>
#define __DEBUG__ 0
#define LOG(msg) cout<<__LINE__<<": "<<msg<<"\n\n"
using namespace std;

/*
Following functions are for RVMT class
*/
Segment * RVMT::getSegmentByName(string segmentName){
	ostringstream out;
	if(__DEBUG__){
		out<<"Inside RVMT::getSegmentByName() with argument: "<<segmentName;
		LOG(out.str());
		out.str("");
	}
	if(segmentMapByName.find(segmentName)!=segmentMapByName.end()){
		if(__DEBUG__){
			out<<"Inside RVMT::getSegmentByName(), segment found";
			LOG(out.str());
			out.str("");
		}
		return segmentMapByName[segmentName];
	}
	if(__DEBUG__){
		out<<"Inside RVMT::getSegmentByName(), segment not found";
		LOG(out.str());
		out.str("");
	}
	return NULL;
}

Segment * RVMT::getSegmentByStart(void * segmentStart){
	if(segmentMapByStart.find(segmentStart)!=segmentMapByStart.end()){
		return segmentMapByStart[segmentStart];
	}
	return NULL;
}

Segment * RVMT::generateSegment(string segmentName, int sizeToCreate){
	ostringstream out;
	if(__DEBUG__){
		out<<"Inside RVMT::generateSegment()";
		LOG(out.str());
		out.str("");
	}
	Segment * newSegment = new Segment();
	newSegment->segmentName = segmentName;
	(newSegment->segmentItem).resize(sizeToCreate);
	newSegment->segmentStartPoint=(void *)&(newSegment->segmentItem[0]);
	segmentMapByName[segmentName] = newSegment;
	segmentMapByStart[newSegment->segmentStartPoint] = newSegment;
	if(__DEBUG__){
		out<<"Successfully called RVMT::generateSegment()";
		LOG(out.str());
		out.str("");
	}
	return newSegment;
}

void RVMT::deleteSegment(Segment *segment){
	segmentMapByName.erase(segment->segmentName);
	segmentMapByStart.erase(segment->segmentStartPoint);
	delete(segment);
}

void RVMT::reloadSegmentFromFile(Segment *segment, int sizeToCreate){
	ostringstream out;
	if(__DEBUG__){
		out<<"Inside RVMT::reloadSegmentFromFile()";
		LOG(out.str());
		out.str("");
	}
	string segmentFileName = directoryName +'/'+ segment->segmentName;

	fstream segmentFile(segmentFileName,ios::binary |ios::in | ios::out|ios::ate);
	if(!segmentFile){
		segmentFile.open(segmentFileName,ios::binary | ios::out);
	}
    if (segmentFile.is_open()) { 
    	if(__DEBUG__){
			out<<"Inside RVMT::reloadSegmentFromFile(), opened file Successfully";
			LOG(out.str());
			out.str("");
		}	
		segmentFile.seekg (0, segmentFile.end); 
		int fileLength = segmentFile.tellg();
		    
		// If given segment legth is more, 
		// adjust the file size
		if (sizeToCreate > fileLength){
		    segmentFile.seekp(sizeToCreate-1);
		    char nullChar = '\0';
		    segmentFile.write(&nullChar, 1);
		}
		segmentFile.seekg (0, segmentFile.beg);
		    
		segmentFile.read(&(segment->segmentItem[0]), sizeToCreate);
		segmentFile.close();
    }
    else{
		fprintf(stderr, "Error in opening file %s \n", segment->segmentName.c_str());
		// perror("*********ERROR************");
		return;
    }
    if(__DEBUG__){
		out<<"Successfully done RVMT::reloadSegmentFromFile()";
		LOG(out.str());
		out.str("");
	}
}

/*
Save changes from Redo logfile to backed up segment  
*/
void RVMT::readSegmentFromRedo(string logFileName,string segmentName){
	ifstream logFile;
	logFile.open(directoryName+'/'+logFileName, ios::binary|ios::in);
	logFile.seekg(0,ios::beg);

	fstream segmentFile;
	segmentFile.open(directoryName+'/'+segmentName, ios::binary|ios::out|ios::in);
	segmentFile.seekg (0, segmentFile.end); 
	int fileLength = segmentFile.tellg();
	segmentFile.seekg (0, segmentFile.beg);
	char * segmentContentChar = new char[fileLength+2];    
	segmentFile.read(segmentContentChar, fileLength);

	// string segmentContent(segmentContentChar);


	while(logFile!=NULL){
		// char *bufferOffset = new char[5];
		// char *bufferLength = new char[5];
		int offset;
		int length;
		logFile.read((char *)&offset,sizeof(offset));
		if(!logFile) break;

		// Done transaction
		if(offset==-1){
			segmentFile.seekp(0);
			segmentFile.write(segmentContentChar, fileLength);
			continue;
		}

		logFile.read((char *)&length,sizeof(length));
		if(!logFile) break;

		char *bufferContent = new char[length+2];
		logFile.read(bufferContent, length);
		string content(bufferContent);

		memcpy(segmentContentChar+offset, bufferContent, length);
		// segmentFile.seekp(offset);
		// segmentFile.write(content.c_str(), content.length());
	}
	logFile.close();
	segmentFile.close();
}
/*
===========================================================================================================
===========================================================================================================
===========================================================================================================
===========================================================================================================
===========================================================================================================
===========================================================================================================
*/

/*
	Following functions are for TransactionManager class
*/
void TransactionManager:: createUndoLogs(void *segbase){
	// create blank entry in undo logs
	// Will fill this in about_to_modify
	undoLogsMap[segbase]= new vector<pair<int, string>> ();
}

int TransactionManager:: saveUndoLogs(void *segbase, int offset, int size){
	// create blank entry in undo logs
	// Will fill this in about_to_modify
	ostringstream out;
	if(__DEBUG__){
		out<<"Inside TransactionManager::saveUndoLogs()";
		LOG(out.str());
		out.str("");
	}
	if(undoLogsMap.find(segbase)== undoLogsMap.end()){
		fprintf(stderr, "The segment was not passed in begin_trans call\n" );
		return -1;
	}
	string updatedValues = string((char*)segbase+ offset, size);
	undoLogsMap[segbase]->push_back(make_pair(offset,updatedValues ));
	if(__DEBUG__){
		out<<"Successfully done TransactionManager::saveUndoLogs()";
		LOG(out.str());
		out.str("");
	}
	return 0;
}

void TransactionManager:: commitToRedoLogs(){
	// We can use the undo map itself to 
	// put entries into redo logs
	ostringstream out;
	if(__DEBUG__){
		LOG("Inside TransactionManager::commitToRedoLogs()");
	}
	unordered_map<string, int> segmentsInTrans;
	for(auto it = undoLogsMap.begin(); it != undoLogsMap.end(); it++){
		void *segbasePointer = it->first;
		std::vector<pair<int, string>> *logList=it->second;

		Segment *segment = rvm->getSegmentByStart(segbasePointer);
		if(segment==NULL){
			LOG("Some mistake, segment not found");
		}
		if(__DEBUG__){
			out<<"Applying transaction to segment: "<<segment->segmentName;
			LOG(out.str());
			out.str("");
		}
		// fstream segmentFile;
		// segmentFile.open((rvm->directoryName +'/' + segment->segmentName).c_str(), ios::binary|ios::in|ios::out);

		string logFileName = rvm->directoryName+"/"+segment->segmentName+".log";
		std::ofstream logFile;
		logFile.open(logFileName, std::ofstream::binary | std::ofstream::out | std::ofstream::app);
		
		if(logFile==NULL){
			fprintf(stderr, "File could not be opened\n");
			return;
		}

		for(auto it = logList->begin();it!=logList->end();it++){
			int offset = it->first;
			int contentLength = it->second.size();
			logFile.write((char *)&offset, sizeof(int));
			logFile.write((char *)&contentLength, sizeof(int));
			logFile.write((char *)(segbasePointer+offset), contentLength);

			// segmentFile.seekp(offset);
			// segmentFile.write((char *)(segbasePointer+offset),it->second.size());
		}
		segmentsInTrans[segment->segmentName] = 1;

		delete(logList);

		segment->inTransaction = false;
		logFile.close();
	} 
	/*
	Putting DONE msg at the end of transaction 
	*/
	for(auto it=segmentsInTrans.begin();it!=segmentsInTrans.end();it++){
		string logFileName = rvm->directoryName+"/"+it->first+".log";
		std::ofstream logFile;
		logFile.open(logFileName, std::ofstream::binary | std::ofstream::out | std::ofstream::app);
		if(logFile==NULL){
			fprintf(stderr, "File could not be opened\n");
			return;
		}
		int DONE=-1;
		logFile.write((char *)&DONE, sizeof(int));
		logFile.close();
	}

	undoLogsMap.clear();
}

void TransactionManager:: copyBackUndoLogs(){
	// We can use the undo map itself to 
	// put entries into redo logs
	ostringstream out;
	if(__DEBUG__){
		LOG("Inside TransactionManager::copyBackUndoLogs()");
	}
	for(auto it = undoLogsMap.begin(); it != undoLogsMap.end(); it++){
		void *segbasePointer = it->first;
		std::vector<pair<int, string>> *logList=it->second;

		Segment *segment = rvm->getSegmentByStart(segbasePointer);
		if(segment==NULL){
			LOG("Some mistake, segment not found");
		}
		if(__DEBUG__){
			out<<"Aborting transaction to segment: "<<segment->segmentName;
			LOG(out.str());
			out.str("");
		}

		for(auto it = logList->begin();it!=logList->end();it++){
			int offset = it->first;
			size_t lenToCopy = it->second.size();
			memcpy((void *)((char *)segbasePointer+offset), (char *)(it->second.c_str()),lenToCopy); 
		}

		segment->inTransaction = false;
	} 
	undoLogsMap.clear();
}


/*
===========================================================================================================
===========================================================================================================
===========================================================================================================
===========================================================================================================
===========================================================================================================
===========================================================================================================
*/

/*
This is a utility function that gets called by 
truncate logs and rvm_map both  
*/
void redoLogsToDataSegment(rvm_t rvm, string fileName,string segmentName ){
	rvm->readSegmentFromRedo(fileName, segmentName );

	ofstream ofs;
	ofs.open(rvm->directoryName+'/'+fileName, std::ofstream::out | std::ofstream::trunc);
	ofs.close();
}

/*
Definition of all the library functions
*/
rvm_t rvm_init(const char *directory){
	ostringstream out;
	if(__DEBUG__){
		out<<"Inside rvm_init with argument "<<string(directory);
		LOG(out.str());
		out.str("");
	}
	rvm_t rvm = new RVMT(directory);
	rvm_truncate_log(rvm);
	return rvm; 
}

void *rvm_map(rvm_t rvm, const char *segname, int sizeToCreate){
	ostringstream out;
	if(__DEBUG__){
		out<<"Inside rvm_map with argument "<<string(segname);
		LOG(out.str());
		out.str("");
	}
	Segment * segment = rvm->getSegmentByName(segname);
	if(segment == NULL){
		segment = rvm->generateSegment(segname, sizeToCreate);
		string segmentRedoFileName = string(segname) + ".log";
		redoLogsToDataSegment(rvm, segmentRedoFileName, segname);

		rvm->reloadSegmentFromFile(segment,sizeToCreate);
		if(__DEBUG__){
			LOG("Successfully done rvm_map");
		}
		return segment->segmentStartPoint;
	}else{
		fprintf(stderr, "Segment has already been mapped \n");
		return NULL;	
	}

}

void rvm_unmap(rvm_t rvm, void *segbase){
	if(__DEBUG__){
		LOG("Inside rvm_unmap()");
	}
	Segment * segment = rvm->getSegmentByStart(segbase);
	if(segment != NULL){
		rvm->deleteSegment(segment);
	}else{
		fprintf(stderr, "Segment has not been mapped \n");
		return;
	}
}
void rvm_destroy(rvm_t rvm, const char *segname){
	if(__DEBUG__){
		LOG("Inside rvm_destroy()");
	}
	Segment *segment = rvm->getSegmentByName(segname);
	if(segment==NULL){
		string segmentFileName = rvm->directoryName + "/" + segname;
		unlink(segmentFileName.c_str());
		string redoLogFileName = rvm->directoryName + "/" + segname + ".log";
		unlink(redoLogFileName.c_str());
	}else{
		fprintf(stderr, "Segment is mapped, can't be destroyed\n" );
	}
	if(__DEBUG__){
		LOG("Successfully done rvm_destroy()");
	}
}

trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases){
	if(__DEBUG__){
		LOG("Inside rvm_begin_trans()");
	}
	trans_t trans = new TransactionManager();
	if(numsegs<=0){
		fprintf(stderr, "Number of segments must be nonzero positive\n" );
		return (trans_t)-1;
	}
	if(rvm == NULL){
		fprintf(stderr, "passed rvm argument is NULL\n");
		return (trans_t)-1;
	}
	for(int i=0;i<numsegs;i++){
		Segment *segment = rvm->getSegmentByStart(segbases[i]);
		if(segment==NULL){
			// Do we want to go with remaining names????
			fprintf(stderr, "segment has not been mapped\n"	);
			delete(trans);
			return (trans_t)-1;
		}else{
			// return ERROR even if one segment is already in transaction
			if(segment->inTransaction){
				fprintf(stderr, "segment %s is already in transaction\n", segment->segmentName.c_str());
				delete(trans);
				return (trans_t)-1;
			}else{
				segment->inTransaction = 1;
				trans->createUndoLogs(segbases[i]);
			}
		}
	} 
	trans->rvm = rvm;
	if(__DEBUG__){
		LOG("Successfully done rvm_begin_trans()");
	}
	return trans;
}

void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size){
	if(__DEBUG__){
		LOG("Inside rvm_about_to_modify()");
	}
	if(tid == NULL || tid == (trans_t)-1){
		fprintf(stderr, "Passed transaction is invalid\n");
		return;
	}
	rvm_t rvm = tid->rvm;
	if(rvm == NULL){
		fprintf(stderr, "RVM associated with transaction is invalid\n");
		return;
	}
	bool status = tid->saveUndoLogs(segbase, offset, size);
	if(__DEBUG__){
		LOG("Successfully done rvm_about_to_modify()");
	}
	// TODO: undo logs
}

void rvm_commit_trans(trans_t tid){
	if(__DEBUG__){
		LOG("Inside rvm_commit_trans()");
	}
	if(tid == NULL || tid == (trans_t)-1){
		fprintf(stderr, "Passed transaction is invalid\n");
		return;
	}
	tid->commitToRedoLogs();
	if(__DEBUG__){
		LOG("Successfully done rvm_commit_trans()");
	}
}

void rvm_abort_trans(trans_t tid){
	if(__DEBUG__){
		LOG("Inside rvm_abort_trans()");
	}
	if(tid == NULL || tid == (trans_t)-1){
		fprintf(stderr, "Passed transaction is invalid\n");
		return;
	}
	tid->copyBackUndoLogs();
	if(__DEBUG__){
		LOG("Successfully done rvm_abort_trans()");
	}
}
bool has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}


void rvm_truncate_log(rvm_t rvm){
	DIR *dir;
	struct dirent *directoryEntry;
	if((dir = opendir (rvm->directoryName.c_str())) != NULL){
		while((directoryEntry = readdir(dir)) != NULL) {
			string fileName = directoryEntry->d_name;
			unsigned int pos = fileName.rfind(".log");
			// Log file found
			if(has_suffix(fileName,".log")){
				string segmentName = fileName.substr(0,pos);
				redoLogsToDataSegment(rvm, fileName, segmentName);
			}
		}
	  	closedir(dir);
	}else {
		fprintf(stderr, "Could not open log directory for truncation\n" );
		return;
	}
}