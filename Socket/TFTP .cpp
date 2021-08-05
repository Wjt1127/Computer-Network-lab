#define _CRT_SECURE_NO_WARNINGS
#include<winsock.h>   
#include<iostream>
#include<stdio.h>
#include<string.h>
#include<ctime>
#pragma comment(lib,"ws2_32.lib")  
using namespace std;

#define SERVER_PORT 69   //����ͻ��˳�ʼ���Ӷ˿ں�
#define BUFFER_SIZE 1024 
#define File_Size  100   
#define Time_Out 30      

const char* Write_File_Path = "C:\\Users\\12432\\Desktop\\����ʵ��\\Socket\\Tftpd64\\�ϴ��ļ�\\";
const char* Read_File_Path = "C:\\Users\\12432\\Desktop\\����ʵ��\\Socket\\Tftpd64\\�����ļ�\\";
const char* Log_File = "C:\\Users\\12432\\Desktop\\����ʵ��\\Socket\\Tftpd64\\��־�ļ�.log";
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

	cout << "\t\t\t\t�����������������д����ļ���:";
	cin >> Filename;
	memcpy(Buffer + count, Filename, strlen(Filename));
	count += strlen(Filename);
	Buffer[count] = 0;
	count++;

	string File = Write_File_Path + (string)Filename;

	int mod;
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
		if ((fp = fopen(File.c_str(), "rb")) == NULL) {
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
		if ((fp = fopen(File.c_str(), "rb")) == NULL) {
			cout << "\t\t\t\t�ļ���ʧ��\n";
			exit(-1);
		}
	}
	return count;
}


int Make_Data_Package(char* Buffer, int Block_Num) {

	memset(Buffer, 0, sizeof(Buffer));
	Buffer[0] = 0; Buffer[1] = 3;   //����Opcode Ϊ3
	Buffer[2] = (short)Block_Num >> 8; Buffer[3] = Block_Num & 0xff;
	int length = fread(Buffer + 4, 1, 512, fp);
	return (4 + length);   //����Buffer�ĳ���
}


int Make_Read_Request(char* Buffer) {
	int count = 0;
	memset(Buffer, 0, sizeof(Buffer));
	char Filename[File_Size];
	memset(Filename, 0, sizeof(Filename));
	Buffer[0] = 0; Buffer[1] = 1;		//����OpcodeΪ1
	count += 2;

	cout << "\t\t\t\t����������ӷ����������ص��ļ���:";
	cin >> Filename;
	memcpy(Buffer + count, Filename, strlen(Filename));
	count += strlen(Filename);
	Buffer[count] = 0;
	count++;

	string File = Read_File_Path + (string)Filename;

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
		if ((fpr = fopen(File.c_str(), "w")) == NULL) {
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
		if ((fpr = fopen(File.c_str(), "wb")) == NULL) {
			cout << "\t\t\t\t�ļ���ʧ��\n";
			exit(-1);
		}
	}

	return count;
}


void Make_ACK(char* Buffer, int block) {
	memset(Buffer, 0, BUFFER_SIZE * sizeof(char));
	Buffer[0] = 0; Buffer[1] = 4;//������Ϊ4
	Buffer[2] = ((short)block) >> 8; Buffer[3] = block & 0xff;
}



void Write_Request(void) {
	int re = WSAStartup(0x0202, &wsaData);
	if (re) {   //����ֵΪ0 �ų�ʼ��Winsock�ɹ�
		fprintf(logfile, "�����룺%d    ", WSAGetLastError());
		fprintf(logfile, "winsock��ʼ������  ");
		Timeshow();
		exit(-1);
	}

	//����ͻ��˵ĵ�ַ��Ϣ
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 0;   //����Ϊ0  ��ʾ�Զ�����
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//����socket�׽���
	SOCKET client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {   //С��0 �׽��ִ���ʧ��
		fprintf(logfile, "�����룺%d    ", WSAGetLastError());
		fprintf(logfile, "�ͻ����׽���socket����ʧ��  ");
		Timeshow();
		exit(-1);
	}

	if ((bind(client_fd, (LPSOCKADDR)&client_addr, sizeof(client_addr)))) {
		fprintf(logfile, "�����룺%d    ", WSAGetLastError());
		fprintf(logfile, "�������˰�ʧ��  ");
		Timeshow();
		exit(-1);
	}

	long long Sum = 0;
	char Buffer[BUFFER_SIZE];
	memset(Buffer, 0, sizeof(Buffer));
	int count = Make_Write_Request(Buffer);  //�õ������ĵ�

	long long start = clock();
	//�����������WRQ���ݰ�
	if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		fprintf(logfile, "�����룺%d    ", WSAGetLastError());
		fprintf(logfile, "�ͻ��˷�������ʧ��  ");
		Timeshow();
		exit(-1);
	}

	Sum += count;
	char* srcip = inet_ntoa(client_addr.sin_addr);
	char* dstip = inet_ntoa(server_addr.sin_addr);
	fprintf(logfile, "�ͻ���������������ϴ�������,���ĳ���Ϊ%d�ֽ�,ip.src=%s,srcport=%d,ip.dst=%s,dstport=%d  ", count, srcip, client_addr.sin_port, dstip, server_addr.sin_port);
	Timeshow();
	fprintf(logfile, "�����ϴ�%s�ļ�����������ģʽΪ%s  ", Buffer + 2, Buffer + 2 + strlen(Buffer + 2) + 1);
	Timeshow();

	char receive[BUFFER_SIZE];
	memset(receive, 0, sizeof(receive));
	//���շ��������ص�ȷ����Ϣ��
	int server_addr_len = sizeof(server_addr);
	if ((count = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len)) == SOCKET_ERROR) {
		fprintf(logfile, "�����룺%d    ", WSAGetLastError());
		fprintf(logfile, "�ͻ��˽�������ʧ��  ");
		Timeshow();
		exit(-1);
	}
	fprintf(logfile, "�ͻ��˽��յ��������ķ��ذ�  ");
	Timeshow();

	int block = 0, ack;   //��ű�ʶ
	int l;// l ���ڼ�¼���ݰ��Ĵ�С
	int flag = 0;
	while (1) {
		if (receive[1] == 4) {
			ack = (((receive[2] & 0xff) << 8) + (receive[3] & 0xff)) & 0xffff;
			if (ack == block) {
				Sum += 4;
				fprintf(logfile, "�ͻ����յ���������ȷ�ϱ��ģ�ACK=%d  ", ack);
				Timeshow();
				block++;
				block %= 65536;
				l = Make_Data_Package(Buffer, block);
				//��������������ݰ�
				if (sendto(client_fd, Buffer, l, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					fprintf(logfile, "�����룺%d    ", WSAGetLastError());
					fprintf(logfile, "�ͻ��˷�������ʧ��  ");
					Timeshow();
					exit(-1);
				}

				Sum += l;

				fprintf(logfile, "�ͻ�����������ϴ� %d �����ݣ����Ĵ�СΪ %d �ֽ�,ip.src=%s,srcport=%d,ip.dst=%s,dstport=%d  ", block, l, srcip, client_addr.sin_port, dstip, server_addr.sin_port);
				Timeshow();

				memset(receive, 0, sizeof(receive));
				//���շ��������ص�ȷ����Ϣ��
				if (recvfrom(client_fd, receive, BUFFER_SIZE, 0, (LPSOCKADDR)&server_addr, &server_addr_len) == SOCKET_ERROR) {
					fprintf(logfile, "�����룺%d    ", WSAGetLastError());
					fprintf(logfile, "�ͻ��˽�������ʧ��  ");
					Timeshow();
					exit(-1);
				}

				if (l < 516) break;
			}
		}

		if (receive[1] == 5) {
			fprintf(logfile, "�ͻ��˽��յ��������Ĵ����  "); Timeshow();
			flag = 1;  //�����ֱ�ӽ���
			switch (receive[3]) {
			case 0: fprintf(logfile, "���󣡴�����Ϊ��0\t������Ϣ��%s  ", receive + 4); Timeshow();
				break;
			case 1: fprintf(logfile, "���󣡴�����Ϊ��1\t������Ϣ��File not found  "); Timeshow();
				break;
			case 2: fprintf(logfile, "���󣡴�����Ϊ��2\t������Ϣ��Access violation  "); Timeshow();
				break;
			case 3: fprintf(logfile, "���󣡴�����Ϊ��3\t������Ϣ��Disk full or alloction exceeded  "); Timeshow();
				break;
			case 4: fprintf(logfile, "���󣡴�����Ϊ��4\n������Ϣ��Illegal TFTP operation  "); Timeshow();
				break;
			case 5: fprintf(logfile, "���󣡴�����Ϊ��5\n������Ϣ��ID Unknown transfer ID\n"); Timeshow();
				break;
			case 6: fprintf(logfile, "���󣡴�����Ϊ��6\n������Ϣ��File allready exits\n"); Timeshow();
				break;
			case 7: fprintf(logfile, "���󣡴�����Ϊ��7\n������Ϣ��No such user\n"); Timeshow();
				break;
			}
		}
		if (flag) break;
	}
	long long end = clock();

	fprintf(logfile, "���ݴ������  ");
	Timeshow();
	if (!flag) {
		fprintf(logfile, "�����ϴ��ļ���ƽ��������Ϊ %.3f KBps  ", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
		Timeshow();
	}
	closesocket(client_fd);
	fclose(fp);

}

void Read_Request(void) {
	//��ʼ��Winsock
	int re = WSAStartup(0x0202, &wsaData);
	if (re) {
		fprintf(logfile, "winsock��ʼ������  ");
		Timeshow();
		exit(-1);
	}

	// ����ͻ��˵ĵ�ַ��Ϣ
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 0;   //����Ϊ0  ��ʾ�Զ�����
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //����ip

	//����socket�׽���
	SOCKET client_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_fd < 0) {
		fprintf(logfile, "�����룺%d    ", WSAGetLastError());
		fprintf(logfile, "�ͻ����׽���socket����ʧ��  ");
		Timeshow();
		exit(-1);
	}

	if ((bind(client_fd, (LPSOCKADDR)&client_addr, sizeof(client_addr)))) {
		fprintf(logfile, "�����룺%d    ", WSAGetLastError());
		fprintf(logfile, "�������˰�ʧ��  ");
		Timeshow();
		exit(-1);
	}

	char Buffer[BUFFER_SIZE];
	memset(Buffer, 0, sizeof(Buffer));
	int count = Make_Read_Request(Buffer);
	int l;// l ���ڼ�¼���ݰ��Ĵ�С
	long long Sum = 0;

	long long start = clock();
	//�����������RRQ���ݰ�
	if (sendto(client_fd, Buffer, count, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
		fprintf(logfile, "�����룺%d    ", WSAGetLastError());
		fprintf(logfile, "�ͻ��˷�������ʧ��  ");
		Timeshow();
		exit(-1);
	}
	Sum += count;

	char* srcip = inet_ntoa(client_addr.sin_addr);
	char* dstip = inet_ntoa(server_addr.sin_addr);
	fprintf(logfile, "�ͻ������������������������,���ĳ���Ϊ%d�ֽ�,ip.src=%s,srcport=%d,ip.dst=%s,dstport=%d  ", count, srcip, client_addr.sin_port, dstip, server_addr.sin_port);
	Timeshow();
	fprintf(logfile, "��������%s�ļ�����������ģʽΪ%s  ", Buffer + 2, Buffer + 2 + strlen(Buffer + 2) + 1);
	Timeshow();

	char receive[BUFFER_SIZE];
	memset(receive, 0, sizeof(receive));
	//���շ��������ص�ȷ����Ϣ��
	int server_addr_len = sizeof(server_addr);
	if ((l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len)) == SOCKET_ERROR) {
		fprintf(logfile, "�����룺%d    ", WSAGetLastError());
		fprintf(logfile, "�ͻ��˽�������ʧ��  ");
		Timeshow();
		exit(-1);
	}

	fprintf(logfile, "�ͻ��˽��յ��������ķ��ذ�  ");
	Timeshow();


	int ack = 1, block;   //��ű�ʶ
	int flag = 0;
	while (1) {
		if (receive[1] == 3) {

			Sum += 4 + (long long)l;
			block = (((receive[2] & 0xff) << 8) + (receive[3] & 0xff)) & 0xffff;
			ack = (ack % 65536);

			if (ack == block) {
				fwrite(receive + 4, 1, l, fpr);
				fprintf(logfile, "�ͻ����յ����������͵ĵ� %d �����ݱ��ģ����Ĵ�СΪ %d �ֽڣ�ip.src=%s,srcport=%d,ip.dst=%s,dstport=%d  ", block, l, srcip, client_addr.sin_port, dstip, server_addr.sin_port);
				Timeshow();
				Sum += l;
				Make_ACK(Buffer, block);	//��дȷ�ϱ���
				//��������������ݰ�
				if (sendto(client_fd, Buffer, 4, 0, (LPSOCKADDR)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
					fprintf(logfile, "�����룺%d    ", WSAGetLastError());
					fprintf(logfile, "�ͻ��˷�������ʧ��  ");
					Timeshow();
					exit(-1);
				}

				Sum += 4;
				fprintf(logfile, "�ͻ��������������ȷ�ϱ��ģ�ACK= %d   ", ack);
				Timeshow();

				if (l < 512) break;

				memset(receive, 0, sizeof(receive));
				//���շ��������ص�ȷ����Ϣ��
				if ((l = recvfrom(client_fd, receive, BUFFER_SIZE, 0, (LPSOCKADDR)&server_addr, &server_addr_len)) == SOCKET_ERROR) {
					fprintf(logfile, "�����룺%d    ", WSAGetLastError());
					fprintf(logfile, "�ͻ��˽�������ʧ��  ");
					Timeshow();
					exit(-1);
				}

			}
			ack++;
		}

		if (receive[1] == 5) {
			flag = 1;
			switch (receive[3]) {
			case 0: fprintf(logfile, "���󣡴�����Ϊ��0\t������Ϣ��%s  ", receive + 4); Timeshow();
				break;
			case 1: fprintf(logfile, "���󣡴�����Ϊ��1\t������Ϣ��File not found  "); Timeshow();
				break;
			case 2: fprintf(logfile, "���󣡴�����Ϊ��2\t������Ϣ��Access violation  "); Timeshow();
				break;
			case 3: fprintf(logfile, "���󣡴�����Ϊ��3\t������Ϣ��Disk full or alloction exceeded  "); Timeshow();
				break;
			case 4: fprintf(logfile, "���󣡴�����Ϊ��4\n������Ϣ��Illegal TFTP operation  "); Timeshow();
				break;
			case 5: fprintf(logfile, "���󣡴�����Ϊ��5\n������Ϣ��ID Unknown transfer ID\n"); Timeshow();
				break;
			case 6: fprintf(logfile, "���󣡴�����Ϊ��6\n������Ϣ��File allready exits\n"); Timeshow();
				break;
			case 7: fprintf(logfile, "���󣡴�����Ϊ��7\n������Ϣ��No such user\n"); Timeshow();
				break;
			}
		}
		if (flag) break;

	}
	long long end = clock();
	fprintf(logfile, "���ݴ������  ");
	Timeshow();
	if (!flag) {
		fprintf(logfile, "���������ļ���ƽ��������Ϊ %.3f KBps  ", Sum * 1.0 * 1000 / ((end - start) * 1.0 * 1024));
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
	}
	WSACleanup();
	return 0;
}
