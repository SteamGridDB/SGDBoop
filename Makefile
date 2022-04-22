make_sgdb:
	ifeq ($(OS),Windows_NT)
		gcc sgdbop.c curl-helper.c string-helpers.c -lcurl -o SGDBop.exe
	else
		gcc sgdbop.c curl-helper.c string-helpers.c -lcurl -o SGDBop