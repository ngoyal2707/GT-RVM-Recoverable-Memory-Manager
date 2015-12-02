#ifndef RVM_INTERNAL
#define RVM_INTERNAL

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <sys/stat.h>

using namespace std; 

typedef struct Segment{
	// Name of the segment
	string segmentName;
	// stores the content of segment
	string segmentItem;
	// pointer to the start of segment
	void * segmentStartPoint;
	// Whether the sgement is already in 
	// transaction
	bool inTransaction=false;
}Segment;

class RVMT{
public:
	// directory name, passed by API user
	string directoryName;
	// map for segment use, using segment name
	unordered_map<string, Segment *> segmentMapByName;
	// map for segment use, using start pointer
	unordered_map<void *, Segment *> segmentMapByStart;

	RVMT(const char *dirName){
		directoryName = dirName;
		int status = mkdir(dirName, 0755);
		if(status==-1){
			// cout<<"Error in directory creation\n";
			// strerror ("The following error occurred");
		}
	}
	~RVMT(){		
	}
	Segment * getSegmentByName(string segmentName);	
	Segment * getSegmentByStart(void * segmentStart);	
	Segment * generateSegment(string segmentName, int sizeToCreate);
	void reloadSegmentFromFile(Segment *segment, int sizeToCreate);
	void deleteSegment(Segment *segment);
	void readSegmentFromRedo(string fileName,string segmentName);
};

typedef RVMT * rvm_t;

class TransactionManager {
public:
    RVMT *rvm;
    // Map for undo logs, 
    // Key is segment base pointer and value is list of undo logs 
    unordered_map<void *, vector<pair<int, string>> * > undoLogsMap;
    void createUndoLogs(void* segbase);
    int saveUndoLogs(void* segbase, int offset, int size);
    void commitToRedoLogs();
    void copyBackUndoLogs();
};
typedef TransactionManager * trans_t;

#endif