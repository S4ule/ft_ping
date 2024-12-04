
SRCS	=	ping.c\
			pars.c\
			timer.c\
			sqrt.c

#---------------------------
CC		=	gcc
NAME	=	ft_ping
LIBS    =
INCLUDE =	-I ./include
CFLAGS	=	-g3 -Wall -Wextra -Werror
#---------------------------
SRCDIR  =	./srcs
OBJDIR  =	./objs
OBJS	=	$(addprefix $(OBJDIR)/,$(SRCS:.c=.o))
#---------------------------

default:
	make all

$(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

$(NAME): $(OBJDIR) $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LIBS) $(INCLUDE) -o $(NAME)

all:	$(NAME)

clean:
	rm -f $(OBJS)
	rm -Rf $(OBJDIR)

fclean:		clean
	rm -f $(NAME)

re:
	make fclean
	make all

.PHONY:	default all clean fclean re
