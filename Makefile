
BASE_LIB = voice_cmd_convert
TARGET_STATIC_LIBS = lib$(BASE_LIB).a
TARGET_SHARE_LIBS = lib$(BASE_LIB).so
#CC = gcc -Werror
#AR = ar rcs
SRC = $(wildcard ./*.c) 
OBJ = $(patsubst %.c,%.o,${SRC})
LDFLAG += -L
INCLUDE = -I./include
INCLUDE += -I$(OUTDIR)/include

all:$(TARGET_STATIC_LIBS)

$(TARGET_SHARE_LIBS):$(OBJ)
	$(CC) -shared -fPIC -o $@ $(SRC) $(INCLUDE)

$(TARGET_STATIC_LIBS):$(OBJ)
	$(AR) -rc $@ $^

%.o:%.c
	$(CC) $(CFLAG) -fPIC  $(INCLUDE) -o $@ -c $<

install:
	@cp -rf voice_cmd_convert.h $(OUTDIR)/include/
	@cp -rf $(TARGET_STATIC_LIBS) $(OUTDIR)/lib/

clean:
	-rm -rf $(TARGET_SHARE_LIBS) $(TARGET_STATIC_LIBS) $(OBJ)
