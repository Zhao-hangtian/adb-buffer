#define VERBOSE 0
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
#define BUFFSIZE 32768 // ���޸Ļ�������С
#define MAXSIZE  FRAMESIZE*BUFFSIZE

using namespace std;

struct bFrame
{
	char field[FRAMESIZE];
};

struct bBuff
{
	bFrame frames[BUFFSIZE];
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


class DiskSpaceManager {
private:
	FILE * fstream;
public:
	DiskSpaceManager() :fstream(NULL){}

	void openFile(char filepath[]) {
		fstream = fopen(filepath, "rb+"); // �������򴴽�
	}

	void readDisk(int page_id, int frame_id, bBuff * buff){
		bFrame bframe;
		// ���ݿ��
		unsigned int block_num = 0;
		fseek(fstream, page_id*FRAMESIZE, SEEK_SET);
		//cout << "����ֵ: " << fread(&bframe.field, sizeof(bframe.field), 1, fstream) << endl;
		if (fread(&bframe.field, sizeof(bframe.field), 1, fstream) == 1) {
#if VERBOSE
			cout << "��ȡ page_id:" << page_id << " �� frame_id:" << frame_id << " ... ����ɹ���" << endl;
#endif
		}
		else {
#if VERBOSE
			cout << "��ȡ page_id:" << page_id << " �� frame_id:" << frame_id << " ... ����ʧ�ܣ�" << endl;
#endif
		}
		buff->frames[frame_id] = bframe;
	}

	void writeDisk(int page_id, int frame_id, bBuff * buff) {
		bFrame bframe = buff->frames[frame_id];
		fseek(fstream, page_id*FRAMESIZE, SEEK_SET);
		if (fwrite(&bframe.field, sizeof(bframe.field), 1, fstream) == 1) {
#if VERBOSE
			cout << "д�� frame_id:" << frame_id << " �� page_id:" << page_id << " ���̳ɹ���" << endl;
#endif
		}
		else {
#if VERBOSE
			cout << "д�� frame_id:" << frame_id << " �� page_id:" << page_id << " ����ʧ�ܣ�" << endl;
#endif
		}
	}
};

//˫������+��ϣ��ʵ��LRUCache
struct BCB
{
	int page_id, frame_id, ref, count, time, dirty;
	BCB * _prev;
	BCB * _next;
	BCB() : _next(NULL), _prev(NULL) {}
	BCB(int page_id, int frame_id) : page_id(page_id), frame_id(frame_id), count(0), time(0), dirty(0) { BCB(); }
	// �Ͽ����ڵ�
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
	int size, capacity; //��ǰ��С����������
	unordered_map<int, BCB*> hashmap;
	stack<int> frame_ids;
	BCB * head, *tail;
	int hitNum; // ������
	int IONum; // ����IO
	bBuff * buff;
	int f2p[BUFFSIZE]; // frame_id��page_id��ӳ��
	DiskSpaceManager diskSpaceManager;
public:
	LRUCache(int capacity, char dbpath[]) : size(0), capacity(capacity){
		head = new BCB; // ��(LinkNode)malloc(sizeof(LinkNode));
		tail = new BCB;
		head->_next = tail;
		tail->_prev = head;
		hitNum = IONum = 0;
		buff = new bBuff;
		diskSpaceManager.openFile(dbpath);
		for (int i = 0; i < BUFFSIZE; i++)f2p[i] = 0;
		for (int i = 0; i < capacity; i++)frame_ids.push(i);
	}
	// ����LRUCache��Ӧ��frame_id
	int read(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
#if VERBOSE
			cout << "��page_id: " << page_id << " δ�����ڻ����У�" << endl;
#endif
			return -1;
		}
		BCB * node = hashmap[page_id];
		node->disconnect(); // �����ײ�
		head->Insert(node);
		int fid = node->frame_id;
		return fid;
	}

	// ���鿴frame_id
	int read_id(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
#if VERBOSE
			cout << "��page_id: " << page_id << " δ�����ڻ����У�";
#endif
			return -1;
		}
		BCB * node = hashmap[page_id];
		return node->frame_id;
	}

	// д��LRUCache��Ӧ��frame_id
	int put(int page_id, int frame_id = 0) {
		BCB * node;
		if (page_id == -1) { cout << "���������page" << endl; }; // ���������page
		if (hashmap.find(page_id) == hashmap.end()) {
			if (size < capacity) {
				frame_id = frame_ids.top();
				frame_ids.pop();
				node = new BCB(page_id, frame_id);
				size++;
			}
			else
			{
				// �û�
				node = tail->_prev;
				int i = 0;
				for (; node->ref > 1 || node->count > 1; node = node->_next)
				{
					node->ref = 0; i++; if (i == capacity-1) { cout << "���棺�������������ǿ���û�"; };
					break;
				};
				// ������д����棡
				if (node->dirty)
				{
					node->dirty = 0;
					diskSpaceManager.writeDisk(node->page_id, node->frame_id, buff);
#if VERBOSE
					cout << "���������������û����ѽ�page_id:" << node->page_id << " ���ݿ�д����̣�" << endl;
#endif
				}
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

	unsigned int readData(int page_id) {
		int frame_id = read(page_id);
		if (frame_id == -1)
		{
		// dont hit, io
			IONum++;
			frame_id = put(page_id);
			diskSpaceManager.readDisk(page_id, frame_id, buff);
		}
		else {
		// hit, no IO
			hitNum++;
		}
#if VERBOSE
		unsigned int * bln_ptr;
		char bln[4];
		for (int i = 0; i < 4; i++)
		{
			bln[i] = buff->frames[frame_id].field[i];
		}
		bln_ptr = (unsigned int *)&bln;
		cout << "���������ݣ���ţ�:" << *bln_ptr << " ��ָ��ָ���ͷ����ȡ����Ϊ4bit����¼�Ÿÿ�������.dbf�ļ��Ŀ�ţ�" << endl;
		if (*bln_ptr == page_id) cout << "��ȡ������֤ͨ��!" << endl;
#endif
		return 1;
	}

	int writeData(int page_id, char * data) { // ��д��ʱ������в���
		int frame_id = read(page_id);
		if (frame_id == -1)
		{
			// dont hit, IO
			IONum++;
			frame_id = put(page_id);
			diskSpaceManager.readDisk(page_id, frame_id, buff); // �Ƚ�page���뻺��
		}
		else {
			// hit, no IO
			hitNum++;
		}
		buff->frames[frame_id].field[1]++; // ʱ�����1��д�뻺��
		BCB * node = hashmap[page_id];
		node->dirty = 1; // ��λ��1
#if VERBOSE
		unsigned int * timestamp;
		char ts[4];
		for (int i = 0; i < 4; i++)
		{
			ts[i] = buff->frames[frame_id].field[4+i];
		}
		timestamp = (unsigned int *)&ts;
		cout << "����д������ݣ�ʱ�����:" << *timestamp << " ��ָ��ָ������ͷ4bitλ�ã�д�볤��Ϊ4bit����¼�Ÿÿ��ʱ�����" << endl;;
#endif
		return 1;
	}

	int getIOnum() {
		return IONum;
	}

	int getHitNum() {
		return hitNum;
	}

	// ��ɶ���LRUCache�ͷ�ǰ��һЩ����
	void saveDirty2Disk() {
		for (BCB * node = head->_next; node!=tail; node=node->_next)
		{
			if (node->dirty == 1)
			{
				diskSpaceManager.writeDisk(node->page_id, node->frame_id, buff);
				IONum++;
				node->dirty = 0;
			}
		}
		delete tail,head,buff;
#if VERBOSE
		cout << "�����ر�LRUCache: �ѽ�ȫ��������д������!" << endl;
#endif
	}
};