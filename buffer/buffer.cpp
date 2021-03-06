// buffer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
#include "pch.h"
#include "buffer.h"
#include "time.h"
#define CLOCKS_PER_SEC  ((clock_t)1000)
clock_t start, t_end; double duration;

void str2int(int &int_temp, const string &string_temp)
{
	stringstream stream(string_temp);
	stream >> int_temp;
}

vector<string> split(const string& str, const string& delim) {
	vector<string> res;
	if ("" == str) return res;
	//先将要切割的字符串从string类型转换为char*类型
	char * strs = new char[str.length() + 1];
	strcpy(strs, str.c_str());

	char * d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());

	char *p = strtok(strs, d);
	while (p) {
		string s = p; //分割得到的字符串转换为string类型
		res.push_back(s); //存入结果数组
		p = strtok(NULL, d);
	}
	delete strs, d;
	return res;
}

void CreateBlockWRTest(int num_blocks)
{
	// 仅作测试，一次写入多个块
	FILE * fstream_w;
	char dbfile[] = "data.dbf";
	fstream_w = fopen(dbfile, "wb"); // 不存在则创建，存在则覆盖！
	if (fstream_w == NULL)
	{
		printf("open or create file failed!\n");
		exit(1);
	}
	else {
		cout << "Open " << dbfile << " successfully!" << endl;
	}
	// 数据块号
	unsigned int block_num = 0;
	// 时间戳
	unsigned int btimestamp = 0;
	// 偏移量表
	unsigned short int offset[FRAMESIZE / RECORDSIZE];
	int BLOCK_HEAD = sizeof(btimestamp) + sizeof(offset);
	for (int i = 0; i < FRAMESIZE / RECORDSIZE; i++) { offset[i] = BLOCK_HEAD + i * RECORDSIZE; };

	for (int j = 0; j < num_blocks; j++)
		// 写入num_blocks个块，数据记录方法：固定格式定长记录
	{
		// 写入块首部
		block_num = j;
		int writeCnt = fwrite(&block_num, sizeof(block_num), 1, fstream_w); // 数据块号
		writeCnt = fwrite(&btimestamp, sizeof(btimestamp), 1, fstream_w); // 时间戳
		writeCnt = fwrite(&offset, sizeof(offset), 1, fstream_w); // 块内偏移表
		// 写入记录
		char * p_schema = NULL;
		unsigned int timestamp = j;
		unsigned int length = 316;
		char namePtr[32];
		char adrressPtr[256];
		char genderPtr[4];
		char birthdayPtr[12];
		for (int i = 0; i < FRAMESIZE / RECORDSIZE; i++) // 2048/316=12
		{
			writeCnt = fwrite(&fstream_w, sizeof(fstream_w), 1, fstream_w);
			writeCnt = fwrite(&length, sizeof(length), 1, fstream_w);
			writeCnt = fwrite(&timestamp, sizeof(timestamp), 1, fstream_w);
			writeCnt = fwrite(&namePtr, sizeof(namePtr), 1, fstream_w);
			writeCnt = fwrite(&adrressPtr, sizeof(adrressPtr), 1, fstream_w);
			writeCnt = fwrite(&genderPtr, sizeof(genderPtr), 1, fstream_w);
			writeCnt = fwrite(&birthdayPtr, sizeof(birthdayPtr), 1, fstream_w);
		}
		char null[272]; // 填充字节使得4KB对齐
		writeCnt = fwrite(&null, sizeof(null), 1, fstream_w);
	}


	fclose(fstream_w);
}


void NoIndexQuery(int find_block_num)
{
	// 查询指定块的块号
	FILE * fstream_r;
	fstream_r = fopen("data.dbf", "rb");
	unsigned int blocknum = 0;
	cout << "query target: " << find_block_num << endl;
	int offset = find_block_num * FRAMESIZE;
	fseek(fstream_r, offset, SEEK_SET);
	int readCnt = fread(&blocknum, sizeof(blocknum), 1, fstream_r);
	cout << " query result: " << blocknum << endl;

	fclose(fstream_r);
}

void processTrace(int page_id, int operation, LRUCache * LRUCache) {
	// 处理trace文件
	if (operation == 0) // 读操作
	{
#if VERBOSE
		cout << "---[trace 读操作] 读取page_id:" << page_id << " 的数据---" << endl;
#endif
		LRUCache->readData(page_id);
	}
	else if(operation == 1)
	{
#if VERBOSE
		cout << "---[trace 写操作] 修改page_id:" << page_id << " 的数据---" << endl;
#endif
		LRUCache->writeData(page_id, NULL);
	}
}


int main()
{
	cout << "*****************************************" << endl;
	cout << "*   Storage and Buffer Manager by ZHT   *" << endl;
	cout << "*****************************************" << endl;
	cout << "WELCOME!\n";
	//IndexBlockWRTest(); // 生成并写入索引块（不需要，已ab
	CreateBlockWRTest(50000); // 生成一个包含50000个4kb块的无索引的数据库文件，块内存在记录数据
	//NoIndexQuery(1000); // 测试查询
	string tracefilename = "data-5w-50w-zipf.txt";
	ifstream traceFile(tracefilename.c_str());
	if (!traceFile)
	{
		cout << "Error opening " << tracefilename << " for input" << endl;
		exit(-1);
	}
	else {
		cout << "Open " << tracefilename << " successfully!" << endl;
	}
	char dbpath[] = "data.dbf";
	// 实例缓冲器
	LRUCache LRUCache(BUFFSIZE, dbpath);

	int wr = 0; // 0读，1写
	int page_id = 0;
	string trace;
	int request_num = 0;
	cout << "Buffer frame size:" << BUFFSIZE << endl;
	cout << "BUFFER IO PERFORMANCE TESTING!\n";
	start = clock();
	while (!traceFile.eof()) // !file.eof()
	{
		getline(traceFile, trace); // 按行读取trace
		std::vector<string> res = split(trace, ",");
		str2int(wr, res[0]);
		str2int(page_id, res[1]);
		//cout << wr << " " << page_id << endl;
		processTrace(page_id, wr, &LRUCache); // 逐条执行trace的命令

		request_num++;
	}
	t_end = clock();
	cout << "BUFFER IO TEST OK!\n";
	LRUCache.saveDirty2Disk();
	cout << "Request:" << request_num << endl;
	cout << "IO:" << LRUCache.getIOnum() << endl;
	cout << "Hits:" << LRUCache.getHitNum() << endl;
	printf("Hit ratio:%f%%\n", 100 * LRUCache.getHitNum() / float(request_num));
	duration = ((double)t_end - start) / CLOCKS_PER_SEC;
	printf("trace consuming time: %f seconds\n", duration);
	// 测试更新节点
	system("pause");
	traceFile.close();
}