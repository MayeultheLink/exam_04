#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

# define SIDE_IN 1
# define SIDE_OUT 0

# define STDIN 0
# define STDOUT 1
# define STDERR 2

# define TYPE_END 0
# define TYPE_PIPE 1
# define TYPE_BREAK 2

#ifdef TEST_SH
# define TEST 1
#else
# define TEST 0
#endif

typedef struct	s_cmd
{
	char	**args;
	int		type;
	int		length;
	int		pipes[2];
	struct s_cmd	*next;
	struct s_cmd	*previous;
}				t_cmd;

int	ft_strlen(char *str)
{
	int	i = 0;

	while (str && str[i])
		i++;
	return (i);
}

char	*ft_strdup(char *str)
{
	int	i = 0;
	char	*new;

	while (str && str[i])
		i++;
	if (!(new = (char *)malloc(sizeof(char) * (i + 1))))
		return (write(2, "error: fatal\n", ft_strlen("error: fatal\n")), NULL);
	new[i] = 0;
	while (--i >= 0)
		new[i] = str[i];
	return (new);
}

int	add_arg(t_cmd *cmd, char *arg)
{
	char	**tmp;

	if (!(tmp = (char **)malloc(sizeof(char *) * (cmd->length + 2))))
		return (write(2, "error: fatal\n", ft_strlen("error: fatal\n")), EXIT_FAILURE);
	int	i = 0;
	while (i < cmd->length)
	{
		tmp[i] = cmd->args[i];
		i++;
	}
	tmp[i] = ft_strdup(arg);
	if (!tmp[i])
		return (write(2, "error: fatal\n", ft_strlen("error: fatal\n")), EXIT_FAILURE);
	i++;
	tmp[i] = NULL;
	if (cmd->length)
		free(cmd->args);
	cmd->args = tmp;
	cmd->length++;
	return (EXIT_SUCCESS);
}

int	add_cell(t_cmd **cmd, char *arg)
{
	t_cmd	*new;

	if (!(new = (t_cmd *)malloc(sizeof(t_cmd))))
		return (write(2, "error: fatal\n", ft_strlen("error: fatal\n")), EXIT_FAILURE);
	new->args = NULL;
	new->type = TYPE_END;
	new->length = 0;
	new->next = NULL;
	new->previous = NULL;
	if (*cmd)
	{
		(*cmd)->next = new;
		new->previous = *cmd;
	}
	*cmd = new;
	return (add_arg(new, arg));
}

int	parse_cmd(t_cmd **cmd, char *arg)
{
	int	is_break;

	is_break = (strcmp(arg, ";") == 0);
	if (is_break && !*cmd)
		return (EXIT_SUCCESS);
	else if (!is_break && (!*cmd || (*cmd)->type > TYPE_END))
		return (add_cell(cmd, arg));
	else if (!strcmp(arg, "|"))
		(*cmd)->type = TYPE_PIPE;
	else if (is_break)
		(*cmd)->type = TYPE_BREAK;
	else
		return (add_arg(*cmd, arg));
	return (0);
}

int	exec_cmd(t_cmd *cmd, char **env)
{
	int		pid;
	int		status;
	int		ret = EXIT_FAILURE;

	if (cmd->type == TYPE_PIPE)
		if (pipe(cmd->pipes) < 0)
			return (ret);
	pid = fork();
	if (pid < 0)
		return (write(2, "error: fatal\n", ft_strlen("error: fatal\n")), ret);
	if (pid == 0)
	{
		if (cmd->type == TYPE_PIPE && (dup2(cmd->pipes[SIDE_IN], STDOUT) < 0))
		{
			write(2, "error: fatal\n", ft_strlen("error: fatal\n"));
			exit(ret);
		}
		if (cmd->previous && cmd->previous->type == TYPE_PIPE && (dup2(cmd->previous->pipes[SIDE_OUT], STDIN) < 0))
		{
			write(2, "error: fatal\n", ft_strlen("error: fatal\n"));
			exit(ret);
		}
		if ((ret = execve(cmd->args[0], cmd->args, env)) < 0)
		{
			write(2, "error: cannot execute ", ft_strlen("error: cannot execute "));
			write(2, cmd->args[0], ft_strlen(cmd->args[0]));
			write(2, "\n", 1);
			exit(ret);
		}
	}
	if (pid > 0)
	{
		waitpid(pid, &status, 0);
		if (cmd->type == TYPE_PIPE)
			close(cmd->pipes[SIDE_IN]);
		if (cmd->previous && cmd->previous->type == TYPE_PIPE)
			close(cmd->previous->pipes[SIDE_OUT]);
		if (WIFEXITED(status))
			ret = WEXITSTATUS(status);
	}
	return (ret);
}

int	exec_cmds(t_cmd *cmd, char **env)
{
	int	ret = EXIT_SUCCESS;
	int	i;

	while (cmd)
	{
		if (!strcmp(cmd->args[0], "cd"))
		{
			i = 0;
			while (cmd->args && cmd->args[i])
				i++;
			if (i != 2)
				write(2, "error: cd: bad arguments\n", ft_strlen("error: cd: bad arguments\n"));
			else if (chdir(cmd->args[1]))
			{
				write(2, "error: cd: cannot change directory to ", ft_strlen("error: cd: cannot change directory to "));
				write(2, cmd->args[1], ft_strlen(cmd->args[1]));
				write(2, "\n", 1);
			}
		}
		else
			ret = exec_cmd(cmd, env);
		cmd = cmd->next;
	}
	return (ret);
}

void	free_all(t_cmd *cmd)
{
	t_cmd	*tmp;
	int	i;

	while (cmd)
	{
		i = 0;
		while (cmd->args && cmd->args[i])
		{
			free(cmd->args[i]);
			i++;
		}
		free(cmd->args);
		tmp = cmd;
		cmd = cmd->next;
		free(tmp);
	}
}

int	main(int ac, char **av, char **env)
{
	t_cmd	*cmd;
	int		ret;
	int		i = 1;

	ret = EXIT_SUCCESS;
	cmd = NULL;
	while (i < ac)
		parse_cmd(&cmd, av[i++]);
	if (cmd)
	{
		while (cmd->previous)
			cmd = cmd->previous;
		ret = exec_cmds(cmd, env);
	}
	free_all(cmd);
	if (TEST)
		while (1);
	return (ret);
}
