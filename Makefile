NAME	:= ft_ls

SRCS	:=
vpath %.c src/
SRCS	+= ft_ls.c
SRCS	+= utils.c

OBJS := $(SRCS:.c=.o)
OBJS := $(addprefix obj/,$(OBJS))

DEPS := $(OBJS:.o=.d)

CC := cc
LD := cc
RM := rm -rf
MKDIR = mkdir -p $(@D)

CFLAGS :=
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Werror
CFLAGS += -Wunused

CPPFLAGS :=
CPPFLAGS += -Iinclude
CPPFLAGS += -MMD
CPPFLAGS += -MP

LDFLAGS :=

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

all: $(NAME)

$(NAME): $(OBJS)
	$(LD) $(OBJS) -o $@ $(LDFLAGS)

obj/%.o: %.c
	$(MKDIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

-include $(DEPS)

clean:
	$(RM) obj/

fclean: clean
	$(RM) $(NAME)

re: fclean all

run: re
	./$(NAME)

valgrind: re
	valgrind ./$(NAME)

.PHONY: all clean fclean re
