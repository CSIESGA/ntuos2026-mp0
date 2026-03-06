// #include "kernel/types.h"
// #include "kernel/stat.h"
// #include "user/user.h"
// #include "kernel/fs.h"

// void mp0(char *root, char key)
// {
//     // TODO: implement mp0
//     // Hint: Use pipe, fork, and walk
//     printf("%s [error opening dir]\n", root); // Placeholder behavior
//     printf("\n0 directories, 0 files\n");
// }

// int main(int argc, char *argv[])
// {
//     if (argc != 3)
//     {
//         printf("usage: mp0 <root_directory> <key>\n");
//         exit(1);
//     }
//     mp0(argv[1], *argv[2]);
//     exit(0);
// }
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

struct counts {
    int dir_count;
    int file_count;
};

int count_key(char *path, char key)
{
    int cnt = 0;
    for (int i = 0; path[i] != '\0'; i++)
    {
        if (path[i] == key)
            cnt++;
    }
    return cnt;
}

int is_dot_or_dotdot(char *name)
{
    if (strcmp(name, ".") == 0)
        return 1;
    if (strcmp(name, "..") == 0)
        return 1;
    return 0;
}

// Traverse a directory.
// count_self_dir = 1 表示這個 directory 要算進 dir_count
// count_self_dir = 0 表示這個 directory 不算進 dir_count（root 要這樣）
void walk(char *path, char key, struct counts *cnt, int count_self_dir)
{
    int fd;
    struct stat st;
    struct dirent de;
    char buf[512];
    char namebuf[DIRSIZ + 1];
    char *p;

    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        printf("%s [error opening dir]\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        close(fd);
        printf("%s [error opening dir]\n", path);
        return;
    }

    // 題目要求：如果 root_directory 是 file，也要當成 error opening dir
    if (st.type != T_DIR)
    {
        close(fd);
        printf("%s [error opening dir]\n", path);
        return;
    }

    // 成功開啟 directory 才印 trace
    printf("%s %d\n", path, count_key(path, key));

    // root 不算，其他成功開啟的子目錄要算
    if (count_self_dir)
        cnt->dir_count++;

    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf))
    {
        close(fd);
        return;
    }

    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0)
            continue;

        memmove(namebuf, de.name, DIRSIZ);
        namebuf[DIRSIZ] = '\0';

        if (is_dot_or_dotdot(namebuf))
            continue;

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = '\0';

        if (stat(buf, &st) < 0)
        {
            continue;
        }

        if (st.type == T_FILE)
        {
            printf("%s %d\n", buf, count_key(buf, key));
            cnt->file_count++;
        }
        else if (st.type == T_DIR)
        {
            walk(buf, key, cnt, 1);
        }
    }

    close(fd);
}

void mp0(char *root, char key)
{
    int p[2];
    int pid;
    struct counts cnt;

    cnt.dir_count = 0;
    cnt.file_count = 0;

    if (pipe(p) < 0)
    {
        fprintf(2, "pipe failed\n");
        exit(1);
    }

    pid = fork();
    if (pid < 0)
    {
        fprintf(2, "fork failed\n");
        close(p[0]);
        close(p[1]);
        exit(1);
    }

    if (pid == 0)
    {
        // child: do traversal
        close(p[0]);

        walk(root, key, &cnt, 0);

        // blank line between child trace and parent summary
        printf("\n");

        write(p[1], &cnt, sizeof(cnt));
        close(p[1]);
        exit(0);
    }
    else
    {
        // parent: read result and print summary
        close(p[1]);

        if (read(p[0], &cnt, sizeof(cnt)) != sizeof(cnt))
        {
            cnt.dir_count = 0;
            cnt.file_count = 0;
        }

        close(p[0]);
        wait(0);

        printf("%d directories, %d files\n", cnt.dir_count, cnt.file_count);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: mp0 <root_directory> <key>\n");
        exit(1);
    }

    mp0(argv[1], *argv[2]);
    exit(0);
}