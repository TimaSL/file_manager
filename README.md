# file_manager
remote file manager with client impersonation
Задача:
Написать программу-сервер и программу-клиент, работающие под Windows 7-10. Сервер 
должен предоставлять доступ локальным и удаленным клиентам к файлам в своей 
файловой системе.
Требования:
- Statefull сервер;
- Сервер не должен быть интерактивным (интерфейс командной строки);
- Взаимодействие с клиентами должно осуществляться с помощью механизма RPC;
- При обслуживании клиента должна осуществляться его имперсонация;
- Пользователю должны предоставляться следующие операции: копирование 
указанного файла с клиента на сервер, загрузка указанного файла с сервера на клиента, 
удаление указанного файла на сервере;
- Имя файла передается в формате UNC.