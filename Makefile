ifndef (OS)
OS_Type := NotWindows
else
OS_Type := $(OS)
endif

make_sgdb:
ifeq "$(OS_Type)" "Windows_NT"
	gcc sgdbop.c curl-helper.c string-helpers.c -lcurl -o SGDBop.exe			
else
	gcc sgdbop.c curl-helper.c string-helpers.c -lcurl -o SGDBop
endif