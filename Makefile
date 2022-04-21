make_sgdb:
	ifeq ($(OS),Windows_NT)
		gcc sgdb-steamdownloader.c curl-helper.c -lcurl -o sgdb-steamdownloader.exe
	else
		gcc sgdb-steamdownloader.c curl-helper.c -lcurl -o sgdb-steamdownloader