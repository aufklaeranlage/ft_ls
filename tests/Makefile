TEST_NAMES :=
TEST_NAMES += test_utils

SRCS	:=
vpath %.c src/ ../src/

OBJS := $(SRCS:.c=.o)
OBJS := $(addprefix obj/,$(OBJS))

DEPS := $(OBJS:.o=.d)

UTILS_OBJS := utils.o test_utils.o
UTILS_OBJS := $(addprefix obj/, $(UTILS_OBJS))
UTILS_DEPS := $(UTILS_OBJS:.o=.d)

CC := cc
LD := cc
RM := rm -rf
MKDIR = mkdir -p $(@D)

override CFLAGS += -Wall
override CFLAGS += -Wextra
override CFLAGS += -Werror

override CPPFLAGS += -I../include
override CPPFLAGS += -Iinclude
override CPPFLAGS += -MMD
override CPPFLAGS += -MP

override LDFLAGS += -lcmocka

ifdef DEBUG
	CFLAGS += -O0
	CFLAGS += -g3
else
	CFLAGS += -O3
endif

ifeq "$(DEBUG)" "UNDEFINED"
	CFLAGS += -fsanitize=undefined
	LDFLAGS += -fsanitize=undefined
endif

ifeq "$(DEBUG)" "ASAN"
	CFLAGS += -fsanitize=address
	LDFLAGS += -fsanitize=address
endif

ifeq "$(DEBUG)" "LSAN"
	CFLAGS += -fsanitize=leak
	LDFLAGS += -fsanitize=leak
endif

# rules

all: $(TEST_NAMES)

test_utils: $(UTILS_OBJS)
	$(LD) $(UTILS_OBJS) -o $@ $(LDFLAGS)

obj/%.o: %.c
	$(MKDIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

-include $(DEPS)
-include $(UTILS_DEPS)

clean:
	$(RM) obj/

fclean: clean
	$(RM) $(TEST_NAMES)

re: fclean all

run: re
	$(TEST_NAMES:%=./%; )

.PHONY: all clean fclean re
