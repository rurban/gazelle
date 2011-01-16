#include <stddef.h>
#include <stdio.h>

int main()
{
    /* luac.lua needs to know whether it is executed on a 64-Bit computer
     * to produce correct bytecode.
     *
     * The current solution assumes that gazelle is compiled for the same
     * machine architecture as lua was.
     *
     * The correct method would be to examine the lua binary, but this
     * is not portable.
     *
     * A patch for luac that adds preloading support exists [1], but has
     * not been merged into lua so far.
     *
     * [1] http://lua-users.org/lists/lua-l/2009-10/msg00682.html
     */
    if(sizeof(size_t) == 8)
    {
        /* os:execute returns the status code of the shell that executed
         * the command and not the status code of the command
         * itself. Therefore, the program additionally outputs "64bit" to
         * indicate that it was compiled for a 64-Bit architecture.
         */
        printf("64bit\n");
        return 1;
    }
    else
    {
        printf("32bit\n");
        return 0;
    }
}
