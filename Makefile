#------------------------------------------------------------------------------
define colorecho
	@tput setaf $2
	@tput bold
	@echo $1
	@tput sgr0
endef

OK_STRING=$(OK_COLOR)[OK]$(NO_COLOR)
ERROR_STRING=$(ERROR_COLOR)[ERRORS]$(NO_COLOR)
WARN_STRING=$(WARN_COLOR)[WARNINGS]$(NO_COLOR)
#------------------------------------------------------------------------------
SRCDIR=$(shell pwd)
#------------------------------------------------------------------------------
LIB=./lib
OBJ=./obj
SRC=./src
EXSRC=./examples
BIN=./bin

CXX = g++ 
LF = -shared
EX = .so
MKDIR_P = mkdir -p
#------------------------------------------------------------------------------
INCLUDE_DIR += -I. -I$(SRC)/ -I/usr/local/include/
LIBS += -L/usr/local/lib -lm -L/usr/lib/x86_64-linux-gnu/ -ljpeg -lpng -lgflags -lpthread
LIBBCV = -L$(LIB)/ -lbcv

FFMPEG_LIBS=    -lavdevice -lavformat -lavfilter \
				-lavcodec -lswresample -lswscale -lavutil
#------------------------------------------------------------------------------
CXXFLAGS = -O3 -fPIC -Wall -pedantic 
CXXFLAGS += -march=native -mtune=native -msse3 -DHAVE_SSE
BCVLIB_OBJS = rw_jpeg.o rw_png.o bcv_io.o bcv_diff_ops.o bcv_utils.o \
bcv_kmeans.o Slic.o tvsegment.o tvdn.o bcv_imgproc.o
TESTS = test_io test_slic test_tvsegment test_tvdn test_imgproc

VPATH = $(SRC):$(EXAMPLES)
TEST_OBJS = $(addsuffix .o, $(TESTS) )


.PHONY: directories

all: directories libbcv $(TESTS)

directories: $(BIN) $(OBJ) $(LIB)

$(BIN):
	$(MKDIR_P) $(BIN)
$(OBJ):
	$(MKDIR_P) $(OBJ)
$(LIB):
	$(MKDIR_P) $(LIB)

tests: $(TESTS)

#test_ffmpeg: test_ffmpeg.o
#	$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) $(OBJ)/$@.o $(LIBS) $(LIBBCV) $(FFMPEG_LIBS) -o $(BIN)/$@ 
#	$(call colorecho, "CREATED "$(BIN)/$@" SUCCESSFULLY!", 2)

libbcv: $(BCVLIB_OBJS)
	$(CXX) $(LF) $(CXXFLAGS) $(INCLUDE_DIR) \
	$(addprefix $(OBJ)/, $(BCVLIB_OBJS) ) $(LIBS) -o $(LIB)/$@$(EX) 
	$(call colorecho, "BUILT LIBRARY! CREATED lib/$@$(EX) SUCCESSFULLY!", 2)

%.o: $(SRC)/%.cpp $(SRC)/*.h
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) -c $< -o $(OBJ)/$@ $(LIBS)
	$(call colorecho, "COMPILED "$(OBJ)/$@" SUCCESSFULLY!", 2)

%.o: $(EXSRC)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) -c $< -o $(OBJ)/$@ $(LIBS) $(LIBBCV)
	$(call colorecho, "COMPILED "$(OBJ)/$@" SUCCESSFULLY!", 2)
# ------------------------------------------------------------------------------
# 								BUILD EXAMPLES
# ------------------------------------------------------------------------------
$(TESTS): $(TEST_OBJS) libbcv
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) $(OBJ)/$@.o $(LIBS) $(LIBBCV) -o $(BIN)/$@ 
	$(call colorecho, "CREATED "$(BIN)/$@" SUCCESSFULLY!", 2)
#------------------------------------------------------------------------------
clean:
	rm -f $(OBJ)/*.o
	rm -f $(LIB)/libbcv$(EX)

install:
	ln -s $(CURDIR)/$(LIB)/libbcv$(EX) /usr/local/lib/libbcv$(EX) 
	$(call colorecho, "INSTALLED /usr/local/lib/libbcv$(EX) !", 2)
	$(call colorecho, "you may wish to run 'sudo ldconfig' now", 1)
