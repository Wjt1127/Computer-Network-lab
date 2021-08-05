#define _CRT_SECURE_NO_WARNINGS
#include<winsock.h>   //对于Winsock 2,应该#include<winsock2.h>
#include<iostream>
#include<stdio.h>
#include<string.h>
#include<ctime>
#pragma comment(lib,"ws2_32.lib")  //连接ws2_32.lib 否则会有连接错误
using namespace std;

#define SERVER_PORT 69   //定义客户端初始连接（伪连接）端口号
#define BUFFER_SIZE 1024 //设置缓冲区的大小
#define File_Size  100   //设置文件名的最大长度
#define Time_Out 30      //设置超时重传的时间(us)
#define Max_Try  4
const char* Write_File_Path = "C:\\Users\\12432\\Desktop\\计网实验\\Socket\\Tftpd64\\上传文件\\";   //设置写请求时 写文件的源文件路径
const char* Read_File_Path = "C:\\Users\\12432\\Desktop\\计网实验\\Socket\\Tftpd64\\下载文件\\";   //设置读请求时 读文件的文件路径
const char* Log_File = "C:\\Users\\12432\\Desktop\\计网实验\\Socket\\Tftpd64\\日志文件.log";   //设置日志文件路径
WSADATA wsaData;  //为winsock的初始化使用
struct sockaddr_in server_addr;   //设置服务器端的地址信息 注意每次进行请求报文前将port变为69号端口
FILE* fp, * fpr;   //用于文件读写的  设为全局变量是为了方便实现打包函数
FILE* logfile;  //日志文件的写操作  程序结束才关闭log
time_t rawtime;   //为了在日志文件中显示时间设置的
struct tm* timeinfo;
string temp;

//设置输出的颜色
BOOL SetConsoleColor(WORD wa) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole == INVALID_HANDLE_VALUE)
		return FALSE;

	return SetConsoleTextAttribute(hConsole, wa);
}

void Timeshow()
{
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	fprintf(logfile, "%s", asctime(timeinfo));
}

//实现微秒级睡眠
void MSleep(long lTime)
{
	LARGE_INTEGER litmp;
	LONGLONG QPart1, QPart2;
	double dfMinus, dfFreq, dfTim, dfSpec;
	QueryPerformanceFrequency(&litmp);
	dfFreq = (double)litmp.QuadPart;
	QueryPerformanceCounter(&litmp);
	QPart1 = litmp.QuadPart;
	dfSpec = 0.000001 * lTime;

	do
	{
		QueryPerformanceCounter(&litmp);
		QPart2 = litmp.QuadPart;
		dfMinus = (double)(QPart2 - QPart1);
		dfTim = dfMinus / dfFreq;
	} while (dfTim < dfSpec);
}

/*
函数名：Write(char *receive,int l)
输入：接收到的数据 和数据大小
返回：无
功能：实现linux版本服务器的文件写
*/
void Write(char* receive, int l) {
	for (int i = 4; i < l; i++) {
		fwrite(&receive[i], sizeof(char), 1, fpr);
	}
}


/*
函数名：Make_Write_Request
输入：char *Buffer  存储请求包的数组
返回：写请求报文的字节大小
功能：生成一个写请求包的数组Buffer
*/
int Make_Write_Request(char* Buffer) {
	int count = 0;
	memset(Buffer, 0, sizeof(Buffer));
	char Filename[File_Size];
	memset(Filename, 0, sizeof(Filename));
	Buffer[0] = 0; Buffer[1] = 2;		//设置Opcode为2
	count += 2;

	cout << "\t\t\t\t请输入你想向服务器写入的文件名:";
	cin >> Filename;
	memcpy(Buffer + count, Filename, strlen(Filename));  //将文件名接在操作码后
	count += strlen(Filename);
	Buffer[count] = 0;   //文件名后接  0 表示文件名结束
	count++;

	string File = Write_File_Path + (string)Filename;   //形成写文件的路径
	temp = Filename;

	int mod;   //模式的选择
	cout << "\t\t\t\t请选择你想写的数据的类型，（0）ASCII码形式\t（1）二进制形式\n";
	cout << "\t\t\t\t\t";
	cin >> mod;
	if (mod == 0) {
		Buffer[count] = 'n'; Buffer[count + 1] = 'e';
		Buffer[count + 2] = 't'; Buffer[count + 3] = 'a';
		Buffer[count + 4] = 's'; Buffer[count + 5] = 'c';
		Buffer[count + 6] = 'i'; Buffer[count + 7] = 'i';
		Buffer[count + 8] = 0;
		count += 9;
		if ((fp = fopen(File.c_str(), "r+b")) == NULL) {   //如果时选择ASCII模式那么使用 r 模式读文件
			cout << "\t\t\t\t文件打开失败\n";
			return 0;
		}
	}
	else {
		Buffer[count] = 'o'; Buffer[count + 1] = 'c';
		Buffer[count + 2] = 't'; Buffer[count + 3] = 'e';
		Buffer[count + 4] = 't';
		Buffer[count + 5] = 0;
		count += 6;
		if ((fp = fopen(File.c_str(), "rb")) == NULL) {   //如果时选择二进制模式那么使用 rb 模式读文件
			cout << "\t\t\t\t文件打开失败\n";
			return 0;
		}
	}
	return count;   //返回请求报文的字节大小
}


/*
函数名：Make_Data_Package
输入：char* Buffer  一个字符数组   short Block_Num  打包的块号
返回：返回打包的Buffer的大小
功能：将输入的Buffer函数填充满数据
*/
int Make_Data_Package(char* Buffer, int Block_Num) {

	memset(Buffer, 0, sizeof(Buffer));
	Buffer[0] = 0; Buffer[1] = 3;   //设置Opcode 为3
	Buffer[2] = (short)Block_Num >> 8; Buffer[3] = Block_Num & 0xff;
	int length = fread(Buffer + 4, 1, 512, fp);
	return (4 + length);   //返回Buffer的长度
}

/*
函数名：Make_Read_Request
输入：char *Buffer  用于存放请求包的数组
返回：请求包的字节大小
功能：将请求包组装好 存在Buffer数组中
*/
int Make_Read_Request(char* Buffer) {
	int count = 0;
	memset(Buffer, 0, sizeof(Buffer));
	char Filename[File_Size];
	memset(Filename, 0, sizeof(Filename));
	Buffer[0] = 0; Buffer[1] = 1;		//设置Opcode为1
	count += 2;

	cout << "\t\t\t\t请输入你想从服务器中下载的文件名:";
	cin >> Filename;
	memcpy(Buffer + count, Filename, strlen(Filename));  //将文件名接在操作码后
	count += strlen(Filename);
	Buffer[count] = 0;   //文件名后接  0 表示文件名结束
	count++;

	string File = Read_File_Path + (string)Filename;   //形成写文件的路径

	int mod;   //模式的选择
	cout << "\t\t\t\t请选择你想写的数据的类型，（0）ASCII码形式\t（1）二进制形式\n";
	cout << "\t\t\t\t";
	cin >> mod;
	if (mod == 0) {
		Buffer[count] = 'n'; Buffer[count + 1] = 'e';
		Buffer[count + 2] = 't'; Buffer[count + 3] = 'a';
		Buffer[count + 4] = 's'; Buffer[count + 5] = 'c';
		Buffer[count + 6] = 'i'; Buffer[count + 7] = 'i';
		Buffer[count + 8] = 0;
		count += 9;
		if ((fpr = fopen(File.c_str(), "w+b")) == NULL) {   //如果时选择ASCII码模式那么使用 w 模式写文件
			cout << "\t\t\t\t文件打开失败\n";
			exit(-1);
		}
	}
	else {
		Buffer[count] = 'o'; Buffer[count + 1] = 'c';
		Buffer[count + 2] = 't'; Buffer[count + 3] = 'e';
		Buffer[count + 4] = 't';
		Buffer[count + 5] = 0;
		count += 6;
		if ((fpr = fopen(File.c_str(), "wb")) == NULL) {   //如果时选择二进制模式那么使用 wb 模式读文件
			cout << "\t\t\t\t文件打开失败\n";
			exit(-1);
		}
	}

	return count;   //返回请求报文的字节大小
}

/*
函数名：Make_ACK
输入：char *Buffer  用于存储ACK报文的数组  int block 块号
返回：无
功能：将ACK编辑好存放在Buffer中
*/
void Make_ACK(char* Buffer, int block) {
	memset(Buffer, 0, BUFFER_SIZE * sizeof(char));
	Buffer[0] = 0; Buffer[1] = 4;//操作码为4
	Buffer[2] = ((short)block) >> 8; Buffer[3] = block & 0xff;  //设置确认块号
}


/*
函数名：Write_Request(void)
参数：无
返回值:无
功能：向服务器发送写文件请求 同时将数据上传给服务器
*/
void Write_Request(void) {

	//定义客户端的地址信息
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 0;   //设置为0  表示自动分配
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //分配ip就是本地地址

	//创建socket套接字
	SOCKET client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {   //小于0 套接字创建失败
		printf("错误码：%d    ", WSAGetLastError());
		printf("客户端套接字socket创建失败\n");
		exit(-1);
	}

	//将阻塞I/O设置为非阻塞
	u_long mode = 1;
	ioctlsocket(client_fd, FIONBIO, &mode);

	//绑定套接字地址信息  如果要接受信息一定要使用bind 绑定地址信息 这样别人sendto()的信息 才能被接收
	if ((bind(client_fd, (LPSOCKADDR)&client_addr, sizeof(client_addr)))) {   //转化成LPSOCKADDR才行
		printf("错误码：%d    ", WSAGetLastError());
		printf("服务器端绑定失败\n");
		exit(-1);
	}

	long long Sum = 0;
	char Buffer[BUFFER_SIZE];
	memset(Buffer, 0, sizeof(Buffer));
	int count = Make_Write_Request(Buffer);  //得到请求报文的
	int Request_flag = 0;   //设置一个请求报文flag  判断请求报文是否
	long long start = clock();
	//向服务器发送WRQ数据包
	//***!!!!   sendto函数的第二个参数一定要发送请求报文的实际字节大小
	if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		printf("错误码：%d    ", WSAGetLastError());
		printf("客户端发送数据失败\n");
		exit(-1);
	}

	Sum += count;
	char* srcip = inet_ntoa(client_addr.sin_addr);
	char* dstip = inet_ntoa(server_addr.sin_addr);
	fprintf(logfile, "发起上传请求报文,报文长度为%d字节,ip.src=%s,ip.dst=%s\t", count, srcip, dstip);
	printf("发送请求报文\n");
	Timeshow();
	fprintf(logfile, "请求上传%s文件，传输数据模式为%s\t", Buffer + 2, Buffer + 2 + strlen(Buffer + 2) + 1);
	printf( "传输数据模式为%s\n",Buffer + 2 + strlen(Buffer + 2) + 1);
	Timeshow();

	char receive[BUFFER_SIZE];
	memset(receive, 0, sizeof(receive));
	//接收服务器返回的确认信息包
	int server_addr_len = sizeof(server_addr);

	int l;  //l用于记录发送的数据包的大小
	int e = 0;
	while(1){
		e++;
		l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
		if (l > 0) {   //返回值大于0  就说明接收成功
			Sum += l;
			break;
		}
		MSleep(Time_Out);
		if (e > 10) {
			fprintf(logfile, "上传请求超时，进行重传\t");
			printf("上传请求超时，重传请求报文\n");
			e = 0;
			if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
				printf("错误码：%d    ", WSAGetLastError());
				printf("客户端发送数据失败\n");
				exit(-1);
			}
			Sum += count;
			Sleep(10);
		}
	}

	int block = 0, ack;   //块号标识
	int flag = 0;
	while (1) {
		//count为-1表示 超时ACK  那么重传数据包
		if (l == -1) {
			/*{
				if (block == 0) {
					MSleep(2000);
				}
			}*/
			//向服务器重新发送数据包
			Timeshow();
			fprintf(logfile, "\t接收服务器返回报文超时,重传数据报文,Block=%d\n", block);
			printf("接收服务器返回报文超时,重传数据报文,Block=%d\n", block);
			
			//***!!!!   sendto函数的第二个参数一定要发送请求报文的实际字节大小
			if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
				printf("错误码：%d    ", WSAGetLastError());
				printf("客户端发送数据失败\n");
				exit(-1);
			}
			Sum += count;
			while(1) {
				l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
				if (l > 0) {   //返回值大于0  就说明接收成功
					Sum += l;
					break;
				}
				MSleep(Time_Out);
			}
		}
		else {  //接收到了客户端的确认报文
			if (receive[1] == 4) {
				ack = (((receive[2] & 0xff) << 8) + (receive[3] & 0xff)) & 0xffff;
				if (ack == block) {   //收到的确认报文确认号和发送的数据包的块相同
					block++;
					Sum += 4;
					Timeshow();
					fprintf(logfile, "\t客户端收到服务器的确认报文，ACK=%d\n", ack);
					printf("收到确认报文，ACK=%d\n", ack);
					block %= 65536;
					count = Make_Data_Package(Buffer, block);	//返回数据包的大小 
					//向服务器发送数据包
					if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
						printf("错误码：%d    ", WSAGetLastError());
						printf("客户端发送数据失败\n");
						exit(-1);
					}

					Sum += count;
					Timeshow();
					fprintf(logfile, "\t上传 %d 块数据，报文大小为 %d 字节,ip.src=%s,ip.dst=%s\n", block, count, srcip, dstip);
					printf("上传 %d 块数据\n", block);

					memset(receive, 0, sizeof(receive));
					//接收服务器返回的确认信息包
					while(1) {
						l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
						if (l > 0) {   //返回值大于0  就说明接收成功
							Sum += l;
							break;
							MSleep(Time_Out);
						}
					}

					if (count < 516 && l) break;   // l>0说明成功获得返回报文  count<516则说明文件写完了 而且只要收到了确认消息就结束
				}
				else {  //服务器返回的ACK与块号不相同
					//向服务器重新发送数据包
					Timeshow();
					fprintf(logfile, "\t接收服务器返回的确认报文错误,ACK=%d,重传数据报文,Block=%d\n", ack, block);
					printf("接收服务器返回的确认报文错误,ACK=%d,重传数据报文,Block=%d\n", ack, block);
					//***!!!!   sendto函数的第二个参数一定要发送请求报文的实际字节大小
					if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
						printf("错误码：%d    ", WSAGetLastError());
						printf("客户端发送数据失败  ");
						exit(-1);
					}
					Sum += count;

					while(1) {
						l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
						if (l > 0) {   //返回值大于0  就说明接收成功
							Sum += l;
							break;
						}
						MSleep(Time_Out);
					}
				}
			}

			if (receive[1] == 5) {
				Timeshow();fprintf(logfile, "\t客户端接收到服务器的错误包\n"); 
				printf("客户端接收到服务器的错误包\n");
				flag = 1;  //错误包直接结束
				switch (receive[3]) {
				case 0:  Timeshow();fprintf(logfile, "\t错误！错误码为：0\t错误信息：%s\n", receive + 4);
					break;
				case 1: Timeshow();fprintf(logfile, "\t错误！错误码为：1\t错误信息：File not found\n"); 
					break;
				case 2: Timeshow();fprintf(logfile, "\t错误！错误码为：2\t错误信息：Access violation\n"); 
					break;
				case 3: Timeshow();fprintf(logfile, "\t错误！错误码为：3\t错误信息：Disk full or alloction exceeded\n"); 
					break;
				case 4:  Timeshow();fprintf(logfile, "\t错误！错误码为：4\n错误信息：Illegal TFTP operation\n");
					break;
				case 5:  Timeshow();fprintf(logfile, "\t错误！错误码为：5\n错误信息：ID Unknown transfer ID\n");
					break;
				case 6:  Timeshow();fprintf(logfile, "\t错误！错误码为：6\n错误信息：File allready exits\n");
					break;
				case 7:  Timeshow();fprintf(logfile, "\t错误！错误码为：7\n");
					break;
				}
			}
			if (flag) break;
		}
	}
	long long end = clock();
	Timeshow();
	fprintf(logfile, "\t数据传输结束\n");
	printf("数据传输结束\n");

	if (!flag) {		
		Timeshow();
		fprintf(logfile, "\t本次上传文件的平均吞吐量为 %.3f KBps\n", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
		SetConsoleColor(FOREGROUND_RED);
		printf("\t\t\t\t本次下载文件的平均吞吐量为 %.3f KBps  \n", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
		SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	}
	closesocket(client_fd);
	fclose(fp);

}
/*
函数名：Read_Request
输入：char *Buffervoid
返回：无
功能：实现
*/
void Read_Request(void) {

	// 定义客户端的地址信息
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 0;   //设置为0  表示自动分配
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //分配ip

	//创建socket套接字
	SOCKET client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {   //小于0 套接字创建失败
		printf("\t错误码：%d    ", WSAGetLastError());
		printf("客户端套接字socket创建失败\n");

		exit(-1);
	}

	//将阻塞I/O设置为非阻塞
	u_long mode = 1;
	ioctlsocket(client_fd, FIONBIO, &mode);

	//绑定套接字地址信息  如果要接受信息一定要使用bind 绑定地址信息 这样别人sendto()的信息 才能被接收
	if ((bind(client_fd, (LPSOCKADDR)&client_addr, sizeof(client_addr)))) {   //转化成LPSOCKADDR才行
		printf("错误码：%d    ", WSAGetLastError());
		printf("服务器端绑定失败\n");
		exit(-1);
	}

	char Buffer[BUFFER_SIZE];
	memset(Buffer, 0, sizeof(Buffer));
	int count = Make_Read_Request(Buffer);  //得到请求报文的字节数大小 并生成下载请求报文存在Buffer中
	int l;   // l 用于记录数据包的大小
	long long Sum = 0;   //Sum是计算吞吐量的总数据大小

	long long start = clock();   //从传输请求报文开始计算时间
	//向服务器发送RRQ数据包
	//***!!!!   sendto函数的第二个参数一定要发送请求报文的实际字节大小
	if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		printf("\t错误码：%d    ", WSAGetLastError());
		printf("客户端发送数据失败\n");
		exit(-1);
	}
	Sum += count;

	char* srcip = inet_ntoa(client_addr.sin_addr);
	char* dstip = inet_ntoa(server_addr.sin_addr);
	Timeshow();
	fprintf(logfile, "\t发起下载请求报文,报文长度为%d字节,ip.src=%s,ip.dst=%s\n", count, srcip, dstip);
	printf("发起下载请求报文,报文长度为%d字节,ip.src=%s,ip.dst=%s\n", count, srcip, dstip);
	Timeshow();
	fprintf(logfile, "\t请求下载%s文件，传输数据模式为%s\n", Buffer + 2, Buffer + 2 + strlen(Buffer + 2) + 1);
	printf("请求下载%s文件，传输数据模式为%s\n", Buffer + 2, Buffer + 2 + strlen(Buffer + 2) + 1);
	
	int C = 0;
	char receive[BUFFER_SIZE];
	memset(receive, 0, sizeof(receive));
	//接收服务器返回的确认信息包
	int server_addr_len = sizeof(server_addr);
	int b = 0;
	while (1) {
		b++;
		memset(receive, 0, sizeof(receive));
		l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
		if (l > 0) {   //返回值大于0  就说明接收成功
			Sum += l;
			break;
		}
		Sleep(100);
		if (b > 10) {
			Timeshow();
			fprintf(logfile, "\t进行超时重传下载请求报文\n");
			printf("进行超时重传下载请求报文\n");
			
			if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
				printf( "错误码：%d    ", WSAGetLastError());
				printf( "客户端发送数据失败\n");
				exit(-1);
			}
			Sum += count;
			b = 0;
		}
	}
	C = 0;

	//判断此时收到的是确认包  还是错误包
	int block;   //块号标识
	int flag = 0;
	while (1) {
		if (receive[1] == 3) {
			block = (((receive[2] & 0xff) << 8) + (receive[3] & 0xff)) & 0xffff;
			Timeshow();
			fprintf(logfile, "\t接收到第%d块数据包\n", block);
			printf("接收到第%d块数据包\n", block);
			Write(receive, l);
			Make_ACK(Buffer, block);
			int count = 0;
			if (l < 516) {
				if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					printf( "错误码：%d    ", WSAGetLastError());
					printf( "客户端发送数据失败\n");
					exit(-1);
				}
				Sleep(5);
				if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					printf("错误码：%d    ", WSAGetLastError());
					printf("客户端发送数据失败\n");
					exit(-1);
				}
				Sleep(5);
				if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					printf("错误码：%d    ", WSAGetLastError());
					printf("客户端发送数据失败\n");
					exit(-1);
				}
				Sleep(5);
				Sum += 12;
				break;
			}
			else {
				if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					printf("错误码：%d    ", WSAGetLastError());
					printf("客户端发送数据失败\n");
					exit(-1);
				}
			}
			Sum += 4;
			while (1) {
				b++;
				l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
				if (l > 0) {   //返回值大于0  就说明接收成功
					Sum += l;
					break;
				}
				Sleep(100);
				if (b > 10) {
					Timeshow();
					fprintf(logfile, "\t进行超时重传,重新发送%d号ACK报文\n", block);
					printf("进行超时重传,重新发送%d号ACK报文\n", block);
					
					if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
						printf("错误码：%d    ", WSAGetLastError());
						printf("客户端发送数据失败\n");
						exit(-1);
					}
					Sum += 4;
					b = 0;
				}
			}
		}

		if (receive[1] == 5) {
			Timeshow(); fprintf(logfile, "\t客户端接收到服务器的错误包\n");
			printf("客户端接收到服务器的错误包\n");
			flag = 1;  //错误包直接结束
			switch (receive[3]) {
			case 0:  Timeshow(); fprintf(logfile, "\t错误！错误码为：0\t错误信息：%s\n", receive + 4);
				break;
			case 1: Timeshow(); fprintf(logfile, "\t错误！错误码为：1\t错误信息：File not found\n");
				break;
			case 2: Timeshow(); fprintf(logfile, "\t错误！错误码为：2\t错误信息：Access violation\n");
				break;
			case 3: Timeshow(); fprintf(logfile, "\t错误！错误码为：3\t错误信息：Disk full or alloction exceeded\n");
				break;
			case 4:  Timeshow(); fprintf(logfile, "\t错误！错误码为：4\n错误信息：Illegal TFTP operation\n");
				break;
			case 5:  Timeshow(); fprintf(logfile, "\t错误！错误码为：5\n错误信息：ID Unknown transfer ID\n");
				break;
			case 6:  Timeshow(); fprintf(logfile, "\t错误！错误码为：6\n错误信息：File allready exits\n");
				break;
			case 7:  Timeshow(); fprintf(logfile, "\t错误！错误码为：7\n");
				break;
			}
		}
		if (flag) break;
	}
	long long end = clock();
	Timeshow();
	fprintf(logfile, "\t数据传输结束\n");
	if (!flag) {
		fprintf(logfile, "本次下载文件的平均吞吐量为 %.3f KBps  ", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
		SetConsoleColor(FOREGROUND_RED);
		printf("\t\t\t\t本次下载文件的平均吞吐量为 %.3f KBps  \n", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
		SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	}
	closesocket(client_fd);
	fclose(fpr);
}


int main() {
	string a;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr("10.12.168.121");
	while (1)
	{
		logfile = fopen(Log_File, "a");
		//初始化Winsock
		int re = WSAStartup(0x0202, &wsaData);
		if (re) {   //返回值为0 才初始化Winsock成功
			printf("错误码：%d    ", WSAGetLastError());
			printf("winsock初始化错误\n");
			exit(-1);
		}
		system("cls");
		server_addr.sin_port = htons(SERVER_PORT);
		int op;
		cout << "\n\n\n\n\n\n\n";
		printf("\t\t\t\t*-------------------------------------*\n");
		printf("\t\t\t\t*\t选择你要进行的操作序号:       *\n");
		printf("\t\t\t\t*\t1: 从服务器下载文件           *\n");
		printf("\t\t\t\t*\t2: 向服务器上传文件           *\n");
		printf("\t\t\t\t*\t0: 退出                       *\n");
		printf("\t\t\t\t*-------------------------------------*\n");
		cout << "\t\t\t\t输入选择：";
		scanf("%d", &op);
		if (op == 1) {
			Read_Request(); cout << "\t\t\t\t"; system("pause");
		}
		else if (op == 2) {
			Write_Request(); cout << "\t\t\t\t"; system("pause");
		}
		else break;
		fclose(logfile);
		WSACleanup();
	}
	return 0;
}