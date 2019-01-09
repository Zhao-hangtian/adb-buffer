// ����ṹ���뺯��

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
#define BUFFSIZE 1024 // frame��Ŀ
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
//		fstream = fopen(filepath, "wb"); // �������򴴽�
//	}
//	void readDisk(int page_id, int frame_id, bFrame buff[]){
//		bFrame bframe;
//		fseek(fstream, page_id, SEEK_SET);
//#if TEST2
//		if (fread(&bframe, sizeof(bframe), 1, fstream) == 1) {
//			cout << "��ȡ page_id:" << page_id << " �� frame_id:" << frame_id << " ������ɹ���";
//		}else{
//			cout << "��ȡ page_id:" << page_id << " �� frame_id:" << frame_id << " ������ʧ�ܣ�";
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
//			cout << "д�� page_id:" << page_id << " �� frame_id:" << frame_id << " �����̳ɹ���";
//		}else{
//			cout << "д�� page_id:" << page_id << " �� frame_id:" << frame_id << " ������ʧ�ܣ�";
//			}
//#else
//		int readCnt = fwrite(&bframe, sizeof(bframe), 1, fstream);
//#endif
//	}
//};

//˫������+��ϣ��ʵ��LRUCache
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
	// next�������ڵ�
	void Insert(BCB * node) {
		if (_next) {
			node->_prev = this; // this
			_next->_prev = node;
			node->_next = _next;
			_next = node;
		}
	}
};
// ����frames��pages��Ԫ����BCB
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
		head = new BCB; // ��(LinkNode)malloc(sizeof(LinkNode));
		tail = new BCB;
		head->_next = tail;
		tail->_prev = head;
		//DiskSpaceManager diskSpaceManager(filepath);
		for (int i = 0; i < capacity; i++)frame_ids.push(i);
		IOnum = hitNum = 0;
	}
	// ��������
	int read(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			return -1;
		}
		BCB * node = hashmap[page_id];
		node->disconnect(); // �����ײ�
		head->Insert(node);
		return node->frame_id;
	}
	// д�뻺��
	int put(int page_id, int frame_id = 0) {
		BCB * node;
		if (page_id == -1) {}; // ���������page
		if (hashmap.find(page_id) == hashmap.end()) {
			if (size < capacity) {
				frame_id = frame_ids.top();
				frame_ids.pop();
				node = new BCB(page_id, frame_id);
				size++;
			}
			else
			{
				// �û���������д�����
				node = tail->_prev;
				int i = 0;
				for (; node->ref > 1 || node->count > 1; node = node->_next)
				{
					node->ref = 0; i++; if (i == capacity) { cout << "���棺�������������ǿ���û�"; };
				};
				frame_id = node->frame_id; // frame_id����
				node->disconnect();
				hashmap.erase(node->page_id);
				node->page_id = page_id;
				node->frame_id = frame_id;
			}
			hashmap[page_id] = node;
			head->Insert(node); // �����ײ�
		}
		else {
			node = hashmap[page_id];
			node->disconnect(); // �����ײ�
			head->Insert(node);
			frame_id = node->frame_id;
		}
		return frame_id;
	}
	// �鿴Ҫ������frame_id
	int selectVictim() {
		BCB * node;
		node = tail->_prev;
		int i = 0;
		for (; node->ref > 1 || node->count > 1; node = node->_next)
		{
			node->ref = 0; i++; if (i == capacity) { cout << "���棺�����������������ǿ���û�"; };
		};
		return node->frame_id;
	}
	// �ӻ���ɾ��
	void remove(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			cout << "����page_id: " << page_id << " �����ڻ����У�" << endl;
		}
		// ������д�����
		BCB * node = hashmap[page_id];
		frame_ids.push(node->frame_id);
		node->disconnect();
		hashmap.erase(node->page_id);
		size--;
	}
	// ��λ
	int setDirty(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			return -1;
		}
		BCB * node = hashmap[page_id];
		node->dirty = 1;
		node->time += 1;
		node->disconnect(); // �����ײ�
		head->Insert(node);
		return node->frame_id;
	}
	// �û���
	int setCount(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			return -1;
		}
		BCB * node = hashmap[page_id];
		node->count += 1;
		node->time += 1;
		node->disconnect(); // �����ײ�
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
//		//����Ӧpage_id��page���뵽buffer�С����buffer����������Ҫѡ�񻻳���frame
//		cache.read(page_id);
//	}
//
//	void FixNewPage()
//	{
//		//�ڲ�������ʱ����һ����page���������buffer�С�
//		cache.put(-1);
//	}
//
//
//	int SelectVictim()
//	{
//		//ѡ�񻻳���frame_id
//		return cache.selectVictim();
//	}
//
//	int FindFrame(int page_id, LRUCache LRU)
//	{
//		//���Ҹ�����page�Ƿ��Ѿ���ĳ��frame����
//		int frame_id = LRU.read(page_id);
//#if TEST_MODE
//		if (frame_id) { cout << "����Ѱ��ҳ��Ϊ" << page_id << "�Ŀ�... �ÿ���ڴ滺��ҳ��Ϊ" << frame_id << endl; }
//		else { cout << "����Ѱ��ҳ��Ϊ" << page_id << "�Ŀ�... �ÿ黹δ�������ڴ滺�壡" << endl; }
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
//		//��Ϊָ��frame�������Ѿ����޸�
//		cache.setDirty(page_id);
//	}
//};#pragma once
