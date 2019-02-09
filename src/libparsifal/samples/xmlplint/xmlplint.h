#define VER_XMLPLINT "1.1.0"

#define PFOUT fout
#define ASSERT_MEM_ABORT(p) \
  if (!(p)) { printf("Out of memory! Line: %d\n", __LINE__); return XML_ABORT; }

enum outputformat {
	XMLOUTPUTFORMAT_ROUNDRIP,
	XMLOUTPUTFORMAT_EVENTS,
	XMLOUTPUTFORMAT_CANONICAL,
	XMLOUTPUTFORMAT_SILENT
};

struct XMLNOTATION {
	char *name;
	char *publicID;
	char *systemID;
};

#define RET_SUCCESS 0
#define RET_NOTWF 1
#define RET_NOTVALID 2
#define RET_IOERR 3
#define RET_FATAL 4

