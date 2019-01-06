#include <limits.h>
#include <string.h>
#include <stdio.h>
/* https://stackoverflow.com/questions/14834267/reading-a-text-file-backwards-in-c */

//to truncate
#include <unistd.h>
#include <sys/types.h>
/* https://stackoverflow.com/questions/873454/how-to-truncate-a-file-in-c/873653#873653 */

/* File must be open with 'b' (for binary) in the mode parameter to fopen() */
long fsize(FILE* binaryStream) //2 million lines maximum
{
  long ofs, ofs2;
  int result;

  // if cursor isn't placed at beginning (0 octets), return error (return something not 0)
  if (fseek(binaryStream, 0, SEEK_SET) != 0 ||
      fgetc(binaryStream) == EOF)
    return 0;

  ofs = 1;

  //while cursot not at end of file, and cursor is well puted on ofs octets from the file beginning, read one char in result
  while ((result = fseek(binaryStream, ofs, SEEK_SET)) == 0 &&
         (result = (fgetc(binaryStream) == EOF)) == 0 &&
         ofs <= LONG_MAX / 4 + 1)
    ofs *= 2;

  /* If the last seek failed, back up to the last successfully seekable offset */
  if (result != 0)
    ofs /= 2;

  for (ofs2 = ofs / 2; ofs2 != 0; ofs2 /= 2)
    if (fseek(binaryStream, ofs + ofs2, SEEK_SET) == 0 &&
        fgetc(binaryStream) != EOF)
      ofs += ofs2;

  /* Return -1 for files longer than LONG_MAX */
  if (ofs == LONG_MAX)
    return -1;

  return ofs + 1;
}

/* File must be open with 'b' in the mode parameter to fopen() */
/* Set file position to size of file before reading last line of file */
char* getOffsetBeforeLastBuf(char* buf, int n, FILE* binaryStream, off_t offset)
{ /* and returns the last buf after this position setting, as well as initiates/modifies the value of that position setting = offset */
  long fpos;
  int cpos;

  /* when calling ftell, tells the current value of the position indicator.
  If an error occurs, -1L is returned, and the global variable errno is set to a positive value*/
  if (n <= 1 || (fpos = ftell(binaryStream)) == -1 || fpos == 0)
    return NULL;

  //exploration
  long test1 = fpos;
  printf("test1 = %ld\n", test1);

  cpos = n - 1;
  buf[cpos] = '\0'; /* The length of a C string (an array containing the characters and terminated with a '\0' character)
  is found by searching for the (first) NUL byte*/

  int second = 0; //flag to catch only the second '\n' char starting from the end (native booleans don't exist in C)
  for (;;) //infinite loop I guess
  {
    int c;

    //exploration
    long test2 = fpos;
    printf("test2 = %ld\n", test2);

    if (fseek(binaryStream, --fpos, SEEK_SET) != 0 || //failed to move back the cursor of one position unit
        (c = fgetc(binaryStream)) == EOF)
      return NULL;

    //exploration
    printf("%c\n", c);

    //exploration
    test2 = fpos;
    printf("test2 = %ld\n", test2);
    /* these 4 explorations have shown that there is always a '\n' at the EOF and this char when printed
    fills an invisible line in the shell*/

    if (c == '\n' && second == 1) /* accept at most one '\n' */
      break;
    second = 1;

    /* I believe this block may stand for DOS/Windows where newlines are encoded with both '\n'+'\r' as well,
    due to History where the handcraft printer machines used to \n to scroll down and \r to come back to the extreme left */
    if (c != '\r') //So I think it's always yes under linux/posix
    {
      //exploration
      printf("yes\n");

      unsigned char ch = c;
      if (cpos == 0)
      {
        memmove(buf + 1, buf, n - 2);
        ++cpos;
      }
      memcpy(buf + --cpos, &ch, 1); //copy 1 octets from ch to buf so I guess it fills buf from last to first char in the line
    } else
    {
      //exploration
      printf("NO\n");
    }

    if (fpos == 0)
    {
      fseek(binaryStream, 0, SEEK_SET);
      break;
    }
  }

  memmove(buf, buf + cpos, n - cpos); /* I think it finally moves buf from cpos, which from the beginning decreased to an
  offset lower than n (=256 in this current case), forgetting every empty char before, to the new buf that will be filled with
  only what is necessary (unempty chars)*/

  //exploration
  printf("n-cpos = %d\n", n-cpos);

  offset = n-cpos;
  //exploration
  printf("localoffset = %d\n", offset);

  return buf;
}

int main(int argc, char* argv[])
{
  FILE* f;
  long sz;
  off_t localoffset, globaloffset;

  if (argc < 2)
  {
    printf("filename parameter required\n");
    return -1;
  }

  if ((f = fopen(argv[1], "rb")) == NULL)
  {
    printf("failed to open file \'%s\'\n", argv[1]);
    return -1;
  }

  sz = fsize(f);
  printf("file size: %ld\n", sz);

  if (sz > 0)
  {
    char buf[256]; //LENGTH MAX OF A LINE I GUESS
    fseek(f, sz, SEEK_SET); //place the cursor after the last char of the last line
    unsigned int count(0);
    while (getOffsetBeforeLastBuf(buf, sizeof(buf), f, localoffset) != NULL)
    {
      ++count;
      printf("localoffset = %ld\n", localoffset);
      if ((count == 1) && (truncate(argv[1], (globaloffset = sz - localoffset)) == 0));
        printf("globaloffset = %ld\n", globaloffset);
        printf("Truncature done\n");
      printf("%s", buf);
      printf("sizeof(buf) = %ld\n", sizeof(buf));
    }
  }

  fclose(f);
  return 0;
}
