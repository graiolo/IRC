#####################
# GENERIC VARIABLES #
#####################

NAME    = ircserv
INCS    = includes
CC      = c++
RM      = rm -f
CFLAGS  = -Wall -Wextra -Werror -std=c++98 -g
OBJDIR  = obj/
SRCDIR	= srcs/
B_DIR	=

#########
# FILES #
#########

SRCS    = $(wildcard $(SRCDIR)*.cpp)

OBJS = $(addprefix $(OBJDIR), $(notdir $(SRCS:.cpp=.o)))

B_SRCS  = $(wildcard *_bonus.cpp)

B_OBJS  = $(addprefix $(OBJDIR), $(B_SRCS:.cpp=.o))

##########
# COLORS #
##########

YELLOW = \033[0;33m
GREEN = \033[0;32m
RED = \033[0;91m
CIANO = \033[0;36m
RESET = \033[0m

all: ${NAME}

${OBJDIR}%.o: ${SRCDIR}%.cpp
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

${NAME}: ${OBJS}
	${CC} ${OBJS} -o ${NAME} 
	@echo "${GREEN}IRC READY${RESET}"

debug:
	${CC} ${CFLAGS} ${OBJS} -fsanitize=address -g -o ${NAME}
	@echo "${YELLOW}DEBUG READY${RESET}"

clean:
	@${RM} -rf ${OBJDIR}
	@echo "${RED}.O REMOVE${RESET}"

fclean: clean
	@${RM} -f ${NAME}
	@echo "${RED}IRC REMOVE${RESET}"

re: fclean all
