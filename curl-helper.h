// https://stackoverflow.com/a/2329792/16642426
// https://stackoverflow.com/a/1636415/16642426
struct string {
  char *ptr;
  size_t len;
};
void init_string(struct string*);
size_t writefunc(void*, size_t, size_t, struct string*);
