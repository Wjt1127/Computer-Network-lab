#define _CRT_SECURE_NO_WARNINGS
#include<winsock.h>   
#include<iostream>
#include<stdio.h>
#include<string.h>
#include<ctime>
#pragma comment(lib,"ws2_32.lib")  
using namespace std;

#define SERVER_PORT 69   //定义客户端初始连接端口号
#define BUFFER_SIZE 1024 
#define File_Size  100   
#define Time_Out 30      

const char* Write_File_Path = "C:\\Users\\12432\\Desktop\\计网实验\\Socket\\Tftpd64\\上传文件\\";
const char* Read_File_Path = "C:\\Users\\12432\\Desktop\\计网实验\\Socket\\Tftpd64\\下载文件\\";
const char* Log_File = "C:\\Users\\12432\\Desktop\\计网实验\\Socket\\Tftpd64\\日志文件.log";
WSADATA wsaData;
struct sockaddr_in server_addr;
FILE* fp, * fpr;
FILE* logfile;
time_t rawtime;
struct tm* timeinfo;

void Timeshow()
{
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	fprintf(logfile, "%s", asctime(timeinfo));
}

int Make_Write_Request(char* Buffer) {
	int count = 0;
	memset(Buffer, 0, sizeof(Buffer));
	char Filename[File_Size];
	memset(Filename, 0, sizeof(Filename));
	Buffer[0] = 0; Buffer[1] = 2;
	count += 2;

	cout << "\t\t\t\t请输入你想向服务器写入的文件名:";
	cin >> Filename;
	memcpy(Buffer + count, Filename, strlen(Filename));
	count += strlen(Filename);
	Buffer[count] = 0;
	count++;

	string File = Write_File_Path + (string)Filename;

	int mod;
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
		if ((fp = fopen(File.c_str(), "rb")) == NULL) {
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
		if ((fp = fopen(File.c_str(), "rb")) == NULL) {
			cout << "\t\t\t\t文件打开失败\n";
			exit(-1);
		}
	}
	return count;
}


int Make_Data_Package(char* Buffer, int Block_Num) {

	memset(Buffer, 0, sizeof(Buffer));
	Buffer[0] = 0; Buffer[1] = 3;   //设置Opcode 为3
	Buffer[2] = (short)Block_Num >> 8; Buffer[3] = Block_Num & 0xff;
	int length = fread(Buffer + 4, 1, 512, fp);
	return (4 + length);   //返回Buffer的长度
}


int Make_Read_Request(char* Buffer) {
	int count = 0;
	memset(Buffer, 0, sizeof(Buffer));
	char Filename[File_Size];
	memset(Filename, 0, sizeof(Filename));
	Buffer[0] = 0; Buffer[1] = 1;		//设置Opcode为1
	count += 2;

	cout << "\t\t\t\t请输入你想从服务器中下载的文件名:";
	cin >> Filename;
	memcpy(Buffer + count, Filename, strlen(Filename));
	count += strlen(Filename);
	Buffer[count] = 0;
	count++;

	string File = Read_File_Path + (string)Filename;

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
		if ((fpr = fopen(File.c_str(), "w")) == NULL) {
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
		if ((fpr = fopen(File.c_str(), "wb")) == NULL) {
			cout << "\t\t\t\t文件打开失败\n";
			exit(-1);
		}
	}

	return count;
}


void Make_ACK(char* Buffer, int block) {
	memset(Buffer, 0, BUFFER_SIZE * sizeof(char));
	Buffer[0] = 0; Buffer[1] = 4;//操作码为4
	Buffer[2] = ((short)block) >> 8; Buffer[3] = block & 0xff;
}



void Write_Request(void) {
	int re = WSAStartup(0x0202, &wsaData);
	if (re) {   //返回值为0 才初始化Winsock成功
		fprintf(logfile, "错误码：%d    ", WSAGetLastError());
		fprintf(logfile, "winsock初始化错误  ");
		Timeshow();
		exit(-1);
	}

	//定义客户端的地址信息
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 0;   //设置为0  表示自动分配
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//创建socket套接字
	SOCKET client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {   //小于0 套接字创建失败
		fprintf(logfile, "错误码：%d    ", WSAGetLastError());
		fprintf(logfile, "客户端套接字socket创建失败  ");
		Timeshow();
		exit(-1);
	}

	if ((bind(client_fd, (LPSOCKADDR)&client_addr, sizeof(client_addr)))) {
		fprintf(logfile, "错误码：%d    ", WSAGetLastError());
		fprintf(logfile, "服务器端绑定失败  ");
		Timeshow();
		exit(-1);
	}

	long long Sum = 0;
	char Buffer[BUFFER_SIZE];
	memset(Buffer, 0, sizeof(Buffer));
	int count = Make_Write_Request(Buffer);  //得到请求报文的

	long long start = clock();
	//向服务器发送WRQ数据包
	if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		fprintf(logfile, "错误码：%d    ", WSAGetLastError());
		fprintf(logfile, "客户端发送数据失败  ");
		Timeshow();
		exit(-1);
	}

	Sum += count;
	char* srcip = inet_ntoa(client_addr.sin_addr);
	char* dstip = inet_ntoa(server_addr.sin_addr);
	fprintf(logfile, "客户端向服务器发起上传请求报文,报文长度为%d字节,ip.src=%s,srcport=%d,ip.dst=%s,dstport=%d  ", count, srcip, client_addr.sin_port, dstip, server_addr.sin_port);
	Timeshow();
	fprintf(logfile, "请求上传%s文件，传输数据模式为%s  ", Buffer + 2, Buffer + 2 + strlen(Buffer + 2) + 1);
	Timeshow();

	char receive[BUFFER_SIZE];
	memset(receive, 0, sizeof(receive));
	//接收服务器返回的确认信息包
	int server_addr_len = sizeof(server_addr);
	if ((count = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len)) == SOCKET_ERROR) {
		fprintf(logfile, "错误码：%d    ", WSAGetLastError());
		fprintf(logfile, "客户端接收数据失败  ");
		Timeshow();
		exit(-1);
	}
	fprintf(logfile, "客户端接收到服务器的返回包  ");
	Timeshow();

	int block = 0, ack;   //块号标识
	int l;// l 用于记录数据包的大小
	int flag = 0;
	while (1) {
		if (receive[1] == 4) {
			ack = (((receive[2] & 0xff) << 8) + (receive[3] & 0xff)) & 0xffff;
			if (ack == block) {
				Sum += 4;
				fprintf(logfile, "客户端收到服务器的确认报文，ACK=%d  ", ack);
				Timeshow();
				block++;
				block %= 65536;
				l = Make_Data_Package(Buffer, block);
				//向服务器发送数据包
				if (sendto(client_fd, Buffer, l, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					fprintf(logfile, "错误码：%d    ", WSAGetLastError());
					fprintf(logfile, "客户端发送数据失败  ");
					Timeshow();
					exit(-1);
				}

				Sum += l;

				fprintf(logfile, "客户端向服务器上传 %d 块数据，报文大小为 %d 字节,ip.src=%s,srcport=%d,ip.dst=%s,dstport=%d  ", block, l, srcip, client_addr.sin_port, dstip, server_addr.sin_port);
				Timeshow();

				memset(receive, 0, sizeof(receive));
				//接收服务器返回的确认信息包
				if (recvfrom(client_fd, receive, BUFFER_SIZE, 0, (LPSOCKADDR)&server_addr, &server_addr_len) == SOCKET_ERROR) {
					fprintf(logfile, "错误码：%d    ", WSAGetLastError());
					fprintf(logfile, "客户端接收数据失败  ");
					Timeshow();
					exit(-1);
				}

				if (l < 516) break;
			}
		}

		if (receive[1] == 5) {
			fprintf(logfile, "客户端接收到服务器的错误包  "); Timeshow();
			flag = 1;  //错误包直接结束
			switch (receive[3]) {
			case 0: fprintf(logfile, "错误！错误码为：0\t错误信息：%s  ", receive + 4); Timeshow();
				break;
			case 1: fprintf(logfile, "错误！错误码为：1\t错误信息：File not found  "); Timeshow();
				break;
			case 2: fprintf(logfile, "错误！错误码为：2\t错误信息：Access violation  "); Timeshow();
				break;
			case 3: fprintf(logfile, "错误！错误码为：3\t错误信息：Disk full or alloction exceeded  "); Timeshow();
				break;
			case 4: fprintf(logfile, "错误！错误码为：4\n错误信息：Illegal TFTP operation  "); Timeshow();
				break;
			case 5: fprintf(logfile, "错误！错误码为：5\n错误信息：ID Unknown transfer ID\n"); Timeshow();
				break;
			case 6: fprintf(logfile, "错误！错误码为：6\n错误信息：File allready exits\n"); Timeshow();
				break;
			case 7: fprintf(logfile, "错误！错误码为：7\n错误信息：No such user\n"); Timeshow();
				break;
			}
		}
		if (flag) break;
	}
	long long end = clock();

	fprintf(logfile, "数据传输结束  ");
	Timeshow();
	if (!flag) {
		fprintf(logfile, "本次上传文件的平均吞吐量为 %.3f KBps  ", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
		Timeshow();
	}
	closesocket(client_fd);
	fclose(fp);

}

void Read_Request(void) {
	//初始化Winsock
	int re = WSAStartup(0x0202, &wsaData);
	if (re) {
		fprintf(logfile, "winsock初始化错误  ");
		Timeshow();
		exit(-1);
	}

	// 定义客户端的地址信息
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 0;   //设置为0  表示自动分配
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //分配ip

	//创建socket套接字
	SOCKET client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {
		fprintf(logfile, "错误码：%d    ", WSAGetLastError());
		fprintf(logfile, "客户端套接字socket创建失败  ");
		Timeshow();
		exit(-1);
	}

	if ((bind(client_fd, (LPSOCKADDR)&client_addr, sizeof(client_addr)))) {
		fprintf(logfile, "错误码：%d    ", WSAGetLastError());
		fprintf(logfile, "服务器端绑定失败  ");
		Timeshow();
		exit(-1);
	}

	char Buffer[BUFFER_SIZE];
	memset(Buffer, 0, sizeof(Buffer));
	int count = Make_Read_Request(Buffer);
	int l;// l 用于记录数据包的大小
	long long Sum = 0;

	long long start = clock();
	//向服务器发送RRQ数据包
	if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		fprintf(logfile, "错误码：%d    ", WSAGetLastError());
		fprintf(logfile, "客户端发送数据失败  ");
		Timeshow();
		exit(-1);
	}
	Sum += count;

	char* srcip = inet_ntoa(client_addr.sin_addr);
	char* dstip = inet_ntoa(server_addr.sin_addr);
	fprintf(logfile, "客户端向服务器发起下载请求报文,报文长度为%d字节,ip.src=%s,srcport=%d,ip.dst=%s,dstport=%d  ", count, srcip, client_addr.sin_port, dstip, server_addr.sin_port);
	Timeshow();
	fprintf(logfile, "请求下载%s文件，传输数据模式为%s  ", Buffer + 2, Buffer + 2 + strlen(Buffer + 2) + 1);
	Timeshow();

	char receive[BUFFER_SIZE];
	memset(receive, 0, sizeof(receive));
	//接收服务器返回的确认信息包
	int server_addr_len = sizeof(server_addr);
	if ((l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len)) == SOCKET_ERROR) {
		fprintf(logfile, "错误码：%d    ", WSAGetLastError());
		fprintf(logfile, "客户端接收数据失败  ");
		Timeshow();
		exit(-1);
	}

	fprintf(logfile, "客户端接收到服务器的返回包  ");
	Timeshow();


	int ack = 1, block;   //块号标识
	int flag = 0;
	while (1) {
		if (receive[1] == 3) {

			Sum += 4 + (long long)l;
			block = (((receive[2] & 0xff) << 8) + (receive[3] & 0xff)) & 0xffff;
			ack = (ack % 65536);

			if (ack == block) {
				fwrite(receive + 4, 1, l, fpr);
				fprintf(logfile, "客户端收到服务器发送的第 %d 块数据报文，报文大小为 %d 字节，ip.src=%s,srcport=%d,ip.dst=%s,dstport=%d  ", block, l, srcip, client_addr.sin_port, dstip, server_addr.sin_port);
				Timeshow();
				Sum += l;
				Make_ACK(Buffer, block);	//编写确认报文
				//向服务器发送数据包
				if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					fprintf(logfile, "错误码：%d    ", WSAGetLastError());
					fprintf(logfile, "客户端发送数据失败  ");
					Timeshow();
					exit(-1);
				}

				Sum += 4;
				fprintf(logfile, "客户端向服务器发送确认报文，ACK= %d   ", ack);
				Timeshow();

				if (l < 512) break;

				memset(receive, 0, sizeof(receive));
				//接收服务器返回的确认信息包
				if ((l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (LPSOCKADDR)&server_addr, &server_addr_len)) == SOCKET_ERROR) {
					fprintf(logfile, "错误码：%d    ", WSAGetLastError());
					fprintf(logfile, "客户端接收数据失败  ");
					Timeshow();
					exit(-1);
				}

			}
			ack++;
		}

		if (receive[1] == 5) {
			flag = 1;
			switch (receive[3]) {
			case 0: fprintf(logfile, "错误！错误码为：0\t错误信息：%s  ", receive + 4); Timeshow();
				break;
			case 1: fprintf(logfile, "错误！错误码为：1\t错误信息：File not found  "); Timeshow();
				break;
			case 2: fprintf(logfile, "错误！错误码为：2\t错误信息：Access violation  "); Timeshow();
				break;
			case 3: fprintf(logfile, "错误！错误码为：3\t错误信息：Disk full or alloction exceeded  "); Timeshow();
				break;
			case 4: fprintf(logfile, "错误！错误码为：4\n错误信息：Illegal TFTP operation  "); Timeshow();
				break;
			case 5: fprintf(logfile, "错误！错误码为：5\n错误信息：ID Unknown transfer ID\n"); Timeshow();
				break;
			case 6: fprintf(logfile, "错误！错误码为：6\n错误信息：File allready exits\n"); Timeshow();
				break;
			case 7: fprintf(logfile, "错误！错误码为：7\n错误信息：No such user\n"); Timeshow();
				break;
			}
		}
		if (flag) break;

	}
	long long end = clock();
	fprintf(logfile, "数据传输结束  ");
	Timeshow();
	if (!flag) {
		fprintf(logfile, "本次下载文件的平均吞吐量为 %.3f KBps  ", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
		Timeshow();
	}

	closesocket(client_fd);
	fclose(fpr);
}


int main() {
	string a;
	logfile = fopen(Log_File, "a");
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = inet_addr("10.12.168.33");
	while (1)
	{
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
	}
	WSACleanup();
	return 0;
}
