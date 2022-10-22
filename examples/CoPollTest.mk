EXECUTABLE := copolltest

LIBRARIES := \
	stdc++ \
	pthread

DEPENDENCIES := \
	libcurl \
	libcares

DIRECTORIES :=

OBJECTS := \
	FastPoll.o \
	Resolver.o \
	CloudClient.o \
	Compromise.o \
	CoPoll.o \
	CoResolver.o \
	CoCloudClient.o \
	CoPollTest.o

FLAGS += \
	-rdynamic -fPIC -O2 -MMD \
	$(foreach directory, $(DIRECTORIES), -I$(directory)) \
	$(shell pkg-config --cflags $(DEPENDENCIES))

LIBS := \
	$(foreach library, $(LIBRARIES), -l$(library)) \
	$(shell pkg-config --libs $(DEPENDENCIES))

CFLAGS   += $(FLAGS) -std=gnu11
CXXFLAGS += $(FLAGS) -std=c++2a -fcoroutines

all: build

build: $(PREREQUISITES) $(OBJECTS)
	$(CC) $(OBJECTS) $(FLAGS) $(LIBS) -o $(EXECUTABLE)

clean:
	rm -f $(OBJECTS) $(patsubst %.o,%.d,$(OBJECTS)) $(EXECUTABLE) $(EXECUTABLE).d
