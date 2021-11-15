#include <windows.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include "Source_h.h"
#include <locale.h>
#include <conio.h>

#pragma warning(disable:4996)
#pragma comment (lib, "Rpcrt4.lib")

#define SIZE 100

using namespace std;

void download_to_server(int index_client)
{
	char PathToFile[SIZE] = { 0 }, FileName[64] = { 0 };
	int DataOfFile[cMaxBuf] = { 0 };
	unsigned int len_buf = 0, len_path = 0, len_FileName = 0;
	int res, EndOfFile = 0, symbol;

	cout << "Введите путь до файла, который будет загружен на сервер: ";
	cin >> PathToFile;
	len_path = strlen(PathToFile);

	int i;
	for (i = len_path - 1; PathToFile[i] != '\\'; i--);
	for (i += 1; i < len_path && len_FileName < 63; i++)
	{
		FileName[len_FileName] = PathToFile[i];
		len_FileName++;
	}
	char FileNameOnServer[SIZE] = { 0 };
	cout << "Введите путь для загрузки файла на сервер: ";
	cin >> FileNameOnServer;

	FILE* file = fopen(PathToFile, "rb");
	while (1)
	{
		while (len_buf < cMaxBuf)
		{
			res = fread(&symbol, 1, 1, file);
			if (res == 0)
			{
				EndOfFile = 1;
				break;
			}
			DataOfFile[len_buf++] = symbol;
		}

		res = MakeFileOnServer((const unsigned char*)FileNameOnServer, DataOfFile, len_buf, index_client, EndOfFile);

		if (res < 0)
		{
			cout << "Ошибка авторизации." << endl;
			break;
		}
		memset(DataOfFile, 0, cMaxBuf);
		len_buf = 0;
		if (EndOfFile)
		{
			cout << "Файл успешно закачен на сервер." << endl;
			break;
		}
	}
	if (file != NULL)
		fclose(file);
}

void download_from_server(int index_client)
{
	char PathToFile[SIZE] = { 0 }, FileName[64] = { 0 };
	int DataOfFile[cMaxBuf] = { 0 };
	unsigned int len_buf = 0, len_path = 0, len_FileName = 0;
	int res, EndOfFile = 0;
	cout << "Введите путь до файла: ";
	cin >> PathToFile;
	for (; PathToFile[len_path] != L'\0'; len_path++);

	int i = 0;
	for (i = len_path - 1; PathToFile[i] != '\\'; i--);
	for (i += 1; i < len_path && len_FileName < 63; i++)
	{
		FileName[len_FileName] = PathToFile[i];
		len_FileName++;
	}
	FileName[len_FileName] = '\0';

	bool IsOpenedFile = true;
	FILE* file = NULL;
	while (1)
	{

		res = CopyOnClient((const unsigned char*)PathToFile, DataOfFile, &len_buf, index_client, &EndOfFile);
		if (res < 0)
		{
			cout << "Нет доступа." << endl;
			break;
		}
		if (IsOpenedFile)
		{
			file = fopen(FileName, "wb");
			IsOpenedFile = false;
		}
		for (unsigned int i = 0; i < len_buf; i++)
			fwrite(&DataOfFile[i], 1, 1, file);

		memset(DataOfFile, 0, cMaxBuf);
		if (EndOfFile)
			break;
	}
	if (file != NULL)
	{
		cout << "Файл был успешно скачен." << endl;
		fclose(file);
	}
}

int main()
{
	setlocale(LC_ALL, "Rus");
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	RPC_STATUS status;
	RPC_CSTR szStringBinding = NULL;

	char IPv4[32];
	char ListeningPort[8];
	cout << "Введите IPv4: ";
	cin >> IPv4;
	cout << "Введите порт: ";
	cin >> ListeningPort;


	//Функция RpcStringBindingCompose создает дескриптор привязки строки.
	status = RpcStringBindingComposeA(
		NULL,                                        // UUID to bind to.
		(RPC_CSTR)("ncacn_ip_tcp"),					// Use TCP/IP protocol.
		(RPC_CSTR)(IPv4),				// TCP/IP network address to use.
		(RPC_CSTR)(ListeningPort),					// TCP/IP port to use.
		NULL,										// Protocol dependent network options to use.
		&szStringBinding);							// String binding output.

	if (status)
	{
		cout << "Подключение отсутсвует." << endl;
		exit(status);
	}

	//Функция RpcBindingFromStringBinding возвращает дескриптор привязки из строкового представления дескриптора привязки.
	status = RpcBindingFromStringBindingA(
		szStringBinding,        // The string binding to validate.
		&hExample1Binding);     // Put the result in the implicit binding
								// handle defined in the IDL file.
	if (status)
		exit(status);

	//Номер клиента
	int index_client = 0, size_pass;
	char login[SIZE] = { 0 };
	char password[SIZE] = { 0 };
	cout << "Введите логин: ";
	cin >> login;
	cout << "Введите пароль: ";
	for (size_pass = 0; size_pass < SIZE && password[size_pass - 1] != L'\r'; size_pass++)
	{
		password[size_pass] = _getch();
	}
	password[size_pass - 1] = '\0';
	system("cls");
	cout << "Ожидание входа..." << endl;

	int res = MakeClientOnServer((const unsigned char*)login, (const unsigned char*)password, &index_client);
	printf("Присвоен индекс %d\n", index_client);
	switch (res)
	{
	case -2:
		cout << "Превышено кол-во клиентов.";
		_getch();
		return 0;
		break;
	case -1:
		cout << "Ошибка авторизации.";
		_getch();
		return 0;
		break;
	}
	cout << "Вход успешно выполнен." << endl;

	RpcTryExcept
	{
		char PathToFile[SIZE] = { 0 };
		int ChoosenAction = 0;
		while (1)
		{
			cout << "1) Скачать файл с сервера" << endl << "2) Закачать файл на сервер" << endl << "3) Удалить файл с сервера" << endl << "4) Выход" << endl << "Введите номер действия: ";
			cin >> ChoosenAction;
			switch (ChoosenAction)
			{
			case 1:
				download_from_server(index_client);
				break;
			case 2:
				download_to_server(index_client);
				break;
			case 3:
				cout << "Введите путь до файла, который будет удалён: ";
				cin >> PathToFile;
				res = DeleteFileOnServer((const unsigned char*)PathToFile, index_client);
				if (res < 0)
					cout << "Ошибка." << endl;
				else
					cout << "Файл успешно удалён." << endl;
				break;

			case 4:
				ClientOut(index_client);
				goto OUT_;
				break;
			default:
				cout << "Введена некорректная команда." << endl;
			}
			memset(PathToFile, 0, SIZE);
			ChoosenAction = 0;
		}
	}
		RpcExcept(1)
	{
		std::cerr << "Runtime reported exception " << RpcExceptionCode()
			<< std::endl;
	}
	RpcEndExcept

		OUT_ :
	// Free the memory allocated by a string.
	status = RpcStringFreeA(
		&szStringBinding); // String to be freed.

	if (status)
		exit(status);

	// Releases binding handle resources and disconnects from the server.
	status = RpcBindingFree(
		&hExample1Binding); // Frees the implicit binding handle defined in the IDL file.

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
