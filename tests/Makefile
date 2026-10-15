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

CFLAGS :=
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror

CPPFLAGS :=
ifdef CMOCKA_INCLUDE
	CPPFLAGS += -I$(CMOCKA_INCLUDE)
endif
CPPFLAGS += -Iinclude
CPPFLAGS += -MMD
CPPFLAGS += -MP

LDFLAGS :=
ifdef CMOCKA_LIB
	LDFLAGS += -L$(CMOCKA_LIB)
endif
LDFLAGS += -lcmocka

ifneq "$(or $(DEBUG), $(or $(UB), $(or $(ASAN), $(LSAN))))" ""
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
