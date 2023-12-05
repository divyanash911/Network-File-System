#include "headers.h"

void man()
{
    printf(BLUE("Input format are as follows\n"));
    printf(BLUE("READ <PATH_TO_FILE_YOU_WANT_TO_READ> \n"));
    printf(GREEN("WRITE <PATH_TO_FILE_YOU_WANT_TO_WRITE> \n"));
    printf(GREEN("APPEND <PATH_TO_FILE_YOU_WANT_TO_APPEND> \n"));
    printf(YELLOW("INFO <PATH_TO_FILE_YOU_WANT_TO_GET_INFO> \n"));
    printf(YELLOW("CREATE FOLDER/FILE <PATH_TO_DIRECTORY_YOU_WANT_TO_CREATE> <NAME>\n"));
    printf(RED("DELETE FOLDER/FILE <PATH_TO_YOU_WANT_TO_DELETE> \n"));
    printf(RED("COPY FILE/FOLDER <SOURCE> <DESTINATION>\n"));
    printf(CYAN("LIST\n"));
    printf(CYAN("EXIT\n"));
}