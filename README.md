# Copilating and Running #

To comile the libaray and compile and run test case please follow instructions:
a) make clean
b) make 
c) g++ -std=c++11 basic.c -L. -lrvm -o basic
d) ./basic
Alternatively you can also use submitted "run.sh" for compiling the library and all the testcases.


#Log File Responses:#

## How you use logfiles to accomplish persistency plus transaction semantics?##

We have implemented teh redo logs as mentioned in the LRVM paper. There are two types of logs in the system. Undo logs and Redo logs. Undo logs are in memory and redo logs are persisted on the disk. 

a) Whenever user mentions that he wants to modify a segment region by rvm_about_to_modify() method, we copy the element of segment to the undo logs.

b) Now user can do the changes to the that region of the segment.

c-1) If the user commits the transaction, we put all the changes to the redo log in the format (offset)(length of content)(content). -1 in the end is also put to show the transaction end. It makes sure if the commit fails in between. Transaction to any segment are not in between.

c-2) If the user aborts the transaction, all the regions from undo logs are copied back to in memory data sgements.

d) On each rvm_map call or explicit rvm_trancate call, we apply the redo logs to the actual disk data segments and remove the redo logs.


## What goes in them? How do the files get cleaned up, so that they do not expand indefinitely? ##

As mentioned in the above response, the logs foramt is mentioned in point (c-1). The undo logs are in memory only, so no need to clean them up. They gets removed on commit or abort tarnsaction, so there are no memory leaks.

The redo logs are truncated by explicit rvm_truncate call only. They also gets truncated on each call on rvm_map for a particular segment. The explicit rvm_truncate() call truncates all the segment's logs. There is no periodic truncation in the system currently. It can be added very easily by just adding a timer.  
