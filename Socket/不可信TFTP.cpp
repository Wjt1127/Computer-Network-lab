#define _CRT_SECURE_NO_WARNINGS
#include<winsock.h>   //����Winsock 2,Ӧ��#include<winsock2.h>
#include<iostream>
#include<stdio.h>
#include<string.h>
#include<ctime>
#pragma comment(lib,"ws2_32.lib")  //����ws2_32.lib ����������Ӵ���
using namespace std;

#define SERVER_PORT 69   //����ͻ��˳�ʼ���ӣ�α���ӣ��˿ں�
#define BUFFER_SIZE 1024 //���û������Ĵ�С
#define File_Size  100   //�����ļ�������󳤶�
#define Time_Out 30      //���ó�ʱ�ش���ʱ��(us)
#define Max_Try  4
const char* Write_File_Path = "C:\\Users\\12432\\Desktop\\����ʵ��\\Socket\\Tftpd64\\�ϴ��ļ�\\";   //����д����ʱ д�ļ���Դ�ļ�·��
const char* Read_File_Path = "C:\\Users\\12432\\Desktop\\����ʵ��\\Socket\\Tftpd64\\�����ļ�\\";   //���ö�����ʱ ���ļ����ļ�·��
const char* Log_File = "C:\\Users\\12432\\Desktop\\����ʵ��\\Socket\\Tftpd64\\��־�ļ�.log";   //������־�ļ�·��
WSADATA wsaData;  //Ϊwinsock�ĳ�ʼ��ʹ��
struct sockaddr_in server_addr;   //���÷������˵ĵ�ַ��Ϣ ע��ÿ�ν���������ǰ��port��Ϊ69�Ŷ˿�
FILE* fp, * fpr;   //�����ļ���д��  ��Ϊȫ�ֱ�����Ϊ�˷���ʵ�ִ������
FILE* logfile;  //��־�ļ���д����  ��������Źر�log
time_t rawtime;   //Ϊ������־�ļ�����ʾʱ�����õ�
struct tm* timeinfo;
string temp;

//�����������ɫ
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

//ʵ��΢�뼶˯��
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
��������Write(char *receive,int l)
���룺���յ������� �����ݴ�С
���أ���
���ܣ�ʵ��linux�汾���������ļ�д
*/
void Write(char* receive, int l) {
	for (int i = 4; i < l; i++) {
		fwrite(&receive[i], sizeof(char), 1, fpr);
	}
}


/*
��������Make_Write_Request
���룺char *Buffer  �洢�����������
���أ�д�����ĵ��ֽڴ�С
���ܣ�����һ��д�����������Buffer
*/
int Make_Write_Request(char* Buffer) {
	int count = 0;
	memset(Buffer, 0, sizeof(Buffer));
	char Filename[File_Size];
	memset(Filename, 0, sizeof(Filename));
	Buffer[0] = 0; Buffer[1] = 2;		//����OpcodeΪ2
	count += 2;

	cout << "\t\t\t\t�����������������д����ļ���:";
	cin >> Filename;
	memcpy(Buffer + count, Filename, strlen(Filename));  //���ļ������ڲ������
	count += strlen(Filename);
	Buffer[count] = 0;   //�ļ������  0 ��ʾ�ļ�������
	count++;

	string File = Write_File_Path + (string)Filename;   //�γ�д�ļ���·��
	temp = Filename;

	int mod;   //ģʽ��ѡ��
	cout << "\t\t\t\t��ѡ������д�����ݵ����ͣ���0��ASCII����ʽ\t��1����������ʽ\n";
	cout << "\t\t\t\t\t";
	cin >> mod;
	if (mod == 0) {
		Buffer[count] = 'n'; Buffer[count + 1] = 'e';
		Buffer[count + 2] = 't'; Buffer[count + 3] = 'a';
		Buffer[count + 4] = 's'; Buffer[count + 5] = 'c';
		Buffer[count + 6] = 'i'; Buffer[count + 7] = 'i';
		Buffer[count + 8] = 0;
		count += 9;
		if ((fp = fopen(File.c_str(), "r+b")) == NULL) {   //���ʱѡ��ASCIIģʽ��ôʹ�� r ģʽ���ļ�
			cout << "\t\t\t\t�ļ���ʧ��\n";
			return 0;
		}
	}
	else {
		Buffer[count] = 'o'; Buffer[count + 1] = 'c';
		Buffer[count + 2] = 't'; Buffer[count + 3] = 'e';
		Buffer[count + 4] = 't';
		Buffer[count + 5] = 0;
		count += 6;
		if ((fp = fopen(File.c_str(), "rb")) == NULL) {   //���ʱѡ�������ģʽ��ôʹ�� rb ģʽ���ļ�
			cout << "\t\t\t\t�ļ���ʧ��\n";
			return 0;
		}
	}
	return count;   //���������ĵ��ֽڴ�С
}


/*
��������Make_Data_Package
���룺char* Buffer  һ���ַ�����   short Block_Num  ����Ŀ��
���أ����ش����Buffer�Ĵ�С
���ܣ��������Buffer�������������
*/
int Make_Data_Package(char* Buffer, int Block_Num) {

	memset(Buffer, 0, sizeof(Buffer));
	Buffer[0] = 0; Buffer[1] = 3;   //����Opcode Ϊ3
	Buffer[2] = (short)Block_Num >> 8; Buffer[3] = Block_Num & 0xff;
	int length = fread(Buffer + 4, 1, 512, fp);
	return (4 + length);   //����Buffer�ĳ���
}

/*
��������Make_Read_Request
���룺char *Buffer  ���ڴ�������������
���أ���������ֽڴ�С
���ܣ����������װ�� ����Buffer������
*/
int Make_Read_Request(char* Buffer) {
	int count = 0;
	memset(Buffer, 0, sizeof(Buffer));
	char Filename[File_Size];
	memset(Filename, 0, sizeof(Filename));
	Buffer[0] = 0; Buffer[1] = 1;		//����OpcodeΪ1
	count += 2;

	cout << "\t\t\t\t����������ӷ����������ص��ļ���:";
	cin >> Filename;
	memcpy(Buffer + count, Filename, strlen(Filename));  //���ļ������ڲ������
	count += strlen(Filename);
	Buffer[count] = 0;   //�ļ������  0 ��ʾ�ļ�������
	count++;

	string File = Read_File_Path + (string)Filename;   //�γ�д�ļ���·��

	int mod;   //ģʽ��ѡ��
	cout << "\t\t\t\t��ѡ������д�����ݵ����ͣ���0��ASCII����ʽ\t��1����������ʽ\n";
	cout << "\t\t\t\t";
	cin >> mod;
	if (mod == 0) {
		Buffer[count] = 'n'; Buffer[count + 1] = 'e';
		Buffer[count + 2] = 't'; Buffer[count + 3] = 'a';
		Buffer[count + 4] = 's'; Buffer[count + 5] = 'c';
		Buffer[count + 6] = 'i'; Buffer[count + 7] = 'i';
		Buffer[count + 8] = 0;
		count += 9;
		if ((fpr = fopen(File.c_str(), "w+b")) == NULL) {   //���ʱѡ��ASCII��ģʽ��ôʹ�� w ģʽд�ļ�
			cout << "\t\t\t\t�ļ���ʧ��\n";
			exit(-1);
		}
	}
	else {
		Buffer[count] = 'o'; Buffer[count + 1] = 'c';
		Buffer[count + 2] = 't'; Buffer[count + 3] = 'e';
		Buffer[count + 4] = 't';
		Buffer[count + 5] = 0;
		count += 6;
		if ((fpr = fopen(File.c_str(), "wb")) == NULL) {   //���ʱѡ�������ģʽ��ôʹ�� wb ģʽ���ļ�
			cout << "\t\t\t\t�ļ���ʧ��\n";
			exit(-1);
		}
	}

	return count;   //���������ĵ��ֽڴ�С
}

/*
��������Make_ACK
���룺char *Buffer  ���ڴ洢ACK���ĵ�����  int block ���
���أ���
���ܣ���ACK�༭�ô����Buffer��
*/
void Make_ACK(char* Buffer, int block) {
	memset(Buffer, 0, BUFFER_SIZE * sizeof(char));
	Buffer[0] = 0; Buffer[1] = 4;//������Ϊ4
	Buffer[2] = ((short)block) >> 8; Buffer[3] = block & 0xff;  //����ȷ�Ͽ��
}


/*
��������Write_Request(void)
��������
����ֵ:��
���ܣ������������д�ļ����� ͬʱ�������ϴ���������
*/
void Write_Request(void) {

	//����ͻ��˵ĵ�ַ��Ϣ
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 0;   //����Ϊ0  ��ʾ�Զ�����
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //����ip���Ǳ��ص�ַ

	//����socket�׽���
	SOCKET client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {   //С��0 �׽��ִ���ʧ��
		printf("�����룺%d    ", WSAGetLastError());
		printf("�ͻ����׽���socket����ʧ��\n");
		exit(-1);
	}

	//������I/O����Ϊ������
	u_long mode = 1;
	ioctlsocket(client_fd, FIONBIO, &mode);

	//���׽��ֵ�ַ��Ϣ  ���Ҫ������Ϣһ��Ҫʹ��bind �󶨵�ַ��Ϣ ��������sendto()����Ϣ ���ܱ�����
	if ((bind(client_fd, (LPSOCKADDR)&client_addr, sizeof(client_addr)))) {   //ת����LPSOCKADDR����
		printf("�����룺%d    ", WSAGetLastError());
		printf("�������˰�ʧ��\n");
		exit(-1);
	}

	long long Sum = 0;
	char Buffer[BUFFER_SIZE];
	memset(Buffer, 0, sizeof(Buffer));
	int count = Make_Write_Request(Buffer);  //�õ������ĵ�
	int Request_flag = 0;   //����һ��������flag  �ж��������Ƿ�
	long long start = clock();
	//�����������WRQ���ݰ�
	//***!!!!   sendto�����ĵڶ�������һ��Ҫ���������ĵ�ʵ���ֽڴ�С
	if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		printf("�����룺%d    ", WSAGetLastError());
		printf("�ͻ��˷�������ʧ��\n");
		exit(-1);
	}

	Sum += count;
	char* srcip = inet_ntoa(client_addr.sin_addr);
	char* dstip = inet_ntoa(server_addr.sin_addr);
	fprintf(logfile, "�����ϴ�������,���ĳ���Ϊ%d�ֽ�,ip.src=%s,ip.dst=%s\t", count, srcip, dstip);
	printf("����������\n");
	Timeshow();
	fprintf(logfile, "�����ϴ�%s�ļ�����������ģʽΪ%s\t", Buffer + 2, Buffer + 2 + strlen(Buffer + 2) + 1);
	printf( "��������ģʽΪ%s\n",Buffer + 2 + strlen(Buffer + 2) + 1);
	Timeshow();

	char receive[BUFFER_SIZE];
	memset(receive, 0, sizeof(receive));
	//���շ��������ص�ȷ����Ϣ��
	int server_addr_len = sizeof(server_addr);

	int l;  //l���ڼ�¼���͵����ݰ��Ĵ�С
	int e = 0;
	while(1){
		e++;
		l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
		if (l > 0) {   //����ֵ����0  ��˵�����ճɹ�
			Sum += l;
			break;
		}
		MSleep(Time_Out);
		if (e > 10) {
			fprintf(logfile, "�ϴ�����ʱ�������ش�\t");
			printf("�ϴ�����ʱ���ش�������\n");
			e = 0;
			if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
				printf("�����룺%d    ", WSAGetLastError());
				printf("�ͻ��˷�������ʧ��\n");
				exit(-1);
			}
			Sum += count;
			Sleep(10);
		}
	}

	int block = 0, ack;   //��ű�ʶ
	int flag = 0;
	while (1) {
		//countΪ-1��ʾ ��ʱACK  ��ô�ش����ݰ�
		if (l == -1) {
			/*{
				if (block == 0) {
					MSleep(2000);
				}
			}*/
			//����������·������ݰ�
			Timeshow();
			fprintf(logfile, "\t���շ��������ر��ĳ�ʱ,�ش����ݱ���,Block=%d\n", block);
			printf("���շ��������ر��ĳ�ʱ,�ش����ݱ���,Block=%d\n", block);
			
			//***!!!!   sendto�����ĵڶ�������һ��Ҫ���������ĵ�ʵ���ֽڴ�С
			if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
				printf("�����룺%d    ", WSAGetLastError());
				printf("�ͻ��˷�������ʧ��\n");
				exit(-1);
			}
			Sum += count;
			while(1) {
				l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
				if (l > 0) {   //����ֵ����0  ��˵�����ճɹ�
					Sum += l;
					break;
				}
				MSleep(Time_Out);
			}
		}
		else {  //���յ��˿ͻ��˵�ȷ�ϱ���
			if (receive[1] == 4) {
				ack = (((receive[2] & 0xff) << 8) + (receive[3] & 0xff)) & 0xffff;
				if (ack == block) {   //�յ���ȷ�ϱ���ȷ�Ϻźͷ��͵����ݰ��Ŀ���ͬ
					block++;
					Sum += 4;
					Timeshow();
					fprintf(logfile, "\t�ͻ����յ���������ȷ�ϱ��ģ�ACK=%d\n", ack);
					printf("�յ�ȷ�ϱ��ģ�ACK=%d\n", ack);
					block %= 65536;
					count = Make_Data_Package(Buffer, block);	//�������ݰ��Ĵ�С 
					//��������������ݰ�
					if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
						printf("�����룺%d    ", WSAGetLastError());
						printf("�ͻ��˷�������ʧ��\n");
						exit(-1);
					}

					Sum += count;
					Timeshow();
					fprintf(logfile, "\t�ϴ� %d �����ݣ����Ĵ�СΪ %d �ֽ�,ip.src=%s,ip.dst=%s\n", block, count, srcip, dstip);
					printf("�ϴ� %d ������\n", block);

					memset(receive, 0, sizeof(receive));
					//���շ��������ص�ȷ����Ϣ��
					while(1) {
						l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
						if (l > 0) {   //����ֵ����0  ��˵�����ճɹ�
							Sum += l;
							break;
							MSleep(Time_Out);
						}
					}

					if (count < 516 && l) break;   // l>0˵���ɹ���÷��ر���  count<516��˵���ļ�д���� ����ֻҪ�յ���ȷ����Ϣ�ͽ���
				}
				else {  //���������ص�ACK���Ų���ͬ
					//����������·������ݰ�
					Timeshow();
					fprintf(logfile, "\t���շ��������ص�ȷ�ϱ��Ĵ���,ACK=%d,�ش����ݱ���,Block=%d\n", ack, block);
					printf("���շ��������ص�ȷ�ϱ��Ĵ���,ACK=%d,�ش����ݱ���,Block=%d\n", ack, block);
					//***!!!!   sendto�����ĵڶ�������һ��Ҫ���������ĵ�ʵ���ֽڴ�С
					if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
						printf("�����룺%d    ", WSAGetLastError());
						printf("�ͻ��˷�������ʧ��  ");
						exit(-1);
					}
					Sum += count;

					while(1) {
						l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
						if (l > 0) {   //����ֵ����0  ��˵�����ճɹ�
							Sum += l;
							break;
						}
						MSleep(Time_Out);
					}
				}
			}

			if (receive[1] == 5) {
				Timeshow();fprintf(logfile, "\t�ͻ��˽��յ��������Ĵ����\n"); 
				printf("�ͻ��˽��յ��������Ĵ����\n");
				flag = 1;  //�����ֱ�ӽ���
				switch (receive[3]) {
				case 0:  Timeshow();fprintf(logfile, "\t���󣡴�����Ϊ��0\t������Ϣ��%s\n", receive + 4);
					break;
				case 1: Timeshow();fprintf(logfile, "\t���󣡴�����Ϊ��1\t������Ϣ��File not found\n"); 
					break;
				case 2: Timeshow();fprintf(logfile, "\t���󣡴�����Ϊ��2\t������Ϣ��Access violation\n"); 
					break;
				case 3: Timeshow();fprintf(logfile, "\t���󣡴�����Ϊ��3\t������Ϣ��Disk full or alloction exceeded\n"); 
					break;
				case 4:  Timeshow();fprintf(logfile, "\t���󣡴�����Ϊ��4\n������Ϣ��Illegal TFTP operation\n");
					break;
				case 5:  Timeshow();fprintf(logfile, "\t���󣡴�����Ϊ��5\n������Ϣ��ID Unknown transfer ID\n");
					break;
				case 6:  Timeshow();fprintf(logfile, "\t���󣡴�����Ϊ��6\n������Ϣ��File allready exits\n");
					break;
				case 7:  Timeshow();fprintf(logfile, "\t���󣡴�����Ϊ��7\n");
					break;
				}
			}
			if (flag) break;
		}
	}
	long long end = clock();
	Timeshow();
	fprintf(logfile, "\t���ݴ������\n");
	printf("���ݴ������\n");

	if (!flag) {		
		Timeshow();
		fprintf(logfile, "\t�����ϴ��ļ���ƽ��������Ϊ %.3f KBps\n", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
		SetConsoleColor(FOREGROUND_RED);
		printf("\t\t\t\t���������ļ���ƽ��������Ϊ %.3f KBps  \n", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
		SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	}
	closesocket(client_fd);
	fclose(fp);

}
/*
��������Read_Request
���룺char *Buffervoid
���أ���
���ܣ�ʵ��
*/
void Read_Request(void) {

	// ����ͻ��˵ĵ�ַ��Ϣ
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 0;   //����Ϊ0  ��ʾ�Զ�����
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //����ip

	//����socket�׽���
	SOCKET client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {   //С��0 �׽��ִ���ʧ��
		printf("\t�����룺%d    ", WSAGetLastError());
		printf("�ͻ����׽���socket����ʧ��\n");

		exit(-1);
	}

	//������I/O����Ϊ������
	u_long mode = 1;
	ioctlsocket(client_fd, FIONBIO, &mode);

	//���׽��ֵ�ַ��Ϣ  ���Ҫ������Ϣһ��Ҫʹ��bind �󶨵�ַ��Ϣ ��������sendto()����Ϣ ���ܱ�����
	if ((bind(client_fd, (LPSOCKADDR)&client_addr, sizeof(client_addr)))) {   //ת����LPSOCKADDR����
		printf("�����룺%d    ", WSAGetLastError());
		printf("�������˰�ʧ��\n");
		exit(-1);
	}

	char Buffer[BUFFER_SIZE];
	memset(Buffer, 0, sizeof(Buffer));
	int count = Make_Read_Request(Buffer);  //�õ������ĵ��ֽ�����С ���������������Ĵ���Buffer��
	int l;   // l ���ڼ�¼���ݰ��Ĵ�С
	long long Sum = 0;   //Sum�Ǽ����������������ݴ�С

	long long start = clock();   //�Ӵ��������Ŀ�ʼ����ʱ��
	//�����������RRQ���ݰ�
	//***!!!!   sendto�����ĵڶ�������һ��Ҫ���������ĵ�ʵ���ֽڴ�С
	if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		printf("\t�����룺%d    ", WSAGetLastError());
		printf("�ͻ��˷�������ʧ��\n");
		exit(-1);
	}
	Sum += count;

	char* srcip = inet_ntoa(client_addr.sin_addr);
	char* dstip = inet_ntoa(server_addr.sin_addr);
	Timeshow();
	fprintf(logfile, "\t��������������,���ĳ���Ϊ%d�ֽ�,ip.src=%s,ip.dst=%s\n", count, srcip, dstip);
	printf("��������������,���ĳ���Ϊ%d�ֽ�,ip.src=%s,ip.dst=%s\n", count, srcip, dstip);
	Timeshow();
	fprintf(logfile, "\t��������%s�ļ�����������ģʽΪ%s\n", Buffer + 2, Buffer + 2 + strlen(Buffer + 2) + 1);
	printf("��������%s�ļ�����������ģʽΪ%s\n", Buffer + 2, Buffer + 2 + strlen(Buffer + 2) + 1);
	
	int C = 0;
	char receive[BUFFER_SIZE];
	memset(receive, 0, sizeof(receive));
	//���շ��������ص�ȷ����Ϣ��
	int server_addr_len = sizeof(server_addr);
	int b = 0;
	while (1) {
		b++;
		memset(receive, 0, sizeof(receive));
		l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
		if (l > 0) {   //����ֵ����0  ��˵�����ճɹ�
			Sum += l;
			break;
		}
		Sleep(100);
		if (b > 10) {
			Timeshow();
			fprintf(logfile, "\t���г�ʱ�ش�����������\n");
			printf("���г�ʱ�ش�����������\n");
			
			if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
				printf( "�����룺%d    ", WSAGetLastError());
				printf( "�ͻ��˷�������ʧ��\n");
				exit(-1);
			}
			Sum += count;
			b = 0;
		}
	}
	C = 0;

	//�жϴ�ʱ�յ�����ȷ�ϰ�  ���Ǵ����
	int block;   //��ű�ʶ
	int flag = 0;
	while (1) {
		if (receive[1] == 3) {
			block = (((receive[2] & 0xff) << 8) + (receive[3] & 0xff)) & 0xffff;
			Timeshow();
			fprintf(logfile, "\t���յ���%d�����ݰ�\n", block);
			printf("���յ���%d�����ݰ�\n", block);
			Write(receive, l);
			Make_ACK(Buffer, block);
			int count = 0;
			if (l < 516) {
				if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					printf( "�����룺%d    ", WSAGetLastError());
					printf( "�ͻ��˷�������ʧ��\n");
					exit(-1);
				}
				Sleep(5);
				if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					printf("�����룺%d    ", WSAGetLastError());
					printf("�ͻ��˷�������ʧ��\n");
					exit(-1);
				}
				Sleep(5);
				if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					printf("�����룺%d    ", WSAGetLastError());
					printf("�ͻ��˷�������ʧ��\n");
					exit(-1);
				}
				Sleep(5);
				Sum += 12;
				break;
			}
			else {
				if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					printf("�����룺%d    ", WSAGetLastError());
					printf("�ͻ��˷�������ʧ��\n");
					exit(-1);
				}
			}
			Sum += 4;
			while (1) {
				b++;
				l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len);
				if (l > 0) {   //����ֵ����0  ��˵�����ճɹ�
					Sum += l;
					break;
				}
				Sleep(100);
				if (b > 10) {
					Timeshow();
					fprintf(logfile, "\t���г�ʱ�ش�,���·���%d��ACK����\n", block);
					printf("���г�ʱ�ش�,���·���%d��ACK����\n", block);
					
					if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
						printf("�����룺%d    ", WSAGetLastError());
						printf("�ͻ��˷�������ʧ��\n");
						exit(-1);
					}
					Sum += 4;
					b = 0;
				}
			}
		}

		if (receive[1] == 5) {
			Timeshow(); fprintf(logfile, "\t�ͻ��˽��յ��������Ĵ����\n");
			printf("�ͻ��˽��յ��������Ĵ����\n");
			flag = 1;  //�����ֱ�ӽ���
			switch (receive[3]) {
			case 0:  Timeshow(); fprintf(logfile, "\t���󣡴�����Ϊ��0\t������Ϣ��%s\n", receive + 4);
				break;
			case 1: Timeshow(); fprintf(logfile, "\t���󣡴�����Ϊ��1\t������Ϣ��File not found\n");
				break;
			case 2: Timeshow(); fprintf(logfile, "\t���󣡴�����Ϊ��2\t������Ϣ��Access violation\n");
				break;
			case 3: Timeshow(); fprintf(logfile, "\t���󣡴�����Ϊ��3\t������Ϣ��Disk full or alloction exceeded\n");
				break;
			case 4:  Timeshow(); fprintf(logfile, "\t���󣡴�����Ϊ��4\n������Ϣ��Illegal TFTP operation\n");
				break;
			case 5:  Timeshow(); fprintf(logfile, "\t���󣡴�����Ϊ��5\n������Ϣ��ID Unknown transfer ID\n");
				break;
			case 6:  Timeshow(); fprintf(logfile, "\t���󣡴�����Ϊ��6\n������Ϣ��File allready exits\n");
				break;
			case 7:  Timeshow(); fprintf(logfile, "\t���󣡴�����Ϊ��7\n");
				break;
			}
		}
		if (flag) break;
	}
	long long end = clock();
	Timeshow();
	fprintf(logfile, "\t���ݴ������\n");
	if (!flag) {
		fprintf(logfile, "���������ļ���ƽ��������Ϊ %.3f KBps  ", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
		SetConsoleColor(FOREGROUND_RED);
		printf("\t\t\t\t���������ļ���ƽ��������Ϊ %.3f KBps  \n", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
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
		//��ʼ��Winsock
		int re = WSAStartup(0x0202, &wsaData);
		if (re) {   //����ֵΪ0 �ų�ʼ��Winsock�ɹ�
			printf("�����룺%d    ", WSAGetLastError());
			printf("winsock��ʼ������\n");
			exit(-1);
		}
		system("cls");
		server_addr.sin_port = htons(SERVER_PORT);
		int op;
		cout << "\n\n\n\n\n\n\n";
		printf("\t\t\t\t*-------------------------------------*\n");
		printf("\t\t\t\t*\tѡ����Ҫ���еĲ������:       *\n");
		printf("\t\t\t\t*\t1: �ӷ����������ļ�           *\n");
		printf("\t\t\t\t*\t2: ��������ϴ��ļ�           *\n");
		printf("\t\t\t\t*\t0: �˳�                       *\n");
		printf("\t\t\t\t*-------------------------------------*\n");
		cout << "\t\t\t\t����ѡ��";
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