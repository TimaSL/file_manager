#define _CRT_SECURE_NO_WARNINGS
//#include "pch.h"
#include <iostream>
#include "Source_h.h"
#include <fstream>
#include <windows.h>
#include <stdio.h>
using namespace std;

#pragma warning(disable:4996)
#pragma comment (lib, "Rpcrt4.lib")
const unsigned int cMaxClient = 50;

struct client
{
	handle_t client_handle;
	FILE* file;
	bool IsClosedFile;
};

client ALL_Clients[cMaxClient];
unsigned int count_clients = 0;
bool IsConnectedClient[cMaxClient] = { false };

int MakeClientOnServer(
	/* [in][string] */ const unsigned char* login,
	/* [in][string] */ const unsigned char* password,
	/* [out] */ int* index)
{
	if (count_clients >= cMaxClient)
		return -2;
	int new_index = 0;

	for (; (IsConnectedClient[new_index]) && new_index < cMaxClient; new_index++);
	if (new_index == cMaxClient)
		return -2;
	handle_t handle = 0;
	//Если функция завершается успешно, вы получаете дескриптор токена, который представляет вошедшего в систему пользователя. \
	Затем вы можете использовать этот дескриптор токена для олицетворения указанного пользователя или, в большинстве случаев, \
	для создания процесса, который выполняется в контексте указанного пользователя.
	if (!LogonUserA((LPCSTR)login, NULL, (LPCSTR)password, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, &handle))
	{
		cout << "Не получен дескриптор токена.";
		return -1;
	}
	//Функция ImpersonateLoggedOnUser позволяет вызывающему потоку олицетворять контекст безопасности вошедшего в систему пользователя. \
	Пользователь представлен дескриптором токена.
	if (!ImpersonateLoggedOnUser(handle))
	{
		cout << "Ошибка имперсонизации.";
		return -1;
	}
	ALL_Clients[new_index].client_handle = handle;
	ALL_Clients[new_index].IsClosedFile = true;
	ALL_Clients[new_index].file = NULL;
	IsConnectedClient[new_index] = true;
	count_clients++;
	(*index) = new_index;
	cout << "Клиент успешно подключился." << endl;
	return 1;
}

void Output(
	/* [string][in] */ const unsigned char* szOutput)
{
	std::cout << szOutput << std::endl;
}



int CopyOnClient(
	/* [in][string] */ const unsigned char* path,
	/* [out] */ int buf[65534],
	/* [out] */ unsigned int* length_buf,
	/* [in] */ int index,
	/* [out] */ int* check_eof)
{
	if (!ImpersonateLoggedOnUser(ALL_Clients[index].client_handle))
	{
		cout << "У клиента №" << index << " ошибка имперсонизации." << endl;
		return -1;
	}
	if (ALL_Clients[index].IsClosedFile)
	{
		cout << "Первое открытие файла!\n";
		ALL_Clients[index].file = fopen((const char*)path, "rb");
		ALL_Clients[index].IsClosedFile = false;
	}
	if (!ALL_Clients[index].file)
	{
		cout << "Файл закрыт";
		cout << "Нет доступа." << endl;
		ALL_Clients[index].IsClosedFile = true;
		return -1;
	}

	int symbol = 0;
	int len = 0;
	while (len < cMaxBuf)
	{
		int check = fread(&symbol, 1, 1, ALL_Clients[index].file);
		if (check == 0)
		{
			*check_eof = 1;
			fclose(ALL_Clients[index].file);
			ALL_Clients[index].IsClosedFile = true;
			break;
		}
		buf[len] = symbol;
		len++;
	}
	(*length_buf) = len;
	return 1;
}

int MakeFileOnServer(
	/* [in][string] */ const unsigned char* FileName,
	/* [in] */ int buf[65534],
	/* [in] */ int len_buf,
	/* [in] */ int index,
	/* [in] */ int EndOfFile)
{

	if (!ImpersonateLoggedOnUser(ALL_Clients[index].client_handle))
	{
		cout << "У клиента № ошибка имперсонизации." << endl;
		return -1;
	}

	if (ALL_Clients[index].IsClosedFile)
	{
		cout << "Первое открытие файла!\n";
		ALL_Clients[index].file = fopen((const char*)(FileName), "wb");
		ALL_Clients[index].IsClosedFile = false;
	}
	if (!ALL_Clients[index].file)
	{
		cout << "Файл закрыт\n";
		cout << "У клиента нет прав для данного действия" << endl;
		ALL_Clients[index].IsClosedFile = true;
		return -1;
	}
	for (unsigned int i = 0; i < len_buf; i++)
	{
		fwrite(&buf[i], 1, 1, ALL_Clients[index].file);
	}

	if (EndOfFile)
	{
		ALL_Clients[index].IsClosedFile = true;
		fclose(ALL_Clients[index].file);
	}

	return 1;
}

int DeleteFileOnServer(
	/* [in][string] */ const unsigned char* PathToFile,
	/* [in] */ int index)
{

	if (!ImpersonateLoggedOnUser(ALL_Clients[index].client_handle))
	{
		cout << "У клиента ошибка имперсонизации." << endl;
		return -1;
	}

	if (remove((const char*)PathToFile) == -1)
	{
		cout << "У клиента нет прав для данного действия" << endl;
		return -1;
	}
	return 1;
}

int ClientOut(
	/* [in] */ int index)
{
	if (index > cMaxClient || index < 0)
		return -1;
	IsConnectedClient[index] = false;
	ALL_Clients[index].client_handle = 0;
	ALL_Clients[index].IsClosedFile = true;
	ALL_Clients[index].file = NULL;
	count_clients--;
	cout << "Клиент отключился." << endl;
	return 1;
}

// Naive security callback.
RPC_STATUS CALLBACK SecurityCallback(RPC_IF_HANDLE /*hInterface*/, void* /*pBindingHandle*/)
{
	return RPC_S_OK; // Always allow anyone.
}

int main()
{
	setlocale(LC_ALL, "Rus");
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	RPC_STATUS status;
	RpcServerRegisterAuthInfoA(nullptr, RPC_C_AUTHN_WINNT, 0, nullptr);
	//Функция RpcServerUseProtseqEp сообщает библиотеке времени выполнения RPC использовать указанную \
	последовательность протокола в сочетании с указанной конечной точкой для приема вызовов удаленных процедур.
	status = RpcServerUseProtseqEpA(
		(RPC_CSTR)("ncacn_ip_tcp"),			// Use TCP/IP protocol.
		RPC_C_PROTSEQ_MAX_REQS_DEFAULT,		// Backlog queue length for TCP/IP.
		(RPC_CSTR)("9000"),					// TCP/IP port to use.
		NULL);								// No security.

	if (status)
		exit(status);

	//Функция RpcServerRegisterIf2 регистрирует интерфейс с библиотекой времени выполнения RPC.
	status = RpcServerRegisterIf2(
		Example1_v1_0_s_ifspec,              // Interface to register.
		NULL,                                // Use the MIDL generated entry-point vector.
		NULL,                                // Use the MIDL generated entry-point vector.
		RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH, // Forces use of security callback.
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,      // Use default number of concurrent calls.
		(unsigned)-1,                        // Infinite max size of incoming data blocks.
		SecurityCallback);                   // Naive security callback.

	if (status)
		exit(status);
	cout << "Сервер работает..." << endl;

	// Функция RpcServerListen сигнализирует библиотеке времени выполнения RPC прослушивать удаленные вызовы процедур
	status = RpcServerListen(
		1,                                   // Recommended minimum number of threads.
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,      // Recommended maximum number of threads.
		FALSE);                              // Start listening now.
	cout << "Сервер выключен" << endl;
	if (status)
		exit(status);
}

// Memory allocation function for RPC.
// The runtime uses these two functions for allocating/deallocating
// enough memory to pass the string to the server.
void* __RPC_USER midl_user_allocate(size_t size)
{
	return malloc(size);
}

// Memory deallocation function for RPC.
void __RPC_USER midl_user_free(void* p)
{
	free(p);
}
