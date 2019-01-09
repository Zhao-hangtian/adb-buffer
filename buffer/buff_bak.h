// 定义结构体与函数

#include "stdio.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>

#define RECORDSIZE 316
#define FRAMESIZE 4096
#define BUFFSIZE 1024 // frame数目
#define MAXSIZE  FRAMESIZE*BUFFSIZE

using namespace std;

struct bFrame
{
	char field[FRAMESIZE];
};

struct Record
{
	int page_id;
	int slot_num;
};

struct Frame
{
	int frame_id;
	int offset;
};

//class DiskSpaceManager {
//private:
//	FILE * fstream;
//
//public:
//	string filepath;
//	DiskSpaceManager(char filepath[]) :filepath(filepath) {
//		fstream = fopen(filepath, "wb"); // 不存在则创建
//	}
//	void readDisk(int page_id, int frame_id, bFrame buff[]){
//		bFrame bframe;
//		fseek(fstream, page_id, SEEK_SET);
//#if TEST2
//		if (fread(&bframe, sizeof(bframe), 1, fstream) == 1) {
//			cout << "读取 page_id:" << page_id << " 到 frame_id:" << frame_id << " 到缓存成功。";
//		}else{
//			cout << "读取 page_id:" << page_id << " 到 frame_id:" << frame_id << " 到缓存失败！";
//#else
//		int readCnt = fread(&bframe, sizeof(bframe), 1, fstream);
//#endif
//		buff[frame_id] = bframe;
//	}
//
//	void writeDisk(int page_id, int frame_id, bFrame buff[]) {
//		bFrame bframe = buff[frame_id];
//		fseek(fstream, page_id, SEEK_SET);
//#if TEST2
//		if (readCnt = fwrite(&bframe, sizeof(bframe), 1, fstream) == 1) {
//			cout << "写入 page_id:" << page_id << " 到 frame_id:" << frame_id << " 到磁盘成功。";
//		}else{
//			cout << "写入 page_id:" << page_id << " 到 frame_id:" << frame_id << " 到磁盘失败！";
//			}
//#else
//		int readCnt = fwrite(&bframe, sizeof(bframe), 1, fstream);
//#endif
//	}
//};

//双向链表+哈希表实现LRUCache
struct BCB
{
	int page_id, frame_id, ref, count, time, dirty;
	BCB * _prev;
	BCB * _next;
	BCB() : _next(NULL), _prev(NULL) {}
	BCB(int page_id, int frame_id) : page_id(page_id), frame_id(frame_id), count(0), time(0), dirty(0) { BCB(); }

	void disconnect() {
		if (_prev) _prev->_next = _next;
		if (_next) _next->_prev = _prev;
	}
	// next方向插入节点
	void Insert(BCB * node) {
		if (_next) {
			node->_prev = this; // this
			_next->_prev = node;
			node->_next = _next;
			_next = node;
		}
	}
};
// 储存frames和pages的元数据BCB
class LRUCache {
private:
	int size, capacity;
	unordered_map<int, BCB*> hashmap;
	stack<int> frame_ids;
	BCB * head, *tail;
	int f2p[BUFFSIZE];
	bFrame buff[BUFFSIZE];
	int IOnum;
	int hitNum;
public:
	LRUCache(int capacity, char filepath[]) : size(0), capacity(capacity) {
		head = new BCB; // 或(LinkNode)malloc(sizeof(LinkNode));
		tail = new BCB;
		head->_next = tail;
		tail->_prev = head;
		//DiskSpaceManager diskSpaceManager(filepath);
		for (int i = 0; i < capacity; i++)frame_ids.push(i);
		IOnum = hitNum = 0;
	}
	// 读出缓存
	int read(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			return -1;
		}
		BCB * node = hashmap[page_id];
		node->disconnect(); // 放至首部
		head->Insert(node);
		return node->frame_id;
	}
	// 写入缓存
	int put(int page_id, int frame_id = 0) {
		BCB * node;
		if (page_id == -1) {}; // 申请磁盘新page
		if (hashmap.find(page_id) == hashmap.end()) {
			if (size < capacity) {
				frame_id = frame_ids.top();
				frame_ids.pop();
				node = new BCB(page_id, frame_id);
				size++;
			}
			else
			{
				// 置换，脏数据写入外存
				node = tail->_prev;
				int i = 0;
				for (; node->ref > 1 || node->count > 1; node = node->_next)
				{
					node->ref = 0; i++; if (i == capacity) { cout << "警告：缓存溢出！进行强行置换"; };
				};
				frame_id = node->frame_id; // frame_id复用
				node->disconnect();
				hashmap.erase(node->page_id);
				node->page_id = page_id;
				node->frame_id = frame_id;
			}
			hashmap[page_id] = node;
			head->Insert(node); // 放至首部
		}
		else {
			node = hashmap[page_id];
			node->disconnect(); // 放至首部
			head->Insert(node);
			frame_id = node->frame_id;
		}
		return frame_id;
	}
	// 查看要换出的frame_id
	int selectVictim() {
		BCB * node;
		node = tail->_prev;
		int i = 0;
		for (; node->ref > 1 || node->count > 1; node = node->_next)
		{
			node->ref = 0; i++; if (i == capacity) { cout << "警告：缓存溢出！即将进行强行置换"; };
		};
		return node->frame_id;
	}
	// 从缓存删除
	void remove(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			cout << "错误！page_id: " << page_id << " 不存在缓存中！" << endl;
		}
		// 脏数据写入外存
		BCB * node = hashmap[page_id];
		frame_ids.push(node->frame_id);
		node->disconnect();
		hashmap.erase(node->page_id);
		size--;
	}
	// 脏位
	int setDirty(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			return -1;
		}
		BCB * node = hashmap[page_id];
		node->dirty = 1;
		node->time += 1;
		node->disconnect(); // 放至首部
		head->Insert(node);
		return node->frame_id;
	}
	// 用户数
	int setCount(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			return -1;
		}
		BCB * node = hashmap[page_id];
		node->count += 1;
		node->time += 1;
		node->disconnect(); // 放至首部
		head->Insert(node);
		return node->frame_id;
	}
};

//class BufferManager {
//private:
//	LRUCache cache{ BUFFSIZE };
//	FILE * dataFile; // data.dbf
//public:
//	BufferManager(LRUCache cache, FILE * dataFile) : cache(cache), dataFile(dataFile) {
//	}
//
//	void FixPage(int page_id)
//	{
//		//将对应page_id的page读入到buffer中。如果buffer已满，则需要选择换出的frame
//		cache.read(page_id);
//	}
//
//	void FixNewPage()
//	{
//		//在插入数据时申请一个新page并将其放在buffer中。
//		cache.put(-1);
//	}
//
//
//	int SelectVictim()
//	{
//		//选择换出的frame_id
//		return cache.selectVictim();
//	}
//
//	int FindFrame(int page_id, LRUCache LRU)
//	{
//		//查找给定的page是否已经在某个frame中了
//		int frame_id = LRU.read(page_id);
//#if TEST_MODE
//		if (frame_id) { cout << "尝试寻找页号为" << page_id << "的块... 该块的内存缓冲页号为" << frame_id << endl; }
//		else { cout << "尝试寻找页号为" << page_id << "的块... 该块还未加载入内存缓冲！" << endl; }
//#endif
//		return -1;
//	}
//
//	void RemoveBCB(int page_id)
//	{
//		cache.remove(page_id);
//	}
//
//	void SetDirty(int page_id)
//	{
//		//认为指定frame的数据已经被修改
//		cache.setDirty(page_id);
//	}
//};#pragma once
