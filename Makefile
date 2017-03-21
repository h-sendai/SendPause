PROG = send_pause
CFLAGS = -g -O2 -Wall
# CFLAGS = -std=c99 -g -O2 -Wall
# CFLAGS += -pthread
# LDLIBS += -L/usr/local/lib -lmylib
# LDLIBS += -lrt
# LDFLAGS += -pthread

all: $(PROG) addcap
OBJS += $(PROG).o
OBJS += set_timer.o
OBJS += my_signal.o
OBJS += logUtil.o
OBJS += flow_ctrl_pause.o
$(PROG): $(OBJS)

addcap:
	sudo setcap cap_net_raw=ep $(PROG)

clean:
	rm -f *.o $(PROG)
